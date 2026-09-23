#include "Dijkstra.h"
#include <queue>
#include <unordered_map>
#include <limits>

double unitWeight(const Edge&) { return 1.0; }
double lengthWeight(const Edge& e) { return e.length; }

namespace {

// ------------------------------------------------------------
// Task A3.3 - Min heap
// State giữ (khoảng cách hiện tại tới node, node đó).
// priority_queue mặc định là MAX-heap, nên phải dùng greater<State>
// để lấy ra được State có dist NHỎ NHẤT trước - đúng ý tưởng của Dijkstra
// ("luôn xử lý node gần nguồn nhất còn lại trước").
// ------------------------------------------------------------
struct State {
    double dist;
    int    node;
};

bool operator>(const State& a, const State& b) {
    return a.dist > b.dist;
}

} // namespace

PathResult dijkstra(const Graph& g, int source, int destination, const WeightFn& weight, bool onlyBuilt) {
    PathResult result;

    if (!g.hasNode(source) || !g.hasNode(destination)) return result;  // reachable = false

    // dist/parent/parentEdge dùng unordered_map vì id node có thể không liên tục (sparse)
    std::unordered_map<int, double> dist;
    std::unordered_map<int, int>    parent;      // parent[v] = node trước v trên đường đi ngắn nhất
    std::unordered_map<int, int>    parentEdge;  // parentEdge[v] = edgeId nối parent[v] -> v
    std::unordered_map<int, bool>   visited;     // node đã "chốt" khoảng cách ngắn nhất chưa

    for (int id : g.nodeIds()) dist[id] = std::numeric_limits<double>::infinity();
    dist[source] = 0.0;

    // Task A3.3: Min heap - luôn pop ra state có dist nhỏ nhất
    std::priority_queue<State, std::vector<State>, std::greater<State>> pq;
    pq.push({0.0, source});

    while (!pq.empty()) {
        State cur = pq.top();
        pq.pop();

        // Bỏ record cũ: node đã chốt rồi, hoặc dist trong state đã lỗi thời
        // (đã bị một lần relax sau ghi đè bằng giá trị nhỏ hơn) thì bỏ qua.
        if (visited[cur.node]) continue;
        if (cur.dist > dist[cur.node]) continue;

        visited[cur.node] = true;
        if (cur.node == destination) break;   // đã chốt xong đích, không cần đi tiếp

        // Task A3.4: Relaxation
        for (int edgeId : g.getNeighbors(cur.node)) {
            const Edge& e = g.getEdge(edgeId);
            if (!e.isUp) continue;             // day dang dut -> khong the di qua
            if (onlyBuilt && !e.isBuilt) continue;  // chi di tren day DA LAP (MST + Backup)
            int v = g.otherEndpoint(cur.node, edgeId);
            if (v == -1 || visited[v]) continue;

            double w = weight(e);
            double newDist = dist[cur.node] + w;

            if (newDist < dist[v]) {           // if dist[v] > dist[u] + w
                dist[v] = newDist;
                parent[v] = cur.node;
                parentEdge[v] = edgeId;        // lưu ĐÚNG edgeId - quan trọng khi có cạnh song song
                pq.push({newDist, v});
            }
        }
    }

    if (!visited[destination]) {
        return result;   // reachable = false, không tới được
    }

    // Task A3.5: Reconstruct path - đi ngược từ destination về source rồi đảo chiều
    std::vector<int> nodesRev;
    std::vector<int> edgesRev;
    int cur = destination;
    nodesRev.push_back(cur);
    while (cur != source) {
        edgesRev.push_back(parentEdge[cur]);
        cur = parent[cur];
        nodesRev.push_back(cur);
    }

    result.reachable = true;
    result.nodes.assign(nodesRev.rbegin(), nodesRev.rend());
    result.edges.assign(edgesRev.rbegin(), edgesRev.rend());
    result.totalCost = dist[destination];
    result.hopCount  = static_cast<int>(result.edges.size());

    return result;
}
