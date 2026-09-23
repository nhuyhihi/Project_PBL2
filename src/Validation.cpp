#include "Validation.h"

namespace Validation {

bool isValidNode(const Node& node, std::string& reason) {
    if (node.totalPorts < 0) {
        reason = "totalPorts không được âm";
        return false;
    }
    return true;
}

bool isValidEdge(const Edge& edge, const Graph& graph, std::string& reason) {
    try {
        graph.getNode(edge.u);
    } catch (...) {
        reason = "Node u (" + std::to_string(edge.u) + ") không tồn tại";
        return false;
    }
    
    try {
        graph.getNode(edge.v);
    } catch (...) {
        reason = "Node v (" + std::to_string(edge.v) + ") không tồn tại";
        return false;
    }

    if (edge.bandwidthMbps <= 0) {
        reason = "bandwidthMbps phải lớn hơn 0";
        return false;
    }
    if (edge.mtu <= 0) {
        reason = "mtu phải lớn hơn 0";
        return false;
    }
    if (edge.currentLoad < 0 || edge.currentLoad > 1) {
        reason = "currentLoad phải nằm trong khoảng [0, 1]";
        return false;
    }
    return true;
}

bool isValidPacket(const Packet& packet, std::string& reason) {
    if (packet.sizeBytes < 0) {
        reason = "sizeBytes không được âm";
        return false;
    }
    return true;
}

bool checkMaxLength(const Edge& edge) {
    return edge.length <= edge.maxSegmentLength;
}

bool checkPortCapacity(const Node& node) {
    return node.usedPorts < node.totalPorts;
}

bool isStaticFeasible(const Edge& edge) {
    return checkMaxLength(edge);
}

}
