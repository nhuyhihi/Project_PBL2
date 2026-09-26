#include <iostream>
#include <vector>
#include "Graph.h"
#include "IOManager.h"
#include "Kruskal.h"
#include "CostModel.h"
#include "Reporter.h"

int main() {
    std::cout << "=== KHOI TAO DO THI PBL2 ===\n";
    Graph graph;
    
    std::cout << "\n--- Load Nodes ---\n";
    if (IOManager::loadNodes("data/nodes.txt", graph)) {
        std::cout << "=> So luong node hien tai: " << graph.V() << "\n";
    }
    
    std::cout << "\n--- Load Edges ---\n";
    if (IOManager::loadEdges("data/edges.txt", graph)) {
        std::cout << "=> So luong edge hien tai: " << graph.E() << "\n";
    }
    
    std::cout << "\n--- Load Packets ---\n";
    std::vector<Packet> packets;
    if (IOManager::loadPackets("data/packets.txt", packets)) {
        std::cout << "=> So luong packet load thanh cong: " << packets.size() << "\n";
    }

    std::cout << "\n=== TIEN HANH QUY HOACH MANG LUI (KRUSKAL) ===\n";
    auto costFn = [](const Edge& e) { return CostModel::calculateCablingCost(e).total; };
    MSTResult mstResult = buildMST(graph, costFn);
    if (mstResult.connected) {
        std::cout << "=> Da ket noi thanh cong toan bo mang luoi!\n";
    } else {
        std::cout << "=> Canh bao: Mang bi chia cat, khong the ket noi tat ca cac nut.\n";
    }

    std::cout << "\n=== TIEN HANH TIM CAP DU PHONG ===\n";
    BackupResult backupResult = selectBackupLinks(graph, mstResult, 2, costFn);
    std::cout << "=> Da chon them " << backupResult.edgeIds.size() << " cap du phong.\n";

    std::cout << "\n=== IN BAO CAO CHI PHI VA DINH TUYEN ===\n";
    Reporter::printMSTReport(graph);
    Reporter::printPacketRouteReport(graph, packets);

    std::cout << "\n=== KET THUC CHUONG TRINH ===\n";
    return 0;
}
