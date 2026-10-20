#ifndef VALIDATION_H
#define VALIDATION_H

#include <string>
#include "Node.h"
#include "Edge.h"
#include "Packet.h"
#include "Graph.h"

namespace Validation {
    // Kiểm tra hợp lệ cho Node
    bool isValidNode(const Node& node, std::string& reason);
    
    // Kiểm tra hợp lệ cho Edge.
    bool isValidEdge(const Edge& edge, const Graph& graph, std::string& reason);
    
    // Kiểm tra hợp lệ cho Packet (sizeBytes > 0)
    bool isValidPacket(const Packet& packet, std::string& reason);

    // Kiểm tra hợp lệ cho Packet kèm kiểm tra node nguồn và đích có tồn tại trong Graph
    bool isValidPacket(const Packet& packet, const Graph& graph, std::string& reason);

    bool checkMaxLength(const Edge& edge);
    bool checkPortCapacity(const Node& node);
    bool isStaticFeasible(const Edge& edge);
}

#endif // VALIDATION_H
