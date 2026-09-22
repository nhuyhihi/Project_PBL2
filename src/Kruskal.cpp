#include "Kruskal.h"
#include <algorithm>
#include <queue>
#include <unordered_map>
#include "Validation.h"

double defaultCablingCost(const Edge& e) {
    return e.length * e.unitPrice * e.terrainFactor + e.equipmentCost + e.maintenanceCost;
}

const char* toString(RejectReason r) {
    switch (r) {
        case RejectReason::NONE:      return "OK";
        case RejectReason::LINK_DOWN: return "LINK_DOWN";
        case RejectReason::TOO_LONG:  return "TOO_LONG";
        case RejectReason::CYCLE:     return "CYCLE";
        case RejectReason::NO_PORT:   return "NO_PORT";
    }
    return "?";
}

// ------------------------------------------------------------
// Sắp xếp: rẻ trước; bằng nhau thì id nhỏ trước
// (hai edge cùng dữ liệu tính cùng công thức nên so sánh bằng chính xác)
// ------------------------------------------------------------
bool compareEdges(const Edge& a, const Edge& b, const CostFn& cost) {
    const double ca = cost(a);
    const double cb = cost(b);
    if (ca != cb) return ca < cb;
    return a.id < b.id;
}

RejectReason checkEdge(const Graph& g, const Edge& e, DSU& dsu) {
    if (!e.isUp) return RejectReason::LINK_DOWN;                    // dây đang hỏng, không thể lắp
    if (!Validation::isStaticFeasible(e)) return RejectReason::TOO_LONG; // vượt độ dài tối đa của loại cáp
    if (dsu.connected(e.u, e.v)) return RejectReason::CYCLE;         // đã có đường u~v, thêm nữa thành chu trình
    if (!Validation::checkPortCapacity(g.getNode(e.u)) || !Validation::checkPortCapacity(g.getNode(e.v)))
        return RejectReason::NO_PORT;                                // một đầu hết cổng
    return RejectReason::NONE;
}

// ------------------------------------------------------------
// Kruskal có ràng buộc
// ------------------------------------------------------------
MSTResult buildMST(Graph& g, const CostFn& cost, std::vector<Rejection>* rejections) {
    g.resetBuildState();
    if (rejections) rejections->clear();

    MSTResult result;
    DSU dsu(g.maxNodeId() + 1);

    // Bước 1: sắp xếp id edge theo compareEdges
    std::vector<int> order = g.edgeIds();
    std::sort(order.begin(), order.end(), [&](int x, int y) {
        return compareEdges(g.getEdge(x), g.getEdge(y), cost);
    });

    // Bước 2-3: duyệt, chọn nếu hợp lệ
    for (int id : order) {
        Edge& e = g.getEdge(id);
        RejectReason why = checkEdge(g, e, dsu);
        if (why != RejectReason::NONE) {
            if (rejections) rejections->push_back({id, why});
            continue;
        }
        dsu.unite(e.u, e.v);
        g.getNode(e.u).usedPorts++;
        g.getNode(e.v).usedPorts++;
        e.isBuilt = true;
        result.edgeIds.push_back(id);
        result.totalCost += cost(e);
    }

    // Nối hết mọi node <=> đủ V-1 cạnh (V > 0)
    result.connected = (g.V() > 0 && static_cast<int>(result.edgeIds.size()) == g.V() - 1);
    return result;
}

// ------------------------------------------------------------
// Dây dự phòng
// ------------------------------------------------------------
namespace {

// Rừng tạo từ các cạnh cây: parent/depth/comp để tìm đường u -> v trong cây
struct TreeIndex {
    std::unordered_map<int, int> comp;        // thành phần chứa node
    std::unordered_map<int, int> depth;
    std::unordered_map<int, int> parentNode;
    std::unordered_map<int, int> parentEdge;  // edge nối node lên cha

    TreeIndex(const Graph& g, const std::vector<int>& treeEdges) {
        std::unordered_map<int, std::vector<int>> adj;
        for (int id : treeEdges) {
            const Edge& e = g.getEdge(id);
            adj[e.u].push_back(id);
            adj[e.v].push_back(id);
        }
        int c = 0;
        for (int s : g.nodeIds()) {
            if (comp.count(s)) continue;
            std::queue<int> q;
            q.push(s);
            comp[s] = c;
            depth[s] = 0;
            while (!q.empty()) {
                int x = q.front();
                q.pop();
                for (int id : adj[x]) {
                    int y = g.otherEndpoint(x, id);
                    if (comp.count(y)) continue;
                    comp[y] = c;
                    depth[y] = depth[x] + 1;
                    parentNode[y] = x;
                    parentEdge[y] = id;
                    q.push(y);
                }
            }
            ++c;
        }
    }

    // Các cạnh cây trên đường u -> v (leo lên từ node sâu hơn cho đến khi gặp nhau)
    std::vector<int> pathEdges(int u, int v) const {
        std::vector<int> out;
        while (u != v) {
            if (depth.at(u) >= depth.at(v)) {
                out.push_back(parentEdge.at(u));
                u = parentNode.at(u);
            } else {
                out.push_back(parentEdge.at(v));
                v = parentNode.at(v);
            }
        }
        return out;
    }
};

} // namespace

BackupResult selectBackupLinks(Graph& g, const MSTResult& mst, int k, const CostFn& cost) {
    BackupResult result;
    result.treeEdges = static_cast<int>(mst.edgeIds.size());
    if (k <= 0 || mst.edgeIds.empty()) return result;

    TreeIndex tree(g, mst.edgeIds);
    std::unordered_map<int, bool> covered;   // cạnh cây nào đã có đường thay thế

    // Ứng viên: chưa xây, đang up, hợp lệ về độ dài; sắp rẻ trước
    std::vector<int> candidates;
    for (int id : g.edgeIds()) {
        const Edge& e = g.getEdge(id);
        if (!e.isBuilt && e.isUp && e.length <= e.maxSegmentLength) candidates.push_back(id);
    }
    std::sort(candidates.begin(), candidates.end(), [&](int x, int y) {
        return compareEdges(g.getEdge(x), g.getEdge(y), cost);
    });

    for (int id : candidates) {
        if (static_cast<int>(result.edgeIds.size()) >= k) break;
        Edge& e = g.getEdge(id);
        if (!Validation::checkPortCapacity(g.getNode(e.u)) || !Validation::checkPortCapacity(g.getNode(e.v))) continue;
        if (tree.comp.at(e.u) != tree.comp.at(e.v)) continue;   // không có đường trong cây để bảo vệ

        // Chỉ chọn nếu bảo vệ thêm ít nhất một cạnh cây chưa được bảo vệ
        std::vector<int> path = tree.pathEdges(e.u, e.v);
        bool addsCoverage = false;
        for (int t : path) if (!covered[t]) { addsCoverage = true; break; }
        if (!addsCoverage) continue;

        for (int t : path) covered[t] = true;
        g.getNode(e.u).usedPorts++;
        g.getNode(e.v).usedPorts++;
        e.isBuilt = true;
        e.isBackup = true;
        result.edgeIds.push_back(id);
        result.totalCost += cost(e);
    }

    for (int t : mst.edgeIds) if (covered[t]) ++result.protectedTreeEdges;
    return result;
}

double sumCablingCost(const Graph& g, const std::vector<int>& edgeIds, const CostFn& cost) {
    double sum = 0.0;
    for (int id : edgeIds) sum += cost(g.getEdge(id));
    return sum;
}
