// ============================================================
//
// Quy trinh dung 100% theo so do Tuan 3:
//   Load du lieu -> Kruskal (MST + Backup) -> Reporter (chi phi ha tang)
//   -> Dijkstra dinh tuyen tung packet -> DelayModel (do tre/MTU/TCP/UDP)
//
// Dung du lieu goc trong thu muc data/ (nodes.txt, edges.txt, packets.txt).
// ============================================================
#include <iostream>
#include <vector>
#include "Graph.h"
#include "IOManager.h"
#include "Kruskal.h"
#include "CostModel.h"
#include "Reporter.h"
#include "RouteReport.h"

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

    // Ham chi phi that: dung CostModel (module cua Y) thay vi cong thuc tam
    auto realCost = [](const Edge& e) { return CostModel::calculateCablingCost(e).total; };

    // Buoc 1: Kruskal xay cay khung (MST) - chon cac day re nhat, khong
    // vuot ton cong, khong tao chu trinh (module cua Quyen, Tuan 1-2)
    std::cout << "\n=== XAY DUNG HA TANG (KRUSKAL) ===\n";
    MSTResult mst = buildMST(graph, realCost);
    std::cout << "=> Da chon " << mst.edgeIds.size() << " day cho MST, "
              << "chi phi = " << mst.totalCost
              << (mst.connected ? " (da noi het cac node)\n" : " (CHUA noi het cac node!)\n");

    // Buoc 2: chon them day du phong de mang co duong thay the khi dut day
    BackupResult backup = selectBackupLinks(graph, mst, DEFAULT_BACKUP_LINKS, realCost);
    std::cout << "=> Da chon " << backup.edgeIds.size() << " day du phong, "
              << "bao ve them " << backup.protectedTreeEdges << "/" << backup.treeEdges
              << " canh cay.\n";

    // Buoc 3: bao cao chi phi ha tang (module cua Y) - so sanh Build All vs
    // MST only vs MST + Backup
    Reporter::printMSTReport(graph);

    // Buoc 4: dinh tuyen tung packet tren HA TANG DA XAY (Dijkstra cua
    // Quyen, onlyBuilt=true) roi tinh do tre bang DelayModel (cua Y)
    RouteReport::printPacketRouteReport(graph, packets, lengthWeight);

    std::cout << "\n=== KET THUC DEMO ===\n";
    return 0;
}
