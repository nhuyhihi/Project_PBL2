#include "IOManager.h"
#include "Validation.h"
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
    if (str == "ROUTER") return NodeType::ROUTER;
    if (str == "HOST") return NodeType::HOST;
    if (str == "SWITCH") return NodeType::SWITCH;
    throw std::invalid_argument("Unknown NodeType: " + str);
}

MediaType IOManager::stringToMediaType(const std::string& str) {
    if (str == "FIBER") return MediaType::FIBER;
    if (str == "COPPER") return MediaType::COPPER;
    if (str == "WIRELESS") return MediaType::WIRELESS;
    throw std::invalid_argument("Unknown MediaType: " + str);
}

Protocol IOManager::stringToProtocol(const std::string& str) {
    if (str == "TCP") return Protocol::TCP;
    if (str == "UDP") return Protocol::UDP;
    throw std::invalid_argument("Unknown Protocol: " + str);
}

PacketType IOManager::stringToPacketType(const std::string& str) {
    if (str == "REAL_TIME") return PacketType::REAL_TIME;
    if (str == "BULK_DATA") return PacketType::BULK_DATA;
    if (str == "CONTROL") return PacketType::CONTROL;
    throw std::invalid_argument("Unknown PacketType: " + str);
}

EventType IOManager::stringToEventType(const std::string& str) {
    if (str == "LINK_DOWN") return EventType::LINK_DOWN;
    if (str == "LINK_UP") return EventType::LINK_UP;
    if (str == "LOAD_UPDATE") return EventType::LOAD_UPDATE;
    if (str == "DELAY_UPDATE") return EventType::DELAY_UPDATE;
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

        graph.addNode(node);
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

        graph.addEdge(edge);
    }
    return true;
}

bool IOManager::loadPackets(const std::string& filepath, std::vector<Packet>& packets) {
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
        if (!Validation::isValidPacket(pkt, reason)) {
            std::cerr << "REJECT Packet ID " << pkt.id << ": " << reason << "\n";
            continue;
        }

        packets.push_back(pkt);
    }
    return true;
}
