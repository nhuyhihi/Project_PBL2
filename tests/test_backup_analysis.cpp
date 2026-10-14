#include <iostream>
#include <cassert>
#include <vector>
#include "Graph.h"
#include "IOManager.h"
#include "CostModel.h"
#include "Kruskal.h"
#include "Reliability.h"

int main() {
    std::cout << "=== BAT DAU KIEM THU TASK B5.2 (BACKUP ANALYSIS) ===\n";

    Graph g;
    assert(IOManager::loadNodes("data/nodes.txt", g));
    assert(IOManager::loadEdges("data/edges.txt", g));

    CostModel pricingModel;
    pricingModel.loadPricing("data/pricing.txt");
    auto costFn = [&pricingModel](const Edge& e) { return pricingModel.calculateCablingCost(e).total; };

    // 1. Xây MST và chọn Backup links
    MSTResult mst = buildMST(g, costFn);
    assert(mst.connected && "Loi: MST phai ket noi toan mang");
    BackupResult backup = selectBackupLinks(g, mst, 2, costFn);
    assert(backup.edgeIds.size() == 2 && "Loi: Phai chon duoc 2 cap du phong");

    std::cout << "-> Danh sach canh cay MST: ";
    for (int tId : mst.edgeIds) std::cout << tId << " ";
    std::cout << "\n-> Danh sach cap Backup da xay: ";
    for (int bId : backup.edgeIds) std::cout << bId << " ";
    std::cout << "\n";

    // 2. Test phan tich tim Backup cho Edge #1 (1-3) thuoc MST
    // Khi Edge #1 bi dut, phai tim duoc cap du phong thay the noi 2 nhanh
    BackupCandidate cand1 = Reliability::findBackupForTreeEdge(g, mst.edgeIds, 1);
    assert(cand1.backupEdgeId != -1 && "Loi: Edge #1 phai co cap du phong thay the");
    std::cout << "[PASS] Edge #1 (1-3) bi dut -> Tim thay cap du phong cuu nguy: Edge #" 
              << cand1.backupEdgeId << " (Ly do: " << cand1.reason << ")\n";

    // 3. Test phan tich tim Backup cho Edge #0 (0-1) thuoc MST
    BackupCandidate cand0 = Reliability::findBackupForTreeEdge(g, mst.edgeIds, 0);
    assert(cand0.backupEdgeId != -1 && "Loi: Edge #0 phai co cap du phong thay the");
    std::cout << "[PASS] Edge #0 (0-1) bi dut -> Tim thay cap du phong cuu nguy: Edge #" 
              << cand0.backupEdgeId << " (Ly do: " << cand0.reason << ")\n";

    // 4. Test phan tich cho Canh cau (Critical Bridge): Edge #6 (0-4) noi toi Host 4
    // Host 4 chi co 1 cong, khong co duong thay the nao khac -> phai bao khong co backup
    BackupCandidate candBridge = Reliability::findBackupForTreeEdge(g, mst.edgeIds, 6);
    assert(candBridge.backupEdgeId == -1 && "Loi: Edge #6 la canh cau duy nhat, khong duoc co backup");
    std::cout << "[PASS] Canh cau duy nhat Edge #6 bi dut -> Bao dung khong co backup (Ly do: " 
              << candBridge.reason << ")\n";

    // 5. Test findAllBackupsForTreeEdge
    std::vector<BackupCandidate> allBackups = Reliability::findAllBackupsForTreeEdge(g, mst.edgeIds, 1);
    assert(!allBackups.empty() && "Loi: Phai liet ke duoc it nhat 1 ung vien cho Edge #1");
    std::cout << "[PASS] findAllBackupsForTreeEdge tim duoc " << allBackups.size() << " ung vien cho Edge #1.\n";

    std::cout << "\n>>> CHUC MUNG! BAN DA PASS 100% YEU CAU CUA TASK B5.2! <<<\n";
    return 0;
}
