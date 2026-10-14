#include <iostream>
#include <cassert>
#include <sstream>
#include "Graph.h"
#include "IOManager.h"
#include "CostModel.h"
#include "Kruskal.h"
#include "Dijkstra.h"
#include "RoutingCost.h"
#include "Reporter.h"
#include "Simulator.h"

int main() {
    std::cout << "=== BAT DAU KIEM THU TASK B5.3 (FAILURE REPORT) ===\n";

    Graph g;
    assert(IOManager::loadNodes("data/nodes.txt", g));
    assert(IOManager::loadEdges("data/edges.txt", g));

    CostModel pricingModel;
    pricingModel.loadPricing("data/pricing.txt");
    auto costFn = [&pricingModel](const Edge& e) { return pricingModel.calculateCablingCost(e).total; };

    MSTResult mst = buildMST(g, costFn);
    selectBackupLinks(g, mst, 2, costFn);

    Packet pkt;
    pkt.id = 1;
    pkt.source = 0;
    pkt.destination = 3;
    pkt.type = PacketType::REAL_TIME;
    pkt.sizeBytes = 500;

    // 1. Đường ban đầu khi mạng bình thường
    PathResult oldPath = dijkstra(g, pkt.source, pkt.destination, RoutingCost::makeWeightFn(pkt), true);
    assert(oldPath.reachable);

    // 2. Mô phỏng đứt Edge #2 (0-2) trên đường đi -> Reroute
    Simulator::applyEvent(g, {EventType::LINK_DOWN, 2, 0.0, 1.0});
    PathResult newPath = dijkstra(g, pkt.source, pkt.destination, RoutingCost::makeWeightFn(pkt), true);
    assert(newPath.reachable);
    assert(newPath.nodes[1] == 1); // Reroute sang 0 -> 1 -> 3

    std::cout << "\n[TEST CASE 1: REROUTE THANH CONG]\n";
    Reporter::printFailureRecoveryReport(g, pkt, oldPath, 2, newPath, "Duong chinh bi dut -> Kich hoat nhanh du phong qua Router 1");

    // 3. Mô phỏng đứt thêm Edge #0 (0-1) -> Mất hết đường ra (DROP)
    Simulator::applyEvent(g, {EventType::LINK_DOWN, 0, 0.0, 2.0});
    PathResult dropPath = dijkstra(g, pkt.source, pkt.destination, RoutingCost::makeWeightFn(pkt), true);
    assert(!dropPath.reachable);

    std::cout << "\n[TEST CASE 2: MAT LIEN LAC (DROP)]\n";
    Reporter::printFailureRecoveryReport(g, pkt, newPath, 0, dropPath, "Ca 2 cong ra deu bi dut -> Mua bao lam co lap node nguon");

    std::cout << "\n>>> CHUC MUNG! BAN DA PASS 100% YEU CAU CUA TASK B5.3! <<<\n";
    return 0;
}
