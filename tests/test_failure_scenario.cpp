#include <iostream>
#include <cassert>
#include <vector>
#include "Graph.h"
#include "IOManager.h"
#include "Simulator.h"
#include "RoutingCost.h"
#include "Dijkstra.h"
#include "CostModel.h"
#include "Kruskal.h"

int main() {
    std::cout << "=== BAT DAU KIEM THU TASK B5.1 (FAILURE SCENARIO) ===\n";

    // 1. Kiểm tra đọc file kịch bản failure.txt
    Scenario scenario;
    bool loaded = IOManager::loadScenario("data/scenarios/failure.txt", scenario);
    assert(loaded && "Loi: Khong the load data/scenarios/failure.txt");
    assert(!scenario.events.empty() && "Loi: Kich ban khong co su kien nao");
    std::cout << "[PASS] Load thanh cong kịch bản " << scenario.name << " voi " << scenario.events.size() << " su kien.\n";

    // 2. Khởi tạo đồ thị mạng mẫu
    Graph g;
    assert(IOManager::loadNodes("data/nodes.txt", g));
    assert(IOManager::loadEdges("data/edges.txt", g));

    // Khởi tạo quy hoạch mạng (MST + 2 Backup links) giống kịch bản thực tế của main.cpp
    CostModel pricingModel;
    pricingModel.loadPricing("data/pricing.txt");
    auto costFn = [&pricingModel](const Edge& e) { return pricingModel.calculateCablingCost(e).total; };
    MSTResult mstResult = buildMST(g, costFn);
    selectBackupLinks(g, mstResult, 2, costFn);

    Packet pkt;
    pkt.id = 100;
    pkt.source = 0;
    pkt.destination = 3;
    pkt.sizeBytes = 500;
    pkt.type = PacketType::REAL_TIME;
    pkt.canFragment = false;

    // Đường ban đầu tốt nhất từ 0 -> 3: 0 -> 2 -> 3 (Edge 2 và 3)
    PathResult initPath = dijkstra(g, 0, 3, RoutingCost::makeWeightFn(pkt), true);
    assert(initPath.reachable && "Loi: Ban dau phai tim duoc duong");
    assert(initPath.nodes.size() == 3 && initPath.nodes[1] == 2 && "Loi: Ban dau phai di qua node 2 (0->2->3)");
    std::cout << "[PASS] Duong ban dau: 0 -> 2 -> 3.\n";

    // Case 1: Link ngoai path DOWN (Edge #4: 1-2)
    ScenarioEvent evOutside = {EventType::LINK_DOWN, 4, 0.0, 1.0};
    Simulator::applyEvent(g, evOutside);
    assert(!g.getEdge(4).isUp && "Loi: Edge 4 phai bi LINK_DOWN");
    PathResult pCase1 = dijkstra(g, 0, 3, RoutingCost::makeWeightFn(pkt), true);
    assert(pCase1.reachable && pCase1.nodes[1] == 2 && "Loi: Link ngoai path down thi route van la 0->2->3");
    std::cout << "[PASS] Case 1: Link ngoai path DOWN -> Route khong bi anh huong.\n";

    // Case 2: Link tren path DOWN (Edge #2: 0-2)
    ScenarioEvent evOnPath = {EventType::LINK_DOWN, 2, 0.0, 2.0};
    Simulator::applyEvent(g, evOnPath);
    assert(!g.getEdge(2).isUp && "Loi: Edge 2 phai bi LINK_DOWN");
    PathResult pCase2 = dijkstra(g, 0, 3, RoutingCost::makeWeightFn(pkt), true);
    assert(pCase2.reachable && "Loi: Phai reroute sang duong backup");
    assert(pCase2.nodes.size() == 3 && pCase2.nodes[1] == 1 && "Loi: Phai reroute sang duong 0 -> 1 -> 3");
    std::cout << "[PASS] Case 2: Link tren path DOWN -> Dijkstra tu dong reroute thanh cong sang 0 -> 1 -> 3.\n";

    // Case 3: LINK_UP phuc hoi
    ScenarioEvent evUp = {EventType::LINK_UP, 2, 0.0, 3.0};
    Simulator::applyEvent(g, evUp);
    assert(g.getEdge(2).isUp && "Loi: Edge 2 phai duoc bat lai");
    PathResult pCase3 = dijkstra(g, 0, 3, RoutingCost::makeWeightFn(pkt), true);
    assert(pCase3.nodes[1] == 2 && "Loi: Sau khi LINK_UP, phai quay lai duong toi uu 0 -> 2 -> 3");
    std::cout << "[PASS] Case 3: LINK_UP phuc hoi -> Tuyen toi uu duoc khoi phuc.\n";

    // Case 4: Multiple failures (dut ca Edge 2 va Edge 0)
    ScenarioEvent evMulti1 = {EventType::LINK_DOWN, 2, 0.0, 4.0};
    ScenarioEvent evMulti2 = {EventType::LINK_DOWN, 0, 0.0, 4.5};
    Simulator::applyEvent(g, evMulti1);
    Simulator::applyEvent(g, evMulti2);
    PathResult pCase4 = dijkstra(g, 0, 3, RoutingCost::makeWeightFn(pkt), true);
    assert(!pCase4.reachable && "Loi: Khi ca 2 duong ra deu dut thi reachable phai bang false");
    SimulationResult simRes = Simulator::sendPacket(pkt, pCase4, g);
    assert(!simRes.success && simRes.failReason == "NO_ROUTE_AVAILABLE" && "Loi: Gui packet phai bao DROP");
    std::cout << "[PASS] Case 4: Multiple failures -> Khong con duong, goi tin bi DROP dung ly do.\n";

    // Case 5: Bridge DOWN (Edge #6: 0-4, cau noi duy nhat toi Host 4)
    // Phuc hoi lai mang chung
    Simulator::applyEvent(g, {EventType::LINK_UP, 0, 0.0, 5.0});
    Simulator::applyEvent(g, {EventType::LINK_UP, 2, 0.0, 5.0});
    // Danh dut cau Edge 6 noi Host 4
    Simulator::applyEvent(g, {EventType::LINK_DOWN, 6, 0.0, 6.0});
    PathResult pBridge = dijkstra(g, 0, 4, RoutingCost::makeWeightFn(pkt), true);
    assert(!pBridge.reachable && "Loi: Bridge down thi Host 4 phai bi co lap hoan toan");
    std::cout << "[PASS] Case 5: Bridge DOWN -> Nut phu thuoc bi co lap hoan toan.\n";

    std::cout << "\n>>> CHUC MUNG! BAN DA HOAN THANH XUAT SAC TASK B5.1! <<<\n";
    return 0;
}
