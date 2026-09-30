#include <iostream>
#include <vector>
#include "Graph.h"
#include "IOManager.h"
#include "Kruskal.h"
#include "CostModel.h"
#include "Reporter.h"
#include "Simulator.h"
#include <iomanip>

int main() {
    std::cout << "=== KHOI TAO DO THI ===\n";
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

    std::cout << "=== NẠP BẢNG GIÁ (PRICING) ===\n";
    CostModel pricingModel;
    pricingModel.loadPricing("data/pricing.txt");

    std::cout << "\n=== TIEN HANH QUY HOACH MANG LUI (KRUSKAL) ===\n";
    // 1. Chạy thuật toán Kruskal tìm cây khung nhỏ nhất (MST)
    auto costFn = [&pricingModel](const Edge& e) { return pricingModel.calculateCablingCost(e).total; };
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
    // 3. In Báo cáo Chi phí lắp đặt 
    Reporter::printMSTReport(graph, pricingModel);

    // 4. In Bảng định tuyến (Routing Table) toàn mạng
    Reporter::printFullRoutingTable(graph);

    std::cout << "\n=== TIEN HANH MO PHONG TRUYEN GOI TIN (SIMULATOR) ===\n";
    // 5. Mô phỏng gửi gói tin bằng Module Simulator (Task của Ý)
    for (const Packet& pkt : packets) {
        // Tạm thời dùng lengthWeight vì Quyến chưa xong Task A4.3 (Dynamic Cost)
        PathResult path = dijkstra(graph, pkt.source, pkt.destination, lengthWeight, /*onlyBuilt=*/true);
        
        std::cout << "Packet #" << pkt.id << " (" << pkt.source << " -> " << pkt.destination << ", " << pkt.sizeBytes << " bytes):\n";
        
        // Gọi thẳng hàm sendPacket của Ý
        SimulationResult res = Simulator::sendPacket(pkt, path, graph);
        
        if (res.success) {
            std::cout << "  => SUCCESS! Thoi gian truyen: " << std::fixed << std::setprecision(6) << res.totalDelay 
                      << " s (Phan manh: " << res.fragmentCount << " manh)\n";
        } else {
            std::cout << "  => DROP! Ly do: " << res.failReason << "\n";
        }
    }

    std::cout << "\n=== KET THUC CHUONG TRINH ===\n";
    return 0;
}
