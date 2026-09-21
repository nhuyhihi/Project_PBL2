#include "Graph.h"
#include <algorithm>

Graph::Graph() {}

Graph::~Graph() {}

bool Graph::addNode(const Node& node) {
    if (nodes.count(node.id)) return false;   // id trùng: không ghi đè
    nodes[node.id] = node;
    adjacency[node.id];                       // tạo sẵn danh sách kề rỗng cho node này
    return true;
}

bool Graph::addEdge(const Edge& edge) {
    if (edges.count(edge.id)) return false;                   // id trùng
    if (!hasNode(edge.u) || !hasNode(edge.v)) return false;   // đầu mút phải tồn tại
    if (edge.u == edge.v) return false;                       // không cho dây khuyên
    edges[edge.id] = edge;
    // Mỗi edge xuất hiện ở adjacency của CẢ HAI đầu => tổng kích thước = 2E
    adjacency[edge.u].push_back(edge.id);
    adjacency[edge.v].push_back(edge.id);
    return true;
}

bool Graph::hasNode(int nodeId) const { return nodes.count(nodeId) > 0; }
bool Graph::hasEdge(int edgeId) const { return edges.count(edgeId) > 0; }

Node& Graph::getNode(int nodeId) {
    auto it = nodes.find(nodeId);
    if (it == nodes.end()) throw std::runtime_error("Node not found!");
    return it->second;
}

const Node& Graph::getNode(int nodeId) const {
    auto it = nodes.find(nodeId);
    if (it == nodes.end()) throw std::runtime_error("Node not found!");
    return it->second;
}

Edge& Graph::getEdge(int edgeId) {
    auto it = edges.find(edgeId);
    if (it == edges.end()) throw std::runtime_error("Edge not found!");
    return it->second;
}

const Edge& Graph::getEdge(int edgeId) const {
    auto it = edges.find(edgeId);
    if (it == edges.end()) throw std::runtime_error("Edge not found!");
    return it->second;
}

const std::vector<int>& Graph::getNeighbors(int nodeId) const {
    static const std::vector<int> empty;
    auto it = adjacency.find(nodeId);
    return it != adjacency.end() ? it->second : empty;
}

int Graph::otherEndpoint(int nodeId, int edgeId) const {
    auto it = edges.find(edgeId);
    if (it == edges.end()) return -1;
    const Edge& e = it->second;
    if (nodeId == e.u) return e.v;
    if (nodeId == e.v) return e.u;
    return -1;   // node không thuộc edge này
}

void Graph::buildAdjacency() {
    adjacency.clear();
    for (const auto& kv : nodes) adjacency[kv.first];   // node cô lập vẫn có danh sách rỗng
    for (int id : edgeIds()) {                           // duyệt theo id tăng dần cho ổn định
        const Edge& e = edges.at(id);
        if (!hasNode(e.u) || !hasNode(e.v) || e.u == e.v) continue;
        adjacency[e.u].push_back(id);
        adjacency[e.v].push_back(id);
    }
}

int Graph::V() const { return static_cast<int>(nodes.size()); }
int Graph::E() const { return static_cast<int>(edges.size()); }

std::vector<int> Graph::nodeIds() const {
    std::vector<int> ids;
    ids.reserve(nodes.size());
    for (const auto& kv : nodes) ids.push_back(kv.first);
    std::sort(ids.begin(), ids.end());
    return ids;
}

std::vector<int> Graph::edgeIds() const {
    std::vector<int> ids;
    ids.reserve(edges.size());
    for (const auto& kv : edges) ids.push_back(kv.first);
    std::sort(ids.begin(), ids.end());
    return ids;
}

int Graph::maxNodeId() const {
    int best = -1;
    for (const auto& kv : nodes) best = std::max(best, kv.first);
    return best;
}

void Graph::resetBuildState() {
    for (auto& kv : nodes) kv.second.usedPorts = 0;
    for (auto& kv : edges) {
        kv.second.isBuilt = false;
        kv.second.isBackup = false;
    }
}
