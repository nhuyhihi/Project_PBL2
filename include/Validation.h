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
    // Lưu ý: Việc tham chiếu node có tồn tại không sẽ được Graph kiểm tra 
    // hoặc có thể truyền Graph vào để kiểm tra.
    bool isValidEdge(const Edge& edge, const Graph& graph, std::string& reason);
    
    // Kiểm tra hợp lệ cho Packet
    bool isValidPacket(const Packet& packet, std::string& reason);
}

#endif // VALIDATION_H
