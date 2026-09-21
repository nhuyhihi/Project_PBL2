#include <iostream>
#include <vector>
#include <cassert>
#include "Graph.h"
#include "IOManager.h"

int main() {
    std::cout << "========================================\n";
    std::cout << " TEST MODULE VALIDATION & IO (File Input)\n";
    std::cout << "========================================\n\n";

    Graph graph;
    std::vector<Packet> packets;

    std::cout << "--- 1. Testing Node Loading & Validation ---\n";
    if (!IOManager::loadNodes("data/nodes.txt", graph)) {
        std::cerr << "Loi: Khong load duoc data/nodes.txt\n";
        return 1;
    }
    std::cout << "=> Valid Node Count in Graph: " << graph.V() << " (Expected: 6)\n";
    assert(graph.V() == 6);

    std::cout << "\n--- 2. Testing Edge Loading & Validation ---\n";
    if (!IOManager::loadEdges("data/edges.txt", graph)) {
        std::cerr << "Loi: Khong load duoc data/edges.txt\n";
        return 1;
    }
    std::cout << "=> Valid Edge Count in Graph: " << graph.E() << " (Expected: 9)\n";
    assert(graph.E() == 9);

    std::cout << "\n--- 3. Testing Packet Loading & Validation ---\n";
    if (!IOManager::loadPackets("data/packets.txt", packets)) {
        std::cerr << "Loi: Khong load duoc data/packets.txt\n";
        return 1;
    }
    std::cout << "=> Valid Packet Count: " << packets.size() << " (Expected: 2)\n";
    assert(packets.size() == 2);

    std::cout << "\n>>> VALIDATION MODULE PASSED ALL TESTS! <<<\n";
    return 0;
}
