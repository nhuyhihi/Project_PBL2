// ============================================================
// Test tuan 3 (Quyen): PathResult + Dijkstra (A3.1 -> A3.5).
// Chay tu thu muc goc repo (can data/nodes.txt, data/edges.txt cho test that).
//   g++ -std=c++17 -Wall -Wextra -Iinclude src/Graph.cpp src/IOManager.cpp src/Validation.cpp src/Dijkstra.cpp tests/test_dijkstra.cpp -o test_dijkstra
//   ./test_dijkstra
// ============================================================
#include <cmath>
#include <cstdio>
#include <vector>

#include "Graph.h"
#include "Dijkstra.h"
#include "IOManager.h"

static int g_pass = 0, g_fail = 0;
#define CHECK(cond)                                                          \
    do {                                                                     \
        if (cond) { ++g_pass; }                                              \
        else { ++g_fail; std::printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); } \
    } while (0)

static Node makeNode(int id, NodeType t, int ports) {
    Node n;
    n.id = id; n.type = t; n.totalPorts = ports; n.usedPorts = 0; n.processingTime = 0.0;
    return n;
}
static Edge makeEdge(int id, int u, int v, double length) {
    Edge e;
    e.id = id; e.u = u; e.v = v; e.length = length;
    e.maxSegmentLength = 1e9; e.isUp = true;
    return e;
}

// ------------------------------------------------------------
// A3.1: PathResult - cac truong co dung ban chat khong
// ------------------------------------------------------------
static void testPathResultBasics() {
    std::printf("[PathResult co ban]\n");
    Graph g;
    g.addNode(makeNode(0, NodeType::ROUTER, 4));
    g.addNode(makeNode(1, NodeType::ROUTER, 4));
    g.addEdge(makeEdge(0, 0, 1, 10));

    PathResult ok = dijkstra(g, 0, 1, lengthWeight);
    CHECK(ok.reachable == true);
    CHECK(ok.hopCount == static_cast<int>(ok.edges.size()));
    CHECK(ok.nodes.size() == ok.edges.size() + 1);
    CHECK(ok.nodes.front() == 0 && ok.nodes.back() == 1);

    // Khong co duong -> reachable = false
    Graph g2;
    g2.addNode(makeNode(0, NodeType::ROUTER, 4));
    g2.addNode(makeNode(1, NodeType::ROUTER, 4));   // khong noi gi voi 0
    PathResult fail = dijkstra(g2, 0, 1, lengthWeight);
    CHECK(fail.reachable == false);
    CHECK(fail.nodes.empty() && fail.edges.empty());

    // source == destination: duong rong nhung van reachable
    PathResult same = dijkstra(g, 0, 0, lengthWeight);
    CHECK(same.reachable == true);
    CHECK(same.hopCount == 0);
    CHECK(same.nodes == std::vector<int>({0}));

    // Node khong ton tai -> reachable = false, khong crash
    PathResult noNode = dijkstra(g, 0, 999, lengthWeight);
    CHECK(noNode.reachable == false);
}

// ------------------------------------------------------------
// A3.2: weight = 1 (giong BFS) va weight = length
// ------------------------------------------------------------
static void testWeightUnitVsLength() {
    std::printf("[Dijkstra weight=1 va weight=length]\n");
    // 0 -- 1 -- 2   (di thang, moi doan dai 10) : 2 hop, length 20
    // 0 ---------2  (di tat, 1 doan dai 100)     : 1 hop, length 100
    Graph g;
    for (int i = 0; i < 3; ++i) g.addNode(makeNode(i, NodeType::ROUTER, 4));
    g.addEdge(makeEdge(0, 0, 1, 10));
    g.addEdge(makeEdge(1, 1, 2, 10));
    g.addEdge(makeEdge(2, 0, 2, 100));

    // weight = 1: duong tat (1 hop) re hon ve so hop du dai hon ve khoang cach
    PathResult byHop = dijkstra(g, 0, 2, unitWeight);
    CHECK(byHop.reachable && byHop.hopCount == 1);
    CHECK(byHop.edges == std::vector<int>({2}));
    CHECK(std::fabs(byHop.totalCost - 1.0) < 1e-9);

    // weight = length: di vong qua node 1 (20) re hon di tat (100)
    PathResult byLen = dijkstra(g, 0, 2, lengthWeight);
    CHECK(byLen.reachable && byLen.hopCount == 2);
    CHECK(byLen.nodes == std::vector<int>({0, 1, 2}));
    CHECK(byLen.edges == std::vector<int>({0, 1}));
    CHECK(std::fabs(byLen.totalCost - 20.0) < 1e-9);
}

// ------------------------------------------------------------
// A3.3/A3.4: canh song song - phai chon dung EDGE re hon,
// khong chi dung "node ke" (adjacency luu theo edgeId nen lam duoc dieu nay)
// ------------------------------------------------------------
static void testParallelEdges() {
    std::printf("[Canh song song]\n");
    Graph g;
    g.addNode(makeNode(0, NodeType::ROUTER, 4));
    g.addNode(makeNode(1, NodeType::ROUTER, 4));
    g.addEdge(makeEdge(0, 0, 1, 50));   // day dat
    g.addEdge(makeEdge(1, 0, 1, 5));    // day re hon, cung noi 0-1

    PathResult r = dijkstra(g, 0, 1, lengthWeight);
    CHECK(r.reachable);
    CHECK(r.edges == std::vector<int>({1}));   // phai chon edge id=1 (re hon), khong phai id=0
    CHECK(std::fabs(r.totalCost - 5.0) < 1e-9);
}

// ------------------------------------------------------------
// A3.4: Relaxation - parent/parentEdge phai duoc CAP NHAT LAI
// khi tim thay duong re hon sau nay (khong giu ket qua cu)
// ------------------------------------------------------------
static void testRelaxationUpdates() {
    std::printf("[Relaxation cap nhat lai duong tot hon]\n");
    // 0 -> 1 (100), 0 -> 2 (1), 2 -> 1 (1)
    // Duong 0->2->1 (2) re hon 0->1 truc tiep (100) -> parent[1] phai la 2, khong phai 0
    Graph g;
    for (int i = 0; i < 3; ++i) g.addNode(makeNode(i, NodeType::ROUTER, 4));
    g.addEdge(makeEdge(0, 0, 1, 100));
    g.addEdge(makeEdge(1, 0, 2, 1));
    g.addEdge(makeEdge(2, 2, 1, 1));

    PathResult r = dijkstra(g, 0, 1, lengthWeight);
    CHECK(r.reachable);
    CHECK(r.nodes == std::vector<int>({0, 2, 1}));
    CHECK(r.edges == std::vector<int>({1, 2}));
    CHECK(std::fabs(r.totalCost - 2.0) < 1e-9);
}

// ------------------------------------------------------------
// A3.5: Reconstruct path - dung THU TU (source -> destination),
// khong bi nguoc chieu
// ------------------------------------------------------------
static void testReconstructOrder() {
    std::printf("[Reconstruct path dung thu tu]\n");
    // Chuoi thang 0-1-2-3-4, giong vi du trong task board (5 node, 4 hop)
    Graph g;
    for (int i = 0; i < 5; ++i) g.addNode(makeNode(i, NodeType::ROUTER, 4));
    for (int i = 0; i < 4; ++i) g.addEdge(makeEdge(i, i, i + 1, 1));

    PathResult r = dijkstra(g, 0, 4, unitWeight);
    CHECK(r.reachable);
    CHECK(r.nodes == std::vector<int>({0, 1, 2, 3, 4}));   // dung chieu, khong dao nguoc
    CHECK(r.edges == std::vector<int>({0, 1, 2, 3}));
    CHECK(r.hopCount == 4);

    // Doi chieu nguon/dich -> duong phai doi chieu theo, van dung thu tu
    PathResult rev = dijkstra(g, 4, 0, unitWeight);
    CHECK(rev.reachable);
    CHECK(rev.nodes == std::vector<int>({4, 3, 2, 1, 0}));
}

// ------------------------------------------------------------
// Tich hop voi du lieu mang mau that (data/nodes.txt, data/edges.txt)
// H1 = node 4, H2 = node 5. Duong ngan nhat theo do dai da tinh tay truoc:
//   H1 -(e8,130)-> R2(1) -(e1,500)-> R4(3) -(e7,50)-> H2  = tong 680m, 3 hop
// ------------------------------------------------------------
static void testWithSampleNetwork() {
    std::printf("[Mang mau that (data/nodes.txt, data/edges.txt)]\n");
    Graph g;
    bool okNodes = IOManager::loadNodes("data/nodes.txt", g);
    bool okEdges = IOManager::loadEdges("data/edges.txt", g);
    if (!okNodes || !okEdges) {
        std::printf("  (bo qua: khong doc duoc data/nodes.txt hoac data/edges.txt - "
                     "hay chay test tu thu muc goc repo)\n");
        return;
    }

    PathResult byLen = dijkstra(g, 4, 5, lengthWeight);
    CHECK(byLen.reachable);
    CHECK(byLen.nodes == std::vector<int>({4, 1, 3, 5}));
    CHECK(byLen.edges == std::vector<int>({8, 1, 7}));
    CHECK(std::fabs(byLen.totalCost - 680.0) < 1e-6);

    PathResult byHop = dijkstra(g, 4, 5, unitWeight);
    CHECK(byHop.reachable);
    CHECK(byHop.hopCount == 3);   // it nhat 3 hop de tu H1 sang H2
}

int main() {
    testPathResultBasics();
    testWeightUnitVsLength();
    testParallelEdges();
    testRelaxationUpdates();
    testReconstructOrder();
    testWithSampleNetwork();

    std::printf("\nKet qua: %d dat, %d loi\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
