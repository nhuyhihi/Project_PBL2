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

NodeType IOManager::stringToNodeType(const std::string& str) {
    NodeType type;
    if (EnumUtils::parseNodeType(str, type)) return type;
    throw std::invalid_argument("Unknown NodeType: " + str);
}

MediaType IOManager::stringToMediaType(const std::string& str) {
    MediaType media;
    if (EnumUtils::parseMediaType(str, media)) return media;
    throw std::invalid_argument("Unknown MediaType: " + str);
}

Protocol IOManager::stringToProtocol(const std::string& str) {
    Protocol proto;
    if (EnumUtils::parseProtocol(str, proto)) return proto;
    throw std::invalid_argument("Unknown Protocol: " + str);
}

PacketType IOManager::stringToPacketType(const std::string& str) {
    PacketType pktType;
    if (EnumUtils::parsePacketType(str, pktType)) return pktType;
    throw std::invalid_argument("Unknown PacketType: " + str);
}

EventType IOManager::stringToEventType(const std::string& str) {
    EventType evType;
    if (EnumUtils::parseEventType(str, evType)) return evType;
    throw std::invalid_argument("Unknown EventType: " + str);
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

        try {
            node.type = stringToNodeType(typeStr);
        } catch (const std::exception& e) {
            std::cerr << "Lỗi dữ liệu Enum dòng " << lineCount << ": " << e.what() << "\n";
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

        // Định dạng: id u v media length bw maxSeg price terrain equip maint mtu load delay
        if (!(ss >> edge.id >> edge.u >> edge.v >> mediaStr >> edge.length 
                 >> edge.bandwidthMbps >> edge.maxSegmentLength >> edge.unitPrice 
                 >> edge.terrainFactor >> edge.equipmentCost >> edge.maintenanceCost 
                 >> edge.mtu >> edge.currentLoad >> edge.queueDelay)) {
            std::cerr << "Lỗi sai format dòng " << lineCount << " trong " << filepath << "\n";
            continue;
        }

        try {
            edge.mediaType = stringToMediaType(mediaStr);
        } catch (const std::exception& e) {
            std::cerr << "Lỗi dữ liệu Enum dòng " << lineCount << ": " << e.what() << "\n";
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

        try {
            pkt.type = stringToPacketType(typeStr);
            pkt.protocol = stringToProtocol(protoStr);
        } catch (const std::exception& e) {
            std::cerr << "Lỗi dữ liệu Enum dòng " << lineCount << ": " << e.what() << "\n";
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

        try {
            ev.eventType = stringToEventType(evTypeStr);
        } catch (const std::exception& e) {
            std::cerr << "Lỗi dữ liệu Enum dòng " << lineCount << ": " << e.what() << "\n";
            continue;
        }

        scenario.events.push_back(ev);
    }
    return true;
}
