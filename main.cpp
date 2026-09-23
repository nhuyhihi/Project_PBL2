// ============================================================
// main.cpp - Chuong trinh demo tong hop toan bo he thong PBL2.
//
// Quy trinh dung 100% theo so do Tuan 3 trong README:
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

// In danh sach cac ban ghi bi Validation loai bo khi load 1 loai du lieu.
// Yeu cau cua Y: phai thay ro no loai bo bot cai gi truoc khi hien thi tiep.
static void printRejections(const std::string& label, const std::vector<LoadRejection>& rej) {
    if (rej.empty()) {
        std::cout << "  [" << label << "] Khong co ban ghi nao bi loai.\n";
        return;
    }
    std::cout << "  [" << label << "] " << rej.size() << " ban ghi bi loai:\n";
    for (const auto& r : rej) {
        std::cout << "    - ID " << r.id << ": " << r.reason << "\n";
    }
}

int main() {
    std::cout << "=== KHOI TAO DO THI PBL2 ===\n";
    Graph graph;

    // Load toan bo du lieu truoc, dong thoi thu thap lai cac ban ghi
    // bi REJECT de lam bao cao validation ro rang (thay vi chi in rai rac).
    std::vector<LoadRejection> nodeRej, edgeRej, packetRej;

    std::cout << "\n--- Load Nodes ---\n";
    if (IOManager::loadNodes("data/nodes.txt", graph, &nodeRej)) {
        std::cout << "=> So luong node hop le: " << graph.V() << "\n";
    }

    std::cout << "\n--- Load Edges ---\n";
    if (IOManager::loadEdges("data/edges.txt", graph, &edgeRej)) {
        std::cout << "=> So luong edge hop le: " << graph.E() << "\n";
    }

    std::cout << "\n--- Load Packets ---\n";
    std::vector<Packet> packets;
    if (IOManager::loadPackets("data/packets.txt", packets, &packetRej)) {
        std::cout << "=> So luong packet hop le: " << packets.size() << "\n";
    }

    // Bao cao Validation: liet ke lai cho ro nhung gi da bi loai bo va vi sao,
    // truoc khi chay tiep cac buoc khac.
    std::cout << "\n=== KET QUA KIEM TRA DAU VAO (VALIDATION) ===\n";
    printRejections("Node", nodeRej);
    printRejections("Edge", edgeRej);
    printRejections("Packet", packetRej);

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
