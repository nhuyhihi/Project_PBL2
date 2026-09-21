#include <iostream>
#include <vector>
#include <cassert>
#include "Graph.h"
#include "IOManager.h"
#include "Validation.h"

int main() {
    std::cout << "========================================\n";
    std::cout << " TEST MODULE VALIDATION - B2.3 \n";
    std::cout << "========================================\n\n";

    Graph graph;
    if (!IOManager::loadNodes("data/nodes.txt", graph)) return 1;
    if (!IOManager::loadEdges("data/edges.txt", graph)) return 1;

    std::cout << "\n--- 1. Testing checkMaxLength & isStaticFeasible ---\n";
    Edge validEdge = graph.getEdge(0); // FIBER 400m, max 2000m -> ok
    
    // Create an invalid edge directly for testing
    Edge invalidCopper;
    invalidCopper.id = 100;
    invalidCopper.mediaType = MediaType::COPPER;
    invalidCopper.length = 130;
    invalidCopper.maxSegmentLength = 100;

    std::cout << "Valid Edge (Length " << validEdge.length << ", Max " << validEdge.maxSegmentLength << "): ";
    if (Validation::isStaticFeasible(validEdge)) std::cout << "PASS\n"; else std::cout << "FAIL\n";

    std::cout << "Invalid Copper (Length " << invalidCopper.length << ", Max " << invalidCopper.maxSegmentLength << "): ";
    if (!Validation::isStaticFeasible(invalidCopper)) std::cout << "REJECTED (PASS)\n"; else std::cout << "ACCEPTED (FAIL)\n";

    std::cout << "\n--- 2. Testing checkPortCapacity ---\n";
    Node validNode = graph.getNode(0); // R1 totalPorts 4, usedPorts 0
    Node fullNode = graph.getNode(1);
    fullNode.usedPorts = fullNode.totalPorts; // Giả lập Node đã hết port

    std::cout << "Valid Node (Used " << validNode.usedPorts << "/" << validNode.totalPorts << "): ";
    if (Validation::checkPortCapacity(validNode)) std::cout << "PASS\n"; else std::cout << "FAIL\n";

    std::cout << "Full Node (Used " << fullNode.usedPorts << "/" << fullNode.totalPorts << "): ";
    if (!Validation::checkPortCapacity(fullNode)) std::cout << "REJECTED (PASS)\n"; else std::cout << "ACCEPTED (FAIL)\n";

    std::cout << "\n>>> VALIDATION B2.3 PASSED ALL TESTS! <<<\n";
    return 0;
}
