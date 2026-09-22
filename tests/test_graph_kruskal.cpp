// ============================================================
// Test tuan 1-2: Graph, DSU, Kruskal, Backup.
// Chay tu thu muc goc repo.
// Khong can thu vien ngoai. Bien dich va chay:
//   g++ -std=c++17 -Wall -Wextra -Iinclude src/Graph.cpp src/DSU.cpp src/Kruskal.cpp tests/unit/test_graph_kruskal.cpp -o test_graph_kruskal
//   ./test_graph_kruskal
// ============================================================
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <queue>
#include <random>
#include <set>
#include <vector>

#include "Graph.h"
#include "DSU.h"
#include "Kruskal.h"

static int g_pass = 0, g_fail = 0;
#define CHECK(cond)                                                          \
    do {                                                                     \
        if (cond) { ++g_pass; }                                              \
        else { ++g_fail; std::printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); } \
    } while (0)

// ---------- Tao node / edge nhanh ----------
// (tham so cuoi cua makeNode giu lai cho de doc; struct Node hien khong co deviceCost)
static Node makeNode(int id, NodeType t, int ports, double proc, double /*deviceCost*/) {
    Node n;
    n.id = id; n.type = t; n.totalPorts = ports; n.processingTime = proc;
    return n;
}
static Edge makeEdge(int id, int u, int v, MediaType m, double len, double terrain) {
    Edge e;
    e.id = id; e.u = u; e.v = v; e.mediaType = m; e.length = len; e.terrainFactor = terrain;
    if (m == MediaType::FIBER) {
        e.unitPrice = 5; e.equipmentCost = 40; e.maintenanceCost = 8;
        e.maxSegmentLength = 2000; e.bandwidthMbps = 1000;
    } else {
        e.unitPrice = 2; e.equipmentCost = 10; e.maintenanceCost = 5;
        e.maxSegmentLength = 100; e.bandwidthMbps = 100;
    }
    return e;
}

// Chi phi mau theo Phan 6 cua tai lieu: co them phi lap dat (fiber 50, copper 20).
// Struct Edge hien chua co field installationCost nen test tu cong vao day;
// khi CostModel cua ban B xong thi thay bang calculateCablingCost.
static double sampleCost(const Edge& e) {
    double install = (e.mediaType == MediaType::FIBER) ? 50.0 : 20.0;
    return install + defaultCablingCost(e);
}
static MSTResult build(Graph& g, std::vector<Rejection>* r = nullptr) { return buildMST(g, sampleCost, r); }
static BackupResult backup(Graph& g, const MSTResult& m, int k) { return selectBackupLinks(g, m, k, sampleCost); }

// Mang mau giong data/sample va Phan 6 cua tai lieu (R1..R4 = 0..3, H1 = 4, H2 = 5)
static Graph sampleGraph() {
    Graph g;
    for (int i = 0; i < 4; ++i) g.addNode(makeNode(i, NodeType::ROUTER, 4, 1e-4, 500));
    g.addNode(makeNode(4, NodeType::HOST, 1, 0, 0));
    g.addNode(makeNode(5, NodeType::HOST, 1, 0, 0));
    g.addEdge(makeEdge(0, 0, 1, MediaType::FIBER, 400, 1.0));
    g.addEdge(makeEdge(1, 1, 3, MediaType::FIBER, 500, 1.0));
    g.addEdge(makeEdge(2, 0, 2, MediaType::FIBER, 450, 1.2));
    g.addEdge(makeEdge(3, 2, 3, MediaType::FIBER, 600, 1.0));
    g.addEdge(makeEdge(4, 1, 2, MediaType::FIBER, 300, 1.0));
    g.addEdge(makeEdge(5, 0, 3, MediaType::FIBER, 1200, 1.5));
    g.addEdge(makeEdge(6, 0, 4, MediaType::COPPER, 40, 1.0));
    g.addEdge(makeEdge(7, 3, 5, MediaType::COPPER, 50, 1.0));
    g.addEdge(makeEdge(8, 1, 4, MediaType::COPPER, 130, 1.0));  // 130 m > 100 m: khong hop le
    return g;
}

static std::set<int> asSet(const std::vector<int>& v) { return std::set<int>(v.begin(), v.end()); }

// Hai node a, b co noi duoc bang cac edge da xay (bo qua edge `skip`) khong
static bool builtConnected(const Graph& g, int a, int b, int skip) {
    std::vector<char> seen(g.V(), 0);
    std::queue<int> q;
    q.push(a); seen[a] = 1;
    while (!q.empty()) {
        int x = q.front(); q.pop();
        if (x == b) return true;
        for (int id : g.getNeighbors(x)) {
            const Edge& e = g.getEdge(id);
            if (id == skip || !e.isBuilt || !e.isUp) continue;
            int y = g.otherEndpoint(x, id);
            if (!seen[y]) { seen[y] = 1; q.push(y); }
        }
    }
    return false;
}

// ============================================================
// TUAN 1
// ============================================================
static void testGraph() {
    std::puts("[Graph]");
    Graph g;
    CHECK(g.V() == 0 && g.E() == 0);
    for (int i = 0; i < 3; ++i) CHECK(g.addNode(makeNode(i, NodeType::ROUTER, 4, 0, 0)));
    CHECK(g.V() == 3);                                   // them 3 node -> V = 3

    CHECK(g.addEdge(makeEdge(0, 0, 1, MediaType::FIBER, 100, 1)));
    CHECK(g.addEdge(makeEdge(1, 1, 2, MediaType::FIBER, 100, 1)));
    CHECK(g.E() == 2);                                   // them 2 edge -> E = 2

    // moi edge xuat hien o adjacency cua CA HAI dau
    auto has = [&](int node, int edge) {
        const auto& a = g.getNeighbors(node);
        return std::find(a.begin(), a.end(), edge) != a.end();
    };
    CHECK(has(0, 0) && has(1, 0));
    CHECK(has(1, 1) && has(2, 1));
    CHECK(!has(0, 1) && !has(2, 0));

    size_t sum = 0;
    for (int u = 0; u < g.V(); ++u) sum += g.getNeighbors(u).size();
    CHECK(sum == static_cast<size_t>(2 * g.E()));        // tong adjacency = 2E

    CHECK(g.getEdge(1).id == 1 && g.getEdge(1).u == 1 && g.getEdge(1).v == 2);
    CHECK(g.otherEndpoint(0, 0) == 1);                   // otherEndpoint(nodeId, edgeId)
    CHECK(g.otherEndpoint(1, 0) == 0);
    CHECK(g.otherEndpoint(2, 0) == -1);                  // node khong thuoc edge
    CHECK(g.otherEndpoint(0, 99) == -1);                 // edge khong ton tai

    // Tu choi du lieu sai va khong thay doi do thi
    CHECK(!g.addNode(makeNode(1, NodeType::ROUTER, 4, 0, 0)));            // trung id node
    CHECK(!g.addEdge(makeEdge(2, 0, 99, MediaType::FIBER, 100, 1)));      // node khong ton tai
    CHECK(!g.addEdge(makeEdge(2, 1, 1, MediaType::FIBER, 100, 1)));       // khuyen
    CHECK(!g.addEdge(makeEdge(1, 0, 2, MediaType::FIBER, 100, 1)));       // trung id edge
    CHECK(g.V() == 3 && g.E() == 2);
    CHECK(g.getEdge(1).u == 1 && g.getEdge(1).v == 2);                    // edge cu khong bi ghi de

    // getNode / getEdge nem exception khi khong co (Validation dua vao hanh vi nay)
    bool threwNode = false, threwEdge = false;
    try { g.getNode(42); } catch (const std::runtime_error&) { threwNode = true; }
    try { g.getEdge(42); } catch (const std::runtime_error&) { threwEdge = true; }
    CHECK(threwNode && threwEdge);
    CHECK(g.hasNode(2) && !g.hasNode(3) && g.hasEdge(0) && !g.hasEdge(2));

    // getNode tra THAM CHIEU: sua qua no thi do thi doi (Kruskal can dieu nay)
    g.getNode(0).usedPorts = 3;
    CHECK(g.getNode(0).usedPorts == 3);
    g.getEdge(0).isUp = false;
    CHECK(!g.getEdge(0).isUp);
    g.getNode(0).usedPorts = 0;
    g.getEdge(0).isUp = true;

    // Id khong lien tuc van dung duoc; nodeIds()/edgeIds() luon tang dan
    Graph sparse;
    sparse.addNode(makeNode(30, NodeType::ROUTER, 4, 0, 0));
    sparse.addNode(makeNode(10, NodeType::ROUTER, 4, 0, 0));
    sparse.addNode(makeNode(20, NodeType::ROUTER, 4, 0, 0));
    sparse.addEdge(makeEdge(7, 30, 10, MediaType::FIBER, 100, 1));
    CHECK(sparse.nodeIds() == std::vector<int>({10, 20, 30}));
    CHECK(sparse.maxNodeId() == 30 && sparse.V() == 3 && sparse.E() == 1);
    CHECK(sparse.getNeighbors(20).empty());              // node co lap: danh sach rong
    CHECK(sparse.getNeighbors(999).empty());             // node khong ton tai: rong, khong nem

    // Hai node co 2 day ung vien (cap dong + cap quang): ca hai deu duoc giu
    CHECK(g.addEdge(makeEdge(2, 0, 1, MediaType::COPPER, 50, 1)));
    CHECK(g.getNeighbors(0).size() == 2 && g.getNeighbors(1).size() == 3);

    // buildAdjacency dung lai cho ket qua giong het
    auto before = g.getNeighbors(1);
    g.buildAdjacency();
    CHECK(g.getNeighbors(1) == before);
}

static void testDSUBasic() {
    std::puts("[DSU co ban]");
    DSU d(3);
    CHECK(d.components() == 3);
    CHECK(d.unite(0, 1));
    CHECK(d.unite(1, 2));
    CHECK(d.find(0) == d.find(1));
    CHECK(d.find(1) == d.find(2));
    CHECK(d.find(0) == d.find(2));
    CHECK(d.components() == 1);
}

// ============================================================
// TUAN 2
// ============================================================
static void testDSUFull() {
    std::puts("[DSU day du]");
    DSU d(6);
    CHECK(d.unite(0, 1));
    CHECK(!d.unite(1, 0));                 // da cung tap -> false
    CHECK(!d.unite(0, 0));
    CHECK(d.unite(2, 3));
    CHECK(!d.connected(1, 2));
    CHECK(d.unite(1, 2));
    CHECK(d.connected(0, 3));
    CHECK(!d.connected(0, 4));
    CHECK(d.components() == 3);            // {0,1,2,3} {4} {5}

    // chuoi dai: find phai dung sau khi nen duong di
    DSU chain(2000);
    for (int i = 0; i + 1 < 2000; ++i) chain.unite(i, i + 1);
    bool allSame = true;
    for (int i = 0; i < 2000; ++i) if (chain.find(i) != chain.find(0)) allSame = false;
    CHECK(allSame);
    CHECK(chain.components() == 1);
}

static void testCompareEdges() {
    std::puts("[compareEdges]");
    Edge cheap = makeEdge(5, 0, 1, MediaType::COPPER, 40, 1);   // 95 (mac dinh, chua co phi lap dat)
    Edge dear  = makeEdge(2, 0, 1, MediaType::FIBER, 400, 1);   // 2048
    CHECK(compareEdges(cheap, dear));       // cost nho dung truoc du id lon hon
    CHECK(!compareEdges(dear, cheap));

    Edge a = makeEdge(3, 0, 1, MediaType::FIBER, 400, 1);
    Edge b = makeEdge(7, 2, 3, MediaType::FIBER, 400, 1);       // cung cost, id lon hon
    CHECK(defaultCablingCost(a) == defaultCablingCost(b));
    CHECK(compareEdges(a, b));              // bang cost -> id nho truoc
    CHECK(!compareEdges(b, a));
    CHECK(!compareEdges(a, a));             // strict weak ordering: khong so sanh ban than

    // Hai lan chay cung du lieu -> cung thu tu
    Graph g = sampleGraph();
    std::vector<Rejection> r1, r2;
    MSTResult m1 = build(g, &r1);
    MSTResult m2 = build(g, &r2);
    CHECK(m1.edgeIds == m2.edgeIds);
    CHECK(r1.size() == r2.size());
}

static void testCanSelectEdge() {
    std::puts("[canSelectEdge]");
    Graph g = sampleGraph();
    DSU dsu(g.V());

    // Dang trong trang thai ban dau: e0 (fiber 400 m, du cong) chon duoc
    CHECK(canSelectEdge(g, g.getEdge(0), dsu));

    // Cung thanh phan -> CYCLE
    dsu.unite(0, 1);
    CHECK(checkEdge(g, g.getEdge(0), dsu) == RejectReason::CYCLE);

    // Qua dai: e8 la cap dong 130 m > 100 m
    CHECK(checkEdge(g, g.getEdge(8), dsu) == RejectReason::TOO_LONG);

    // Het cong: dat H1 (1 cong) da dung 1 -> e6 (R1-H1) bi tu choi
    g.getNode(4).usedPorts = 1;
    CHECK(checkEdge(g, g.getEdge(6), dsu) == RejectReason::NO_PORT);

    // Day dang down
    g.getNode(4).usedPorts = 0;
    g.getEdge(6).isUp = false;
    CHECK(checkEdge(g, g.getEdge(6), dsu) == RejectReason::LINK_DOWN);
}

static void testBuildMSTSample() {
    std::puts("[buildMST tren mang mau]");
    Graph g = sampleGraph();
    std::vector<Rejection> rej;
    MSTResult mst = build(g, &rej);

    CHECK(mst.connected);
    CHECK(static_cast<int>(mst.edgeIds.size()) == g.V() - 1);           // V - 1 canh
    CHECK(asSet(mst.edgeIds) == std::set<int>({0, 1, 4, 6, 7}));
    CHECK(std::fabs(mst.totalCost - 6544.0) < 1e-9);                     // doi chieu checkpoint tuan 2
    CHECK(std::fabs(sumCablingCost(g, mst.edgeIds, sampleCost) - mst.totalCost) < 1e-9);

    // Khong cycle: dung DSU doc lap dem thanh phan
    DSU chk(g.V());
    bool noCycle = true;
    for (int id : mst.edgeIds) if (!chk.unite(g.getEdge(id).u, g.getEdge(id).v)) noCycle = false;
    CHECK(noCycle);
    CHECK(chk.components() == 1);

    // usedPorts khop so canh cay cham vao node
    CHECK(g.getNode(0).usedPorts == 2);   // e0, e6
    CHECK(g.getNode(1).usedPorts == 3);   // e0, e1, e4
    CHECK(g.getNode(2).usedPorts == 1);   // e4
    CHECK(g.getNode(3).usedPorts == 2);   // e1, e7
    CHECK(g.getNode(4).usedPorts == 1 && g.getNode(5).usedPorts == 1);

    // Ly do bi loai
    auto reasonOf = [&](int id) {
        for (const Rejection& r : rej) if (r.edgeId == id) return r.reason;
        return RejectReason::NONE;
    };
    CHECK(reasonOf(8) == RejectReason::TOO_LONG);
    CHECK(reasonOf(2) == RejectReason::CYCLE);
    CHECK(reasonOf(3) == RejectReason::CYCLE);
    CHECK(reasonOf(5) == RejectReason::CYCLE);
    CHECK(rej.size() == 4);               // 9 canh - 5 chon = 4 bi loai

    // Goi lai nhieu lan phai cho cung ket qua (reset trang thai)
    MSTResult again = build(g);
    CHECK(again.edgeIds == mst.edgeIds && g.getNode(1).usedPorts == 3);
}

static void testBuildMSTEdgeCases() {
    std::puts("[buildMST truong hop bien]");

    // Do thi rong / 1 node
    Graph empty;
    CHECK(!build(empty).connected);
    Graph one;
    one.addNode(makeNode(0, NodeType::ROUTER, 4, 0, 0));
    MSTResult r1 = build(one);
    CHECK(r1.connected && r1.edgeIds.empty() && r1.totalCost == 0.0);

    // Khong lien thong: 2 cum {0,1} va {2,3}
    Graph d;
    for (int i = 0; i < 4; ++i) d.addNode(makeNode(i, NodeType::ROUTER, 4, 0, 0));
    d.addEdge(makeEdge(0, 0, 1, MediaType::FIBER, 100, 1));
    d.addEdge(makeEdge(1, 2, 3, MediaType::FIBER, 100, 1));
    MSTResult rd = build(d);
    CHECK(!rd.connected);
    CHECK(rd.edgeIds.size() == 2);        // van chon duoc rung 2 canh

    // Day duy nhat qua dai -> khong noi duoc
    Graph far;
    far.addNode(makeNode(0, NodeType::HOST, 1, 0, 0));
    far.addNode(makeNode(1, NodeType::HOST, 1, 0, 0));
    far.addEdge(makeEdge(0, 0, 1, MediaType::COPPER, 500, 1));
    CHECK(!build(far).connected);

    // Rang buoc cong: trung tam 2 cong noi 4 la -> khong the noi het
    Graph star;
    star.addNode(makeNode(0, NodeType::ROUTER, 2, 0, 0));
    for (int i = 1; i <= 4; ++i) star.addNode(makeNode(i, NodeType::HOST, 1, 0, 0));
    for (int i = 1; i <= 4; ++i) star.addEdge(makeEdge(i - 1, 0, i, MediaType::COPPER, 10, 1));
    MSTResult rs = build(star);
    CHECK(!rs.connected);
    CHECK(rs.edgeIds.size() == 2);        // chi 2 canh vi trung tam chi co 2 cong
    CHECK(star.getNode(0).usedPorts == 2);

    // Id node khong lien tuc (10, 20, 30) van xay duoc cay
    Graph sp;
    for (int id : {10, 20, 30}) sp.addNode(makeNode(id, NodeType::ROUTER, 4, 0, 0));
    sp.addEdge(makeEdge(0, 10, 20, MediaType::FIBER, 100, 1));
    sp.addEdge(makeEdge(1, 20, 30, MediaType::FIBER, 100, 1));
    sp.addEdge(makeEdge(2, 10, 30, MediaType::FIBER, 900, 1));
    MSTResult rsp = build(sp);
    CHECK(rsp.connected && asSet(rsp.edgeIds) == std::set<int>({0, 1}));

    // Day down khong duoc chon, di duong vong
    Graph tri;
    for (int i = 0; i < 3; ++i) tri.addNode(makeNode(i, NodeType::ROUTER, 4, 0, 0));
    tri.addEdge(makeEdge(0, 0, 1, MediaType::FIBER, 100, 1));
    tri.addEdge(makeEdge(1, 1, 2, MediaType::FIBER, 100, 1));
    tri.addEdge(makeEdge(2, 0, 2, MediaType::FIBER, 900, 1));
    tri.getEdge(0).isUp = false;
    MSTResult rt = build(tri);
    CHECK(rt.connected && asSet(rt.edgeIds) == std::set<int>({1, 2}));
}

// Prim (khong rang buoc cong) de doi chieu tong chi phi voi Kruskal
static double primCost(const Graph& g) {
    std::vector<char> in(g.V(), 0);
    using P = std::pair<double, int>;
    std::priority_queue<P, std::vector<P>, std::greater<P>> pq;
    pq.push({0.0, 0});
    double total = 0;
    while (!pq.empty()) {
        auto [w, x] = pq.top(); pq.pop();
        if (in[x]) continue;
        in[x] = 1; total += w;
        for (int id : g.getNeighbors(x)) {
            const Edge& e = g.getEdge(id);
            int y = g.otherEndpoint(x, id);
            if (!in[y] && e.isUp && (e.length <= e.maxSegmentLength)) pq.push({sampleCost(e), y});
        }
    }
    return total;
}

static void testRandomGraphs() {
    std::puts("[do thi ngau nhien]");
    std::mt19937 rng(12345);
    int matched = 0, portOk = 0, treeOk = 0;
    const int TRIALS = 200;
    for (int t = 0; t < TRIALS; ++t) {
        int n = 5 + rng() % 25;
        Graph g;
        for (int i = 0; i < n; ++i) g.addNode(makeNode(i, NodeType::ROUTER, 1000, 0, 0));
        int id = 0;
        for (int i = 1; i < n; ++i)   // dam bao lien thong: moi node noi voi 1 node truoc do
            g.addEdge(makeEdge(id++, static_cast<int>(rng() % i), i, MediaType::FIBER, 50 + rng() % 1500, 1.0 + (rng() % 5) * 0.1));
        int extra = n + rng() % (2 * n);
        for (int j = 0; j < extra; ++j) {
            int a = rng() % n, b = rng() % n;
            if (a == b) continue;
            g.addEdge(makeEdge(id++, a, b, MediaType::FIBER, 50 + rng() % 1500, 1.0 + (rng() % 5) * 0.1));
        }
        MSTResult m = build(g);
        if (m.connected && std::fabs(m.totalCost - primCost(g)) < 1e-6) ++matched;
        if (static_cast<int>(m.edgeIds.size()) == n - 1) ++treeOk;

        // Voi rang buoc cong chat: khong node nao duoc vuot totalPorts
        for (int i = 0; i < n; ++i) g.getNode(i).totalPorts = 2 + static_cast<int>(rng() % 3);
        MSTResult mc = build(g);
        bool ok = true;
        for (int nid : g.nodeIds()) if (g.getNode(nid).usedPorts > g.getNode(nid).totalPorts) ok = false;
        DSU chk(n);
        for (int e : mc.edgeIds) if (!chk.unite(g.getEdge(e).u, g.getEdge(e).v)) ok = false;
        if (ok) ++portOk;
    }
    CHECK(matched == TRIALS);   // Kruskal == Prim khi khong co rang buoc cong
    CHECK(treeOk == TRIALS);
    CHECK(portOk == TRIALS);    // co rang buoc: khong vuot cong, khong cycle
}

static void testBackupSample() {
    std::puts("[selectBackupLinks tren mang mau]");
    Graph g = sampleGraph();
    MSTResult mst = build(g);
    BackupResult b = backup(g, mst, 2);

    CHECK(asSet(b.edgeIds) == std::set<int>({2, 3}));
    CHECK(std::fabs(b.totalCost - (2798.0 + 3098.0)) < 1e-9);
    CHECK(std::fabs(mst.totalCost + b.totalCost - 12440.0) < 1e-9);      // doi chieu checkpoint tuan 2

    // Backup khong trung MST, co co, co isBuilt
    std::set<int> tree = asSet(mst.edgeIds);
    bool flags = true;
    for (int id : b.edgeIds) {
        const Edge& e = g.getEdge(id);
        if (tree.count(id) || !e.isBackup || !e.isBuilt) flags = false;
    }
    CHECK(flags);
    for (int id : mst.edgeIds) CHECK(!g.getEdge(id).isBackup && g.getEdge(id).isBuilt);

    // Khong vuot cong
    bool ports = true;
    for (int nid : g.nodeIds()) if (g.getNode(nid).usedPorts > g.getNode(nid).totalPorts) ports = false;
    CHECK(ports);

    // Backup THAT SU tao duong thay the: kiem tra doc lap bang cach bo tung canh cay
    int protectedByBruteForce = 0;
    for (int t : mst.edgeIds) {
        const Edge& e = g.getEdge(t);
        if (builtConnected(g, e.u, e.v, t)) ++protectedByBruteForce;
    }
    CHECK(protectedByBruteForce == b.protectedTreeEdges);
    CHECK(b.treeEdges == 5 && b.protectedTreeEdges == 3);   // 2 canh host e6, e7 khong the bao ve

    // Cu the: bo e0 van co duong R1 -> R2 (qua R3), nhung bo e6 (canh host) thi mat H1
    CHECK(builtConnected(g, 0, 1, 0));
    CHECK(!builtConnected(g, 0, 4, 6));
}

static void testBackupEdgeCases() {
    std::puts("[selectBackupLinks truong hop bien]");
    // k = 0 khong chon gi
    Graph g0 = sampleGraph();
    MSTResult m0 = build(g0);
    CHECK(backup(g0, m0, 0).edgeIds.empty());

    // k lon: e5 (9098) khong them loi gi (da bao ve het nho e2, e3) -> khong chon
    Graph g1 = sampleGraph();
    MSTResult m1 = build(g1);
    BackupResult b1 = backup(g1, m1, 10);
    CHECK(asSet(b1.edgeIds) == std::set<int>({2, 3}));
    CHECK(!g1.getEdge(5).isBuilt);
    CHECK(!g1.getEdge(8).isBuilt);         // day qua dai khong bao gio thanh backup

    // k = 1 chi chon canh re nhat (e2)
    Graph g2 = sampleGraph();
    MSTResult m2 = build(g2);
    BackupResult b2 = backup(g2, m2, 1);
    CHECK(b2.edgeIds == std::vector<int>({2}));

    // Backup phai ton trong cong: R3 chi con 1 cong trong. e2 (re nhat) lay cong do,
    // e3 cung cham R3 nen bi bo qua; e5 (R1-R4) khong qua R3 va van them duoc loi (bao ve e1).
    Graph g3 = sampleGraph();
    g3.getNode(2).totalPorts = 2;          // R3: e4 (cay) + 1 cong con lai
    MSTResult m3 = build(g3);
    BackupResult b3 = backup(g3, m3, 5);
    CHECK(g3.getNode(2).usedPorts <= 2);
    CHECK(asSet(b3.edgeIds) == std::set<int>({2, 5}));
    CHECK(!g3.getEdge(3).isBuilt);

    // Do thi la cay thuan tuy (khong co ung vien): khong backup
    Graph path;
    for (int i = 0; i < 3; ++i) path.addNode(makeNode(i, NodeType::ROUTER, 4, 0, 0));
    path.addEdge(makeEdge(0, 0, 1, MediaType::FIBER, 100, 1));
    path.addEdge(makeEdge(1, 1, 2, MediaType::FIBER, 100, 1));
    MSTResult mp = build(path);
    BackupResult bp = backup(path, mp, 3);
    CHECK(bp.edgeIds.empty() && bp.protectedTreeEdges == 0 && bp.treeEdges == 2);

    // Tam giac 3 router: them 1 backup la bao ve ca 2 canh cay
    Graph tri;
    for (int i = 0; i < 3; ++i) tri.addNode(makeNode(i, NodeType::ROUTER, 4, 0, 0));
    tri.addEdge(makeEdge(0, 0, 1, MediaType::FIBER, 100, 1));
    tri.addEdge(makeEdge(1, 1, 2, MediaType::FIBER, 100, 1));
    tri.addEdge(makeEdge(2, 0, 2, MediaType::FIBER, 900, 1));
    MSTResult mt = build(tri);
    BackupResult bt = backup(tri, mt, 3);
    CHECK(bt.edgeIds == std::vector<int>({2}));
    CHECK(bt.protectedTreeEdges == 2);
}

int main() {
    testGraph();
    testDSUBasic();
    testDSUFull();
    testCompareEdges();
    testCanSelectEdge();
    testBuildMSTSample();
    testBuildMSTEdgeCases();
    testRandomGraphs();
    testBackupSample();
    testBackupEdgeCases();

    std::printf("\nKet qua: %d dat, %d loi\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
