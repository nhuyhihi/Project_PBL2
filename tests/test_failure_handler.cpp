// ============================================================
// Test tuan 5 (Quyen): FailureHandler - Task A5.1, A5.3, A5.4 + test T14, T15, T16.
// Chay tu thu muc goc repo:
//   g++ -std=c++17 -Wall -Wextra -Iinclude src/Graph.cpp src/Dijkstra.cpp src/DelayModel.cpp src/RoutingCost.cpp src/Connectivity.cpp src/FailureHandler.cpp tests/test_failure_handler.cpp -o test_failure_handler
//   ./test_failure_handler
// ============================================================
#include <cstdio>
#include <string>
#include <algorithm>

#include "Graph.h"
#include "Dijkstra.h"
#include "Config.h"
#include "Connectivity.h"
#include "FailureHandler.h"

using namespace FailureHandler;

static int g_pass = 0, g_fail = 0;
#define CHECK(cond)                                                          \
    do {                                                                     \
        if (cond) { ++g_pass; }                                              \
        else { ++g_fail; std::printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); } \
    } while (0)

static Node makeNode(int id) {
    Node n;
    n.id = id; n.type = NodeType::ROUTER; n.totalPorts = 8; n.usedPorts = 0;
    n.processingTime = 0.0;
    return n;
}

static Edge makeEdge(int id, int u, int v, double length, bool built = true, bool backup = false) {
    Edge e;
    e.id = id; e.u = u; e.v = v; e.mediaType = MediaType::FIBER;
    e.length = length; e.bandwidthMbps = 1000; e.maxSegmentLength = 1e9;
    e.mtu = 1500; e.currentLoad = 0.0; e.queueDelay = 0.0;
    e.isUp = true; e.isBuilt = built; e.isBackup = backup;
    return e;
}

static bool contains(const std::vector<int>& v, int x) {
    return std::find(v.begin(), v.end(), x) != v.end();
}

static bool contains(const std::string& s, const std::string& sub) {
    return s.find(sub) != std::string::npos;
}

// ------------------------------------------------------------
// Mang mau (giong y tuong data/scenarios/failure.txt):
//
//        (e0, 50)      (e1, 50)
//    0 ----------- 2 ----------- 3        duong chinh 0 -> 2 -> 3
//    |                           |
//    | (e2, 100)       (e3, 100) |        e3 la cap DU PHONG (backup)
//    1 --------------------------+        duong vong 0 -> 1 -> 3
//    |
//    0 --- 4  (e5, 10)  Host 4 chi noi bang 1 day (cau / bridge)
//
// Cay khung (MST): e0, e1, e2, e5.   Backup: e3.
// ------------------------------------------------------------
static Graph buildNetwork(bool withBackup) {
    Graph g;
    for (int i = 0; i < 5; ++i) g.addNode(makeNode(i));
    g.addEdge(makeEdge(0, 0, 2, 50));
    g.addEdge(makeEdge(1, 2, 3, 50));
    g.addEdge(makeEdge(2, 0, 1, 100));
    g.addEdge(makeEdge(3, 1, 3, 100, /*built=*/withBackup, /*backup=*/true));
    g.addEdge(makeEdge(5, 0, 4, 10));
    return g;
}

// ------------------------------------------------------------
// A5.1: setLinkState
// ------------------------------------------------------------
static void testSetLinkState() {
    std::printf("[A5.1: setLinkState]\n");
    Graph g = buildNetwork(true);

    CHECK(setLinkState(g, 1, false));
    CHECK(g.getEdge(1).isUp == false);           // Link DOWN -> isUp = false
    CHECK(setLinkState(g, 1, true));
    CHECK(g.getEdge(1).isUp == true);            // Link UP -> isUp = true

    CHECK(!setLinkState(g, 99, false));          // edge khong ton tai

    CHECK(applyLinkEvent(g, EventType::LINK_DOWN, 0));
    CHECK(!g.getEdge(0).isUp);
    CHECK(applyLinkEvent(g, EventType::LINK_UP, 0));
    CHECK(g.getEdge(0).isUp);
    CHECK(!applyLinkEvent(g, EventType::LOAD_UPDATE, 0));   // khong phai su kien LINK
    CHECK(g.getEdge(0).isUp);
}

// ------------------------------------------------------------
// isPathValid
// ------------------------------------------------------------
static void testIsPathValid() {
    std::printf("[A5.3: isPathValid]\n");
    Graph g = buildNetwork(true);
    PathResult p = dijkstra(g, 0, 3, lengthWeight, true);
    CHECK(p.reachable && p.edges.size() == 2);   // 0 -> 2 -> 3

    CHECK(isPathValid(g, p));
    g.getEdge(1).isUp = false;
    std::vector<int> broken;
    CHECK(!isPathValid(g, p, true, &broken));
    CHECK(broken.size() == 1 && broken[0] == 1);

    PathResult none;                              // reachable = false
    CHECK(!isPathValid(g, none));
}

// ------------------------------------------------------------
// T14: dut day TREN duong di -> reroute
// T16: duong moi di qua cap du phong (backup)
// ------------------------------------------------------------
static void testRerouteOnPath() {
    std::printf("[T14 + T16: dut tren duong di -> reroute qua backup]\n");
    Graph g = buildNetwork(true);
    PathResult oldPath = dijkstra(g, 0, 3, lengthWeight, true);
    CHECK(oldPath.nodes == std::vector<int>({0, 2, 3}));

    FailureReport rep = handleFailureEvent(g, 1, /*up=*/false, oldPath, 0, 3, lengthWeight);
    CHECK(rep.applied);
    CHECK(rep.linkUpBefore && !rep.linkUpAfter);
    CHECK(rep.reroute.action == RerouteAction::REROUTED);
    CHECK(rep.reroute.brokenEdges == std::vector<int>({1}));
    CHECK(rep.reroute.path.nodes == std::vector<int>({0, 1, 3}));
    CHECK(contains(rep.reroute.path.edges, 3));  // T16: di qua cap du phong e3
    CHECK(rep.sourceDestConnected);
    CHECK(rep.networkConnected);

    printFailureReport(rep);
}

// ------------------------------------------------------------
// Dut day NGOAI duong di -> khong reroute
// ------------------------------------------------------------
static void testFailureOutsidePath() {
    std::printf("[A5.3: dut ngoai duong di -> giu nguyen]\n");
    Graph g = buildNetwork(true);
    PathResult oldPath = dijkstra(g, 0, 3, lengthWeight, true);

    FailureReport rep = handleFailureEvent(g, 2, false, oldPath, 0, 3, lengthWeight);
    CHECK(rep.reroute.action == RerouteAction::KEEP);
    CHECK(rep.reroute.path.nodes == oldPath.nodes);
    CHECK(rep.reroute.brokenEdges.empty());
    CHECK(rep.networkConnected);                 // 1 van con duong qua e3 -> 3 -> 2 -> 0
}

// ------------------------------------------------------------
// T15: dut du nhieu day -> khong con duong -> DROP + ly do
// ------------------------------------------------------------
static void testNoRouteDrop() {
    std::printf("[T15: mat lien thong -> DROP]\n");
    Graph g = buildNetwork(true);
    PathResult oldPath = dijkstra(g, 0, 3, lengthWeight, true);

    setLinkState(g, 3, false);                   // dut backup truoc (ngoai duong di)
    FailureReport rep = handleFailureEvent(g, 1, false, oldPath, 0, 3, lengthWeight);

    CHECK(rep.reroute.action == RerouteAction::DROPPED);
    CHECK(!rep.reroute.path.reachable);
    CHECK(contains(rep.reroute.reason, "NO_ROUTE"));
    CHECK(contains(rep.reroute.reason, "tach roi"));
    CHECK(!rep.sourceDestConnected);
    CHECK(!rep.networkConnected);
    CHECK(rep.componentCount == 2);              // {0,1,2,4} va {3}

    printFailureReport(rep);
}

// ------------------------------------------------------------
// Kich ban bat buoc: MST thuan (khong backup) -> dut day chinh -> mat lien lac
// ------------------------------------------------------------
static void testMstOnlyLosesConnection() {
    std::printf("[MST thuan: dut day chinh -> mat lien lac]\n");
    Graph g = buildNetwork(/*withBackup=*/false);
    PathResult oldPath = dijkstra(g, 0, 3, lengthWeight, true);
    CHECK(oldPath.reachable);

    FailureReport rep = handleFailureEvent(g, 1, false, oldPath, 0, 3, lengthWeight);
    CHECK(rep.reroute.action == RerouteAction::DROPPED);
    CHECK(!rep.sourceDestConnected);
}

// ------------------------------------------------------------
// Dut cau (bridge) noi Host 4 -> Host 4 bi co lap
// ------------------------------------------------------------
static void testBridgeDown() {
    std::printf("[Dut cau: Host 4 bi co lap]\n");
    Graph g = buildNetwork(true);
    PathResult oldPath = dijkstra(g, 0, 4, lengthWeight, true);

    FailureReport rep = handleFailureEvent(g, 5, false, oldPath, 0, 4, lengthWeight);
    CHECK(rep.reroute.action == RerouteAction::DROPPED);
    CHECK(rep.componentCount == 2);
    // 5 node, tach {0,1,2,3} va {4}: 6 cap lien lac / 10 cap
    CHECK(rep.pairs.totalPairs == 10 && rep.pairs.connectedPairs == 6);
}

// ------------------------------------------------------------
// Van lien thong nhung khong co duong hop le (vd vuot MTU) -> ly do khac
// ------------------------------------------------------------
static void testDropReasonWhenConnected() {
    std::printf("[DROP vi khong co duong hop le du van lien thong]\n");
    Graph g = buildNetwork(true);
    PathResult oldPath = dijkstra(g, 0, 3, lengthWeight, true);

    // Gia lap: e3 co chi phi vo cuc voi goi tin nay (vd vuot MTU, khong duoc phan manh)
    WeightFn w = [](const Edge& e) { return e.id == 3 ? Config::INF : e.length; };
    FailureReport rep = handleFailureEvent(g, 1, false, oldPath, 0, 3, w);

    CHECK(rep.reroute.action == RerouteAction::DROPPED);
    CHECK(rep.sourceDestConnected);              // ve vat ly van con day
    CHECK(contains(rep.reroute.reason, "MTU"));
}

// ------------------------------------------------------------
// LINK_UP phuc hoi: dang DROP -> co duong lai
// ------------------------------------------------------------
static void testRecoveryAfterDrop() {
    std::printf("[LINK_UP phuc hoi sau DROP]\n");
    Graph g = buildNetwork(true);
    setLinkState(g, 1, false);
    setLinkState(g, 3, false);
    PathResult dropped = dijkstra(g, 0, 3, lengthWeight, true);
    CHECK(!dropped.reachable);

    FailureReport rep = handleFailureEvent(g, 3, /*up=*/true, dropped, 0, 3, lengthWeight);
    CHECK(rep.linkUpAfter);
    CHECK(rep.reroute.action == RerouteAction::REROUTED);
    CHECK(rep.reroute.path.nodes == std::vector<int>({0, 1, 3}));
    CHECK(rep.sourceDestConnected);
}

// ------------------------------------------------------------
// Edge khong ton tai -> bo qua, khong lam gi
// ------------------------------------------------------------
static void testUnknownEdge() {
    std::printf("[Edge khong ton tai]\n");
    Graph g = buildNetwork(true);
    PathResult oldPath = dijkstra(g, 0, 3, lengthWeight, true);

    FailureReport rep = handleFailureEvent(g, 99, false, oldPath, 0, 3, lengthWeight);
    CHECK(!rep.applied);
    CHECK(rep.reroute.action == RerouteAction::KEEP);
    CHECK(rep.reroute.path.nodes == oldPath.nodes);
}

int main() {
    testSetLinkState();
    testIsPathValid();
    testRerouteOnPath();
    testFailureOutsidePath();
    testNoRouteDrop();
    testMstOnlyLosesConnection();
    testBridgeDown();
    testDropReasonWhenConnected();
    testRecoveryAfterDrop();
    testUnknownEdge();

    std::printf("\nKet qua: %d dat, %d loi\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
