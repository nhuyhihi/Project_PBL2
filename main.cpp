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

    std::cout << "\n=== TIEN HANH MO PHONG TRUYEN GOI TIN CO BAN (SIMULATOR) ===\n";
    // 5. Mô phỏng gửi gói tin với Snapshot và RoutingCost động (Task Tuần 4 của Quyến & Ý)
    for (const Packet& pkt : packets) {
        // Chụp snapshot trạng thái mạng tại thời điểm bắt đầu định tuyến (Task B4.1, B4.2)
        NetworkSnapshot snap = Simulator::snapshotNetworkState(graph);

        // Định tuyến theo chi phí động (RoutingCost) theo loại QoS (Task A4.2, A4.4)
        PathResult path = dijkstra(graph, pkt.source, pkt.destination, pkt, /*onlyBuilt=*/true);
        
        std::cout << "Packet #" << pkt.id << " (" << pkt.source << " -> " << pkt.destination 
                  << ", " << pkt.sizeBytes << " bytes, Type: " << (pkt.type == PacketType::REAL_TIME ? "REAL_TIME" : (pkt.type == PacketType::BULK_DATA ? "BULK_DATA" : "CONTROL")) << "):\n";
        
        if (path.reachable) {
            std::cout << "  -> Route Dijkstra chon: ";
            for (size_t i = 0; i < path.nodes.size(); ++i) {
                std::cout << path.nodes[i] << (i + 1 < path.nodes.size() ? " -> " : "");
            }
            std::cout << "\n";
        }

        // Mô phỏng gửi gói tin (Task B4.5, B4.6)
        SimulationResult res = Simulator::sendPacket(pkt, path, graph);
        
        if (res.success) {
            std::cout << "  => SUCCESS! Thoi gian truyen: " << std::fixed << std::setprecision(6) << res.totalDelay 
                      << " s (Phan manh: " << res.fragmentCount << " manh)\n";
        } else {
            std::cout << "  => DROP! Ly do: " << res.failReason << "\n";
        }
    }

    std::cout << "\n=== DEMO KICH BAN SU KIEN DONG (SCENARIO SIMULATION) ===\n";
    // 6. Nạp kịch bản tắc nghẽn (data/scenarios/congestion.txt) và áp dụng sự kiện (Task B4.3, B4.4, B4.7)
    Scenario congestionScenario;
    if (IOManager::loadScenario("data/scenarios/congestion.txt", congestionScenario)) {
        std::cout << "-> Da nap kich ban: " << congestionScenario.name 
                  << " voi " << congestionScenario.events.size() << " su kien.\n";

        // Thực thi từng sự kiện trong kịch bản để thấy tác động động
        for (const auto& ev : congestionScenario.events) {
            std::cout << "\n[Time = " << ev.time << "s] Ap dung su kien vao Edge #" << ev.targetId 
                      << " (Load moi: " << ev.value << "):\n";
            
            Simulator::applyEvent(graph, ev);
            
            // Thử định tuyến lại cho gói tin REAL_TIME đầu tiên để xem route có đổi không
            if (!packets.empty()) {
                const Packet& testPkt = packets[0];
                PathResult newPath = dijkstra(graph, testPkt.source, testPkt.destination, testPkt, /*onlyBuilt=*/true);
                
                std::cout << "  -> Route moi cho Packet #" << testPkt.id << " (" << testPkt.source << " -> " << testPkt.destination << "): ";
                for (size_t i = 0; i < newPath.nodes.size(); ++i) {
                    std::cout << newPath.nodes[i] << (i + 1 < newPath.nodes.size() ? " -> " : "");
                }
                std::cout << " (Cost: " << std::fixed << std::setprecision(6) << newPath.totalCost << ")\n";
            }
        }
    }

    std::cout << "\n=== KET THUC CHUONG TRINH ===\n";
    return 0;
}
