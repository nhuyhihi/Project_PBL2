#include "Graph.h"

Graph::Graph() {
    // Stub
}

Graph::~Graph() {
    // Stub
}

void Graph::addNode(const Node& node) {
    // Để bạn Quyến thực hiện chi tiết
    nodes[node.id] = node;
}

void Graph::addEdge(const Edge& edge) {
    // Để bạn Quyến thực hiện chi tiết
    edges[edge.id] = edge;
}

Node Graph::getNode(int nodeId) const {
    auto it = nodes.find(nodeId);
    if (it != nodes.end()) {
        return it->second;
    }
    throw std::runtime_error("Node not found!");
}

Edge Graph::getEdge(int edgeId) const {
    auto it = edges.find(edgeId);
    if (it != edges.end()) {
        return it->second;
    }
    throw std::runtime_error("Edge not found!");
}

std::vector<int> Graph::getNeighbors(int nodeId) const {
    auto it = adjacency.find(nodeId);
    if (it != adjacency.end()) {
        return it->second;
    }
    return {};
}

int Graph::otherEndpoint(int nodeId, int edgeId) const {
    // Để bạn Quyến thực hiện chi tiết
    return -1;
}

void Graph::buildAdjacency() {
    // Để bạn Quyến thực hiện chi tiết
}

int Graph::V() const {
    return nodes.size();
}

int Graph::E() const {
    return edges.size();
}
