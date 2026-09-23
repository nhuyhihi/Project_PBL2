#include <iostream>
#include <vector>
#include "Graph.h"
#include "IOManager.h"
#include "Reporter.h"

int main() {
    std::cout << "========================================\n";
    std::cout << " TEST MODULE REPORTER  \n";
    std::cout << "========================================\n\n";

    Graph graph;
    if (!IOManager::loadNodes("data/nodes.txt", graph)) return 1;
    if (!IOManager::loadEdges("data/edges.txt", graph)) return 1;

    // Giả lập kết quả của thuật toán Kruskal (Do Quyến chưa code xong)
    // MST Edges (Mục tiêu tổng: 6544 ĐV)
    std::vector<int> mstEdges = {0, 1, 4, 6, 7}; // e0, e1, e4, e6, e7
    for (int id : mstEdges) {
        if (graph.hasEdge(id)) {
            graph.getEdge(id).isBuilt = true;
        }
    }

    // Backup Edges (Mục tiêu tổng MST + Backup: 12440 ĐV)
    std::vector<int> backupEdges = {2, 3}; // e2, e3
    for (int id : backupEdges) {
        if (graph.hasEdge(id)) {
            graph.getEdge(id).isBackup = true;
        }
    }

    // Gọi module Reporter để in báo cáo
    Reporter::printMSTReport(graph);

    // Load packets để test phần Dijkstra
    std::vector<Packet> packets;
    if (!IOManager::loadPackets("data/packets.txt", packets)) {
        std::cout << "Khong the load packets.txt\n";
    } else {
        // In báo cáo định tuyến Dijkstra
        Reporter::printPacketRouteReport(graph, packets);
    }

    return 0;
}
