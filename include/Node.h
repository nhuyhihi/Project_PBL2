#ifndef NODE_H
#define NODE_H

#include <string>
#include "Types.h"

struct Node {
    int id;
    std::string name;
    NodeType type;
    int totalPorts;
    int usedPorts;
    double processingTime;

    Node() : id(-1), type(NodeType::ROUTER), totalPorts(0), usedPorts(0), processingTime(0.0) {}
};

#endif 
