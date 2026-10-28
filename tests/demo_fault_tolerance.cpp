// ============================================================
// Demo tuan 5 (Quyen) - Task A5.5: bang chi so chiu loi tren DU LIEU THAT
// (data/nodes.txt, data/edges.txt, data/pricing.txt), so sanh MST vs MST + backup.
// Chay tu thu muc goc repo:
//   g++ -std=c++17 -Iinclude src/*.cpp tests/demo_fault_tolerance.cpp -o demo_fault_tolerance
//   ./demo_fault_tolerance
// ============================================================
#include <iostream>
#include <iomanip>

#include "Graph.h"
#include "IOManager.h"
#include "CostModel.h"
#include "Kruskal.h"
#include "Connectivity.h"

int main() {
    Graph graph;
    if (!IOManager::loadNodes("data/nodes.txt", graph) ||
        !IOManager::loadEdges("data/edges.txt", graph)) {
        std::cout << "Khong doc duoc du lieu trong data/ (hay chay tu thu muc goc repo)\n";
        return 1;
    }

    CostModel pricing;
    pricing.loadPricing("data/pricing.txt");
    auto costFn = [&pricing](const Edge& e) { return pricing.calculateCablingCost(e).total; };

    MSTResult    mst    = buildMST(graph, costFn);
    BackupResult backup = selectBackupLinks(graph, mst, 2, costFn);

    std::cout << "\nSo node: " << graph.V()
              << " | Day cay khung: " << mst.edgeIds.size()
              << " | Day du phong: " << backup.edgeIds.size() << "\n";

    // Trang thai hien tai (chua dut day nao)
    auto now = Connectivity::pairConnectivity(graph, /*onlyBuilt=*/true);
    std::cout << "Hien tai: " << now.connectedPairs << "/" << now.totalPairs
              << " cap node lien lac duoc (" << std::fixed << std::setprecision(1)
              << now.ratio * 100 << "%)\n";

    auto report = Connectivity::compareSingleFailures(graph, mst.edgeIds, backup.edgeIds);
    Connectivity::printFaultToleranceReport(report);
    return 0;
}
