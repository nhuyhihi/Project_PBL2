// ============================================================
// Test tuan 5 (Quyen): Connectivity - Task A5.2 (lien thong) va A5.5 (chi so chiu loi).
// Chay tu thu muc goc repo:
//   g++ -std=c++17 -Wall -Wextra -Iinclude src/Graph.cpp src/Connectivity.cpp tests/test_connectivity.cpp -o test_connectivity
//   ./test_connectivity
// ============================================================
#include <cstdio>
#include <cmath>

#include "Graph.h"
#include "Connectivity.h"

static int g_pass = 0, g_fail = 0;
#define CHECK(cond)                                                          \
    do {                                                                     \
        if (cond) { ++g_pass; }                                              \
        else { ++g_fail; std::printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); } \
    } while (0)

static bool near(double a, double b) { return std::fabs(a - b) < 1e-9; }

static Node makeNode(int id) {
    Node n;
    n.id = id; n.type = NodeType::ROUTER; n.totalPorts = 8; n.usedPorts = 0;
    n.processingTime = 0.0;
    return n;
}

static Edge makeEdge(int id, int u, int v, bool built = true) {
    Edge e;
    e.id = id; e.u = u; e.v = v; e.mediaType = MediaType::FIBER;
    e.length = 100; e.bandwidthMbps = 1000; e.maxSegmentLength = 1e9;
    e.mtu = 1500; e.currentLoad = 0.0; e.queueDelay = 0.0;
    e.isUp = true; e.isBuilt = built; e.isBackup = false;
    return e;
}

// ------------------------------------------------------------
// A5.2: isConnected bang BFS, bo qua day DOWN
// Mang: 0 - 1 - 2 (da lap),  3 chi noi voi 0 bang day CHUA lap (edge 2)
// ------------------------------------------------------------
static void testIsConnected() {
    std::printf("[A5.2: isConnected]\n");
    Graph g;
    for (int i = 0; i < 4; ++i) g.addNode(makeNode(i));
    g.addEdge(makeEdge(0, 0, 1));
    g.addEdge(makeEdge(1, 1, 2));
    g.addEdge(makeEdge(2, 0, 3, /*built=*/false));

    // Connected -> true
    CHECK(Connectivity::isConnected(g, 0, 2));
    CHECK(Connectivity::isConnected(g, 2, 0));           // vo huong
    CHECK(Connectivity::isConnected(g, 1, 1));           // cung 1 node

    // Disconnected -> false (3 chi noi bang day chua lap)
    CHECK(!Connectivity::isConnected(g, 0, 3, /*onlyBuilt=*/true));
    CHECK(Connectivity::isConnected(g, 0, 3, /*onlyBuilt=*/false));

    // Node khong ton tai -> false
    CHECK(!Connectivity::isConnected(g, 0, 99));

    // Bo qua link DOWN
    g.getEdge(1).isUp = false;
    CHECK(!Connectivity::isConnected(g, 0, 2));
    CHECK(Connectivity::isConnected(g, 0, 1));
    g.getEdge(1).isUp = true;

    // reachableFrom + thanh phan lien thong
    auto reach = Connectivity::reachableFrom(g, 0);
    CHECK(reach.size() == 3 && reach[0] == 0 && reach[1] == 1 && reach[2] == 2);
    CHECK(Connectivity::connectedComponents(g).size() == 2);     // {0,1,2} va {3}
    CHECK(!Connectivity::isNetworkConnected(g));
    CHECK(Connectivity::isNetworkConnected(g, /*onlyBuilt=*/false));
}

// ------------------------------------------------------------
// A5.5: ty le cap node con lien lac = so cap lien thong / tong so cap
// ------------------------------------------------------------
static void testPairConnectivity() {
    std::printf("[A5.5: pairConnectivity]\n");
    Graph g;
    for (int i = 0; i < 4; ++i) g.addNode(makeNode(i));
    g.addEdge(makeEdge(0, 0, 1));
    g.addEdge(makeEdge(1, 1, 2));

    // Thanh phan {0,1,2} va {3}: 3 cap lien lac / 6 cap = 50%
    auto p = Connectivity::pairConnectivity(g);
    CHECK(p.totalPairs == 6);
    CHECK(p.connectedPairs == 3);
    CHECK(near(p.ratio, 0.5));

    // Thu dut edge 1 ma KHONG sua graph: {0,1}, {2}, {3} -> 1/6
    auto q = Connectivity::pairConnectivityWith(g, {0, 1}, {1});
    CHECK(q.connectedPairs == 1);
    CHECK(g.getEdge(1).isUp);                 // graph khong bi thay doi

    // Mang 1 node -> ty le 100%
    Graph one;
    one.addNode(makeNode(0));
    CHECK(near(Connectivity::pairConnectivity(one).ratio, 1.0));
}

// ------------------------------------------------------------
// A5.5: so sanh MST vs MST + backup
// Vong 4 node: cay khung 0-1, 1-2, 2-3 ; backup 3-0
// ------------------------------------------------------------
static void testCompareMstVsBackup() {
    std::printf("[A5.5: MST vs MST + backup]\n");
    Graph g;
    for (int i = 0; i < 4; ++i) g.addNode(makeNode(i));
    g.addEdge(makeEdge(0, 0, 1));
    g.addEdge(makeEdge(1, 1, 2));
    g.addEdge(makeEdge(2, 2, 3));
    g.addEdge(makeEdge(3, 3, 0));   // backup

    auto rep = Connectivity::compareSingleFailures(g, {0, 1, 2}, {3});
    CHECK(rep.rows.size() == 3);

    // Chi MST: dut 0-1 -> {0},{1,2,3} = 3/6 ; dut 1-2 -> {0,1},{2,3} = 2/6 ; dut 2-3 -> 3/6
    CHECK(near(rep.rows[0].ratioMstOnly, 3.0 / 6));
    CHECK(near(rep.rows[1].ratioMstOnly, 2.0 / 6));
    CHECK(near(rep.rows[2].ratioMstOnly, 3.0 / 6));
    CHECK(near(rep.avgMstOnly, (3.0 + 2.0 + 3.0) / 18));
    CHECK(rep.survivedMstOnly == 0);

    // Co backup: dut 1 day bat ky van lien thong hoan toan
    for (const auto& r : rep.rows) CHECK(near(r.ratioWithBackup, 1.0));
    CHECK(near(rep.avgWithBackup, 1.0));
    CHECK(rep.survivedWithBackup == 3);

    // Backup giup mang chiu loi tot hon
    CHECK(rep.avgWithBackup > rep.avgMstOnly);

    Connectivity::printFaultToleranceReport(rep);
}

int main() {
    testIsConnected();
    testPairConnectivity();
    testCompareMstVsBackup();

    std::printf("\nKet qua: %d dat, %d loi\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
