#include "IOManager.h"
#include "Validation.h"
#include "EnumUtils.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <cctype>

bool IOManager::isSkippableLine(const std::string& line) {
    if (line.empty()) return true;
    for (char c : line) {
        if (!std::isspace(c)) {
            if (c == '#') return true;
            return false;
        }
    }
    return true;
}


bool IOManager::loadNodes(const std::string& filepath, Graph& graph) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Lỗi: Không thể mở file " << filepath << "\n";
        return false;
    }

    std::string line;
    int lineCount = 0;
    while (std::getline(file, line)) {
        lineCount++;
        if (isSkippableLine(line)) continue;

        std::stringstream ss(line);
        Node node;
        std::string typeStr;

        // Định dạng file: id name type totalPorts processingTime
        if (!(ss >> node.id >> node.name >> typeStr >> node.totalPorts >> node.processingTime)) {
            std::cerr << "Lỗi sai format dòng " << lineCount << " trong " << filepath << "\n";
            continue;
        }

        if (!EnumUtils::parseNodeType(typeStr, node.type)) {
            std::cerr << "Lỗi dữ liệu Enum dòng " << lineCount << ": Unknown NodeType " << typeStr << "\n";
            continue;
        }

        std::string reason;
        if (!Validation::isValidNode(node, reason)) {
            std::cerr << "REJECT Node ID " << node.id << ": " << reason << "\n";
            continue;
        }

        if (!graph.addNode(node)) {
            std::cerr << "REJECT Node ID " << node.id << ": ID đã tồn tại trong đồ thị\n";
            continue;
        }
    }
    return true;
}

bool IOManager::loadEdges(const std::string& filepath, Graph& graph) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Lỗi: Không thể mở file " << filepath << "\n";
        return false;
    }

    std::string line;
    int lineCount = 0;
    while (std::getline(file, line)) {
        lineCount++;
        if (isSkippableLine(line)) continue;

        std::stringstream ss(line);
        Edge edge;
        std::string mediaStr;

        // Định dạng mới (11 cột): id u v media length bw maxSeg terrain mtu load delay
        if (!(ss >> edge.id >> edge.u >> edge.v >> mediaStr >> edge.length 
                 >> edge.bandwidthMbps >> edge.maxSegmentLength 
                 >> edge.terrainFactor 
                 >> edge.mtu >> edge.currentLoad >> edge.queueDelay)) {
            std::cerr << "Lỗi sai format dòng " << lineCount << " trong " << filepath << "\n";
            continue;
        }

        if (!EnumUtils::parseMediaType(mediaStr, edge.mediaType)) {
            std::cerr << "Lỗi dữ liệu Enum dòng " << lineCount << ": Unknown MediaType " << mediaStr << "\n";
            continue;
        }

        std::string reason;
        if (!Validation::isValidEdge(edge, graph, reason)) {
            std::cerr << "REJECT Edge ID " << edge.id << ": " << reason << "\n";
            continue;
        }

        if (!graph.addEdge(edge)) {
            std::cerr << "REJECT Edge ID " << edge.id << ": Cạnh trùng ID hoặc tự nối chính nó\n";
            continue;
        }
    }
    return true;
}

bool IOManager::loadPackets(const std::string& filepath, std::vector<Packet>& packets, const Graph* graph) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Lỗi: Không thể mở file " << filepath << "\n";
        return false;
    }

    std::string line;
    int lineCount = 0;
    while (std::getline(file, line)) {
        lineCount++;
        if (isSkippableLine(line)) continue;

        std::stringstream ss(line);
        Packet pkt;
        std::string typeStr, protoStr;
        int canFrag, newConn;

        // Định dạng: id src dest size type protocol canFragment newConnection
        if (!(ss >> pkt.id >> pkt.source >> pkt.destination >> pkt.sizeBytes 
                 >> typeStr >> protoStr >> canFrag >> newConn)) {
            std::cerr << "Lỗi sai format dòng " << lineCount << " trong " << filepath << "\n";
            continue;
        }

        if (!EnumUtils::parsePacketType(typeStr, pkt.type) || !EnumUtils::parseProtocol(protoStr, pkt.protocol)) {
            std::cerr << "Lỗi dữ liệu Enum dòng " << lineCount << ": Unknown PacketType (" << typeStr 
                      << ") hoặc Protocol (" << protoStr << ")\n";
            continue;
        }
        
        pkt.canFragment = (canFrag != 0);
        pkt.newConnection = (newConn != 0);

        std::string reason;
        bool valid = graph ? Validation::isValidPacket(pkt, *graph, reason)
                           : Validation::isValidPacket(pkt, reason);
        if (!valid) {
            std::cerr << "REJECT Packet ID " << pkt.id << ": " << reason << "\n";
            continue;
        }

        packets.push_back(pkt);
    }
    return true;
}

bool IOManager::loadScenario(const std::string& filepath, Scenario& scenario) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Lỗi: Không thể mở file " << filepath << "\n";
        return false;
    }

    // Gán tên kịch bản từ tên file
    scenario.name = filepath;
    scenario.events.clear();

    std::string line;
    int lineCount = 0;
    while (std::getline(file, line)) {
        lineCount++;
        if (isSkippableLine(line)) continue;

        std::stringstream ss(line);
        ScenarioEvent ev;
        std::string evTypeStr;

        // Định dạng file: [Time] [EventType] [TargetEdgeId] [Value]
        if (!(ss >> ev.time >> evTypeStr >> ev.targetId >> ev.value)) {
            std::cerr << "Lỗi sai format dòng " << lineCount << " trong " << filepath << "\n";
            continue;
        }

        if (!EnumUtils::parseEventType(evTypeStr, ev.eventType)) {
            std::cerr << "Lỗi dữ liệu Enum dòng " << lineCount << ": Unknown EventType " << evTypeStr << "\n";
            continue;
        }

        scenario.events.push_back(ev);
    }
    return true;
}
