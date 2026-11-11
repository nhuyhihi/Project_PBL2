#ifndef VALIDATION_H
#define VALIDATION_H

#include <string>
#include "Node.h"
#include "Edge.h"
#include "Packet.h"
#include "Graph.h"

// Kiểm tra tính hợp lệ dữ liệu đầu vào và ràng buộc kỹ thuật
namespace Validation {
    bool isValidNode(const Node& node, std::string& reason);
    bool isValidEdge(const Edge& edge, const Graph& graph, std::string& reason);
    bool isValidPacket(const Packet& packet, std::string& reason);
    bool isValidPacket(const Packet& packet, const Graph& graph, std::string& reason);

    bool checkMaxLength(const Edge& edge);
    bool checkPortCapacity(const Node& node);
    bool isStaticFeasible(const Edge& edge);
}

#endif // VALIDATION_H
