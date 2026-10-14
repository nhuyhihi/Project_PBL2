// ============================================================
// RoutingCost 
// Chay tu thu muc goc repo:
//   g++ -std=c++17 -Wall -Wextra -Iinclude src/Graph.cpp src/Dijkstra.cpp src/DelayModel.cpp src/RoutingCost.cpp tests/test_routing_cost.cpp -o test_routing_cost
//   ./test_routing_cost
// ============================================================
#include <cstdio>
#include <cmath>

#include "Graph.h"
#include "Dijkstra.h"
#include "RoutingCost.h"
#include "Config.h"

static int g_pass = 0, g_fail = 0;
#define CHECK(cond)                                                          \
    do {                                                                     \
        if (cond) { ++g_pass; }                                              \
        else { ++g_fail; std::printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); } \
    } while (0)

static Node makeNode(int id, int ports) {
    Node n;
    n.id = id; n.type = NodeType::ROUTER; n.totalPorts = ports; n.usedPorts = 0;
    n.processingTime = 0.0;
    return n;
}

static Edge makeEdge(int id, int u, int v, double length, double bw, double load, double queueDelay) {
    Edge e;
    e.id = id; e.u = u; e.v = v; e.mediaType = MediaType::FIBER;
    e.length = length; e.bandwidthMbps = bw; e.maxSegmentLength = 1e9;
    e.mtu = 1500; e.currentLoad = load; e.queueDelay = queueDelay;
    e.isUp = true; e.isBuilt = true; e.isBackup = false;
    return e;
}

static Packet makePacket(int id, int src, int dst, int size, PacketType type) {
    Packet p;
    p.id = id; p.source = src; p.destination = dst; p.sizeBytes = size;
    p.type = type; p.protocol = Protocol::UDP; p.canFragment = true; p.newConnection = false;
    return p;
}

// ------------------------------------------------------------
// CongestionPenalty phai tang theo load, va load=1.0 khong vuot
// gioi han cua mo hinh (10.00).
// ------------------------------------------------------------
static void testCongestionPenalty() {
    std::printf("[A4.1: congestionPenalty]\n");
    CHECK(RoutingCost::congestionPenalty(0.0) == 1.0);
    CHECK(RoutingCost::congestionPenalty(0.80) == 1.0);
    CHECK(RoutingCost::congestionPenalty(0.85) == 1.56);
    CHECK(RoutingCost::congestionPenalty(0.90) == 3.25);
    CHECK(RoutingCost::congestionPenalty(0.95) == 6.06);
    CHECK(RoutingCost::congestionPenalty(1.0) == 10.00);
    // Tang dan, khong duoc giam o bat ky buoc nao
    CHECK(RoutingCost::congestionPenalty(0.80) <= RoutingCost::congestionPenalty(0.85));
    CHECK(RoutingCost::congestionPenalty(0.85) <= RoutingCost::congestionPenalty(0.90));
    CHECK(RoutingCost::congestionPenalty(0.90) <= RoutingCost::congestionPenalty(0.95));
    CHECK(RoutingCost::congestionPenalty(0.95) <= RoutingCost::congestionPenalty(1.0));
}

// ------------------------------------------------------------
// CalculateRoutingCost phai tra INF khi day dut hoac goi DF vuot MTU,
// khong duoc NaN.
// ------------------------------------------------------------
static void testRoutingCostEdgeCases() {
    std::printf("[A4.3: calculateRoutingCost - truong hop bien]\n");
    Edge down = makeEdge(0, 0, 1, 100, 1000, 0.0, 0.001);
    down.isUp = false;
    Packet pkt = makePacket(0, 0, 1, 500, PacketType::CONTROL);
    CHECK(RoutingCost::calculateRoutingCost(pkt, down) == Config::INF);

    Edge smallMtu = makeEdge(1, 0, 1, 100, 1000, 0.0, 0.001);
    smallMtu.mtu = 100;
    Packet dfPkt = makePacket(1, 0, 1, 5000, PacketType::BULK_DATA);
    dfPkt.canFragment = false;
    CHECK(RoutingCost::calculateRoutingCost(dfPkt, smallMtu) == Config::INF);

    Edge ok = makeEdge(2, 0, 1, 100, 1000, 0.5, 0.001);
    Packet okPkt = makePacket(2, 0, 1, 500, PacketType::CONTROL);
    double cost = RoutingCost::calculateRoutingCost(okPkt, ok);
    CHECK(!std::isnan(cost));
    CHECK(cost > 0.0 && cost < Config::INF);
}

// ------------------------------------------------------------
// REAL_TIME phai phat nang hon BULK_DATA tren CUNG 1 day dang tai
// cao (vi wQueue cua REAL_TIME lon hon) - chung minh 2 QoS "nhin" cung 1
// day khac nhau.
// ------------------------------------------------------------
static void testQoSWeightsDiffer() {
    std::printf("[A4.2: QoS khac nhau tren cung 1 day]\n");
    Edge congested = makeEdge(0, 0, 1, 100, 1000, 0.95, 1.0); // queueDelay base = 1.0s de de thay chenh lech
    Packet realtime = makePacket(0, 0, 1, 500, PacketType::REAL_TIME);
    Packet bulk      = makePacket(1, 0, 1, 500, PacketType::BULK_DATA);

    double costRT   = RoutingCost::calculateRoutingCost(realtime, congested);
    double costBulk = RoutingCost::calculateRoutingCost(bulk, congested);

    // REAL_TIME co wQueue=4.0 > BULK_DATA wQueue=2.0 (xem Config.h), nen
    // tren day dang nghen nang, REAL_TIME phai bi phat cost CAO HON BULK_DATA
    // (REAL_TIME "so" nghen hon).
    CHECK(costRT > costBulk);
}

// ------------------------------------------------------------
// Tich hop voi Dijkstra - co 2 duong tu 0 den 2 (mot ngan nhung
// dang nghen, mot dai hon nhung thong thoang). Goi REAL_TIME phai ne
// duong nghen, con BULK_DATA (it nhay cam hon) co the van chon duong
// ngan dang nghen do trong so wTrans cao hon danh gia duoc loi ich bang
// thong lon hon o day do.
// ------------------------------------------------------------
static void testDijkstraRouteChangesWithQoS() {
    std::printf("[A4.4: Dijkstra doi duong theo QoS]\n");
    Graph g;
    g.addNode(makeNode(0, 4));
    g.addNode(makeNode(1, 4));
    g.addNode(makeNode(2, 4));

    // Duong ngan truc tiep 0->2 nhung dang nghen rat nang (load=0.98)
    g.addEdge(makeEdge(0, 0, 2, 100, 1000, 0.98, 2.0));
    // Duong vong 0->1->2 dai hon ve vat ly nhung thong thoang (load=0.1)
    g.addEdge(makeEdge(1, 0, 1, 150, 1000, 0.1, 0.01));
    g.addEdge(makeEdge(2, 1, 2, 150, 1000, 0.1, 0.01));

    Packet realtime = makePacket(0, 0, 2, 500, PacketType::REAL_TIME);
    PathResult pathRT = dijkstra(g, 0, 2, RoutingCost::makeWeightFn(realtime), /*onlyBuilt=*/true);

    CHECK(pathRT.reachable);
    // REAL_TIME rat nhay cam voi nghen (wQueue=4.0) nen phai ne duong
    // nghen nang va di duong vong 0 -> 1 -> 2 (2 chang) thay vi di thang
    // 0 -> 2 (1 chang) dang bi nghen 0.98.
    CHECK(pathRT.hopCount == 2);
    CHECK(pathRT.nodes.size() == 3);
    CHECK(pathRT.nodes[0] == 0 && pathRT.nodes[1] == 1 && pathRT.nodes[2] == 2);
}

int main() {
    testCongestionPenalty();
    testRoutingCostEdgeCases();
    testQoSWeightsDiffer();
    testDijkstraRouteChangesWithQoS();

    std::printf("\nKet qua: %d dat, %d loi\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
