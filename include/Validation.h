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
    
    // Kiểm tra hợp lệ cho Packet
    bool isValidPacket(const Packet& packet, std::string& reason);

    bool checkMaxLength(const Edge& edge);
    bool checkPortCapacity(const Node& node);
    bool isStaticFeasible(const Edge& edge);
}

#endif // VALIDATION_H
