// ============================================================
// Demo/Test: noi Kruskal -> Dijkstra -> DelayModel qua RouteReport.
//
// QUAN TRONG: RouteReport dung dijkstra(..., onlyBuilt=true), tuc la
// goi tin CHI duoc di tren day da isBuilt=true. Vi vay PHAI chay
// Kruskal::buildMST + Kruskal::selectBackupLinks TRUOC, de danh dau
// cac day thuc su duoc xay (MST + Backup). Neu bo qua buoc nay,
// khong day nao co isBuilt=true va moi goi tin se bao "KHONG CO DUONG DI".
//
// Chay tu thu muc goc repo (viet tren 1 dong):
//   g++ -std=c++17 -Wall -Wextra -Iinclude src/Graph.cpp src/IOManager.cpp src/Validation.cpp src/DSU.cpp src/Kruskal.cpp src/CostModel.cpp src/Dijkstra.cpp src/DelayModel.cpp src/RouteReport.cpp tests/test_route_report.cpp -o test_route_report
//   ./test_route_report
// ============================================================
#include <iostream>
#include <vector>
#include "Graph.h"
#include "IOManager.h"
#include "Kruskal.h"
#include "CostModel.h"
#include "RouteReport.h"

int main() {
    Graph graph;
    IOManager::loadNodes("data/nodes.txt", graph);
    IOManager::loadEdges("data/edges.txt", graph);

    std::vector<Packet> packets;
    IOManager::loadPackets("data/packets.txt", packets);

    // Ham chi phi that (module cua Y) - dung de Kruskal chon day xay
    auto realCost = [](const Edge& e) { return CostModel::calculateCablingCost(e).total; };

    // Buoc 1-2: xay ha tang truoc khi dinh tuyen goi tin (dung thu tu
    // trong so do Tuan 3: MST + Backup -> Dijkstra -> ...)
    MSTResult mst = buildMST(graph, realCost);
    selectBackupLinks(graph, mst, DEFAULT_BACKUP_LINKS, realCost);

    std::cout << "Da xay MST (" << mst.edgeIds.size() << " day, chi phi "
              << mst.totalCost << ") + day du phong.\n";

    // Buoc 3: dinh tuyen tung goi tin CHI tren ha tang da xay, roi tinh do tre
    RouteReport::printPacketRouteReport(graph, packets, lengthWeight);
    return 0;
}
