#include "Simulator.h"
#include "DelayModel.h"
#include "Config.h"
#include "Dijkstra.h"
#include "RoutingCost.h"
#include "FailureHandler.h"
#include "EnumUtils.h"
#include "Logger.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <unordered_map>

// Áp dụng các sự kiện thay đổi trạng thái mạng (Load, Delay, Link Up/Down)
void Simulator::applyEvent(Graph& graph, const ScenarioEvent& event) {
    if (!graph.hasEdge(event.targetId)) {
        std::cerr << "[Simulator] Canh bao: Event bo qua do Edge #" << event.targetId << " khong ton tai trong do thi!\n";
        return;
    }

    Edge& targetEdge = graph.getEdge(event.targetId);

    switch (event.eventType) {
        case EventType::LOAD_UPDATE:
            targetEdge.currentLoad = event.value;
            break;

        case EventType::DELAY_UPDATE:
            targetEdge.queueDelay = event.value;
            break;

        case EventType::LINK_DOWN:
        case EventType::LINK_UP:
            FailureHandler::applyLinkEvent(graph, event.eventType, event.targetId);
            break;

        default:
            break;
    }
}

// Đánh giá kết quả truyền gói tin (Read-Only: nhận const Graph&, không làm bẩn currentLoad)
SimulationResult Simulator::evaluatePacket(const Packet& packet, const PathResult& path, const Graph& graph) {
    SimulationResult result;

    if (!path.reachable || path.edges.empty()) {
        result.success = false;
        result.failReason = "NO_ROUTE_AVAILABLE";
        return result;
    }

    for (size_t i = 0; i < path.edges.size(); ++i) {
        int edgeId = path.edges[i];
        if (!graph.hasEdge(edgeId)) {
            result.success = false;
            result.failReason = "EDGE_NOT_FOUND_ERROR";
            return result;
        }
        const Edge& edge = graph.getEdge(edgeId);

        int nextNodeId = (i + 1 < path.nodes.size()) ? path.nodes[i + 1] : graph.otherEndpoint(path.nodes[i], edgeId);
        if (!graph.hasNode(nextNodeId)) {
            result.success = false;
            result.failReason = "NODE_NOT_FOUND_ERROR";
            return result;
        }
        const Node& nextNode = graph.getNode(nextNodeId);

        int fragments = DelayModel::calcFragmentCount(packet.sizeBytes, edge.mtu, packet.canFragment);
        if (fragments == 0) {
            result.success = false;
            result.failReason = "DF_BLOCKED_BY_MTU_AT_EDGE_" + std::to_string(edgeId);
            return result;
        }
        result.fragmentCount = std::max(result.fragmentCount, fragments);

        double edgeDelay = DelayModel::calculateHopDelay(packet, edge, nextNode);
        if (edgeDelay == Config::INF) {
            result.success = false;
            result.failReason = "LINK_DOWN_OR_UNREACHABLE";
            return result;
        }
        result.totalDelay += edgeDelay;
    }

    if (packet.protocol == Protocol::TCP) {
        result.totalDelay += DelayModel::calcProtocolOverhead(packet, result.totalDelay);
    }

    return result;
}

// Gửi gói tin thực tế: đánh giá độ trễ qua evaluatePacket rồi cập nhật tải trên Graph
SimulationResult Simulator::sendPacket(const Packet& packet, const PathResult& path, Graph& graph) {
    SimulationResult result = evaluatePacket(packet, path, graph);
    if (!result.success) {
        return result;
    }

    for (int edgeId : path.edges) {
        Edge& edge = graph.getEdge(edgeId);
        double dTrans = DelayModel::calcTransmissionDelay(packet.sizeBytes, edge.bandwidthMbps);
        updateNetworkState(edge, dTrans);
    }

    return result;
}

void Simulator::updateNetworkState(Edge& edge, double dTrans) {
    double loadDelta = dTrans / Config::LOAD_WINDOW_SECONDS;
    edge.currentLoad = std::max(0.0, std::min(Config::MAX_LOAD, edge.currentLoad + loadDelta));
    edge.currentLoad *= Config::LOAD_DECAY_RATE;
}

void Simulator::runScenarioSimulation(Graph& graph,
                                     const std::vector<Packet>& packets,
                                     const Scenario& scenario,
                                     const std::string& logFilePath) {
    std::cout << "\n========================================================================\n";
    std::cout << "        TIEN HANH CHAY KICH BAN MO PHONG: " << scenario.name << "\n";
    std::cout << "========================================================================\n";
    std::cout << "-> Tong so su kien duoc nap: " << scenario.events.size() << "\n";

    std::vector<Logger::SimulationLogEntry> logEntries;

    if (scenario.events.empty()) {
        std::cout << "(Kich ban tinh - Khong co su kien dong, tien hanh kiem tra truyen goi tin truc tiep):\n";
        const Graph snapshot = graph;
        for (const Packet& pkt : packets) {
            PathResult path = dijkstra(snapshot, pkt.source, pkt.destination,
                                       RoutingCost::makeWeightFn(pkt), /*onlyBuilt=*/true);

            std::string pktTypeStr = EnumUtils::toString(pkt.type);

            std::cout << "  * Packet #" << pkt.id << " (" << pkt.source << "->" << pkt.destination 
                      << ", " << pkt.sizeBytes << "B, DF=" << (pkt.canFragment ? "NO" : "YES") << ", " << pktTypeStr << "): ";

            Logger::SimulationLogEntry entry;
            entry.timestamp = 0.0;
            entry.packetId = pkt.id;
            entry.source = pkt.source;
            entry.destination = pkt.destination;
            entry.packetType = pktTypeStr;
            entry.failureEvent = "";
            entry.isRerouted = false;

            if (path.reachable) {
                std::string routeStr = "";
                for (size_t i = 0; i < path.nodes.size(); ++i) {
                    routeStr += std::to_string(path.nodes[i]) + (i + 1 < path.nodes.size() ? "->" : "");
                }
                std::cout << "Route: " << routeStr;
                entry.path = routeStr;
                entry.hopCount = path.hopCount;

                SimulationResult res = sendPacket(pkt, path, graph);
                entry.fragmentCount = res.fragmentCount;
                if (res.success) {
                    entry.status = "SUCCESS";
                    entry.delay = res.totalDelay;
                    entry.dropReason = "";
                    std::cout << " => SUCCESS (Delay: " << std::fixed << std::setprecision(6) << res.totalDelay 
                              << "s, So manh Fragment: " << res.fragmentCount << ")\n";
                } else {
                    entry.status = "DROP";
                    entry.delay = 0.0;
                    entry.dropReason = res.failReason;
                    std::cout << " => DROP (" << res.failReason << ")\n";
                }
            } else {
                std::cout << "=> DROP (UNREACHABLE: Khong tim thay duong di)\n";
                entry.path = "UNREACHABLE";
                entry.hopCount = 0;
                entry.fragmentCount = 0;
                entry.status = "DROP";
                entry.delay = 0.0;
                entry.dropReason = "NO_PATH_AVAILABLE";
            }
            logEntries.push_back(entry);
        }
        std::cout << "========================================================================\n";
        if (!logFilePath.empty()) {
            if (Logger::exportSimulationLog(logFilePath, scenario.name, logEntries)) {
                std::cout << "[Logger] Da xuat log mo phong thanh cong ra: " << logFilePath << "\n";
            }
        }
        return;
    }

    std::vector<std::pair<double, std::vector<ScenarioEvent>>> timeGroups;
    for (const auto& ev : scenario.events) {
        if (timeGroups.empty() || std::abs(timeGroups.back().first - ev.time) > 1e-6) {
            timeGroups.push_back({ev.time, {ev}});
        } else {
            timeGroups.back().second.push_back(ev);
        }
    }

    std::unordered_map<int, std::vector<int>> lastRoute;
    std::unordered_map<int, bool> lastSuccess;

    for (const auto& group : timeGroups) {
        double t = group.first;
        std::cout << "\n------------------------------------------------------------------------\n";
        std::cout << "[Thoi diem: " << std::fixed << std::setprecision(1) << t << "s]\n";

        std::string failureSummary = "";

        // 1. Áp dụng tất cả sự kiện xảy ra tại thời điểm t
        for (const auto& ev : group.second) {
            std::string evDesc = "";
            if (ev.eventType == EventType::LOAD_UPDATE) {
                std::cout << "  * Su kien: LOAD_UPDATE tren Edge #" << ev.targetId 
                          << " -> Tai moi: " << std::setprecision(2) << ev.value * 100 << "%\n";
                evDesc = "LOAD_UPDATE Edge #" + std::to_string(ev.targetId);
            } else if (ev.eventType == EventType::DELAY_UPDATE) {
                std::cout << "  * Su kien: DELAY_UPDATE tren Edge #" << ev.targetId 
                          << " -> Tro ngai moi: " << std::setprecision(4) << ev.value << "s\n";
                evDesc = "DELAY_UPDATE Edge #" + std::to_string(ev.targetId);
            } else if (ev.eventType == EventType::LINK_DOWN) {
                std::cout << "  * CANH BAO: LINK_DOWN tren Edge #" << ev.targetId << " (Cap bi dut / hong hoc!)\n";
                evDesc = "LINK_DOWN Edge #" + std::to_string(ev.targetId);
            } else if (ev.eventType == EventType::LINK_UP) {
                std::cout << "  * PHUC HOI: LINK_UP tren Edge #" << ev.targetId << " (Cap da hoat dong tro lai)\n";
                evDesc = "LINK_UP Edge #" + std::to_string(ev.targetId);
            }
            if (!evDesc.empty()) {
                if (!failureSummary.empty()) failureSummary += ", ";
                failureSummary += evDesc;
            }
            applyEvent(graph, ev);
        }

        // 2. Chụp bản sao bất biến trạng thái mạng tại mốc t để mọi gói tin cùng thời điểm định tuyến trên trạng thái cố định
        const Graph snapshot = graph;

        // 3. Mô phỏng truyền các gói tin tại thời điểm t
        std::cout << "  -> Ket qua dinh tuyen va truyen goi tin:\n";
        for (const Packet& pkt : packets) {
            PathResult path = dijkstra(snapshot, pkt.source, pkt.destination,
                                       RoutingCost::makeWeightFn(pkt), /*onlyBuilt=*/true);

            std::string pktTypeStr = EnumUtils::toString(pkt.type);

            std::cout << "     Packet #" << pkt.id << " (" << pkt.source << "->" << pkt.destination 
                      << ", " << pktTypeStr << "): ";

            Logger::SimulationLogEntry entry;
            entry.timestamp = t;
            entry.packetId = pkt.id;
            entry.source = pkt.source;
            entry.destination = pkt.destination;
            entry.packetType = pktTypeStr;
            entry.failureEvent = failureSummary;

            if (path.reachable) {
                std::string routeStr = "";
                for (size_t i = 0; i < path.nodes.size(); ++i) {
                    routeStr += std::to_string(path.nodes[i]) + (i + 1 < path.nodes.size() ? "->" : "");
                }

                bool routeChanged = (lastRoute.find(pkt.id) != lastRoute.end() && lastRoute[pkt.id] != path.nodes);
                entry.isRerouted = routeChanged;
                entry.path = routeStr;
                entry.hopCount = path.hopCount;

                if (routeChanged) {
                    std::cout << " [DA DOI DUONG (REROUTE)] Route: " << routeStr;
                } else {
                    std::cout << " Route: " << routeStr;
                }
                lastRoute[pkt.id] = path.nodes;

                SimulationResult res = sendPacket(pkt, path, graph);
                entry.fragmentCount = res.fragmentCount;
                if (res.success) {
                    entry.status = "SUCCESS";
                    entry.delay = res.totalDelay;
                    entry.dropReason = "";
                    std::cout << " => SUCCESS (Delay: " << std::setprecision(6) << res.totalDelay 
                              << "s, Frags: " << res.fragmentCount << ")\n";
                } else {
                    entry.status = "DROP";
                    entry.delay = 0.0;
                    entry.dropReason = res.failReason;
                    std::cout << " => DROP (" << res.failReason << ")\n";
                }
                lastSuccess[pkt.id] = res.success;
            } else {
                std::cout << "=> DROP (UNREACHABLE: Mang bi chia cat, khong con duong)\n";
                lastRoute[pkt.id] = {};
                lastSuccess[pkt.id] = false;
                entry.path = "UNREACHABLE";
                entry.hopCount = 0;
                entry.fragmentCount = 0;
                entry.status = "DROP";
                entry.delay = 0.0;
                entry.dropReason = "NO_PATH_AVAILABLE";
                entry.isRerouted = false;
            }
            logEntries.push_back(entry);
        }
    }
    std::cout << "========================================================================\n";
    if (!logFilePath.empty()) {
        if (Logger::exportSimulationLog(logFilePath, scenario.name, logEntries)) {
            std::cout << "[Logger] Da xuat log mo phong thanh cong ra: " << logFilePath << "\n";
        }
    }
}
