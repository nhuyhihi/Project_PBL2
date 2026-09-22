#include <iostream>
#include <vector>
#include "Graph.h"
#include "IOManager.h"
#include "Reporter.h"

int main() {
    std::cout << "========================================\n";
    std::cout << " TEST MODULE REPORTER - B2.4 \n";
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

    return 0;
}
