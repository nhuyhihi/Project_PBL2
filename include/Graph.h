#ifndef GRAPH_H
#define GRAPH_H

#include <vector>
#include <unordered_map>
#include <stdexcept>
#include "Node.h"
#include "Edge.h"

// Đồ thị vô hướng biểu diễn hạ tầng mạng qua danh sách kề theo edgeId
class Graph {
private:
    std::unordered_map<int, Node> nodes;
    std::unordered_map<int, Edge> edges;
    std::unordered_map<int, std::vector<int>> adjacency; // nodeId -> danh sách edgeId kề

public:
    Graph();
    ~Graph();

    bool addNode(const Node& node);
    bool addEdge(const Edge& edge);

    bool hasNode(int nodeId) const;
    bool hasEdge(int edgeId) const;

    // Truy xuất Node/Edge theo id (trả tham chiếu để cập nhật trạng thái khi chạy thuật toán)
    Node&       getNode(int nodeId);
    const Node& getNode(int nodeId) const;
    Edge&       getEdge(int edgeId);
    const Edge& getEdge(int edgeId) const;

    const std::vector<int>& getNeighbors(int nodeId) const;
    int otherEndpoint(int nodeId, int edgeId) const;

    // Dựng lại danh sách kề theo thứ tự edgeId tăng dần
    void buildAdjacency();

    int V() const;
    int E() const;

    // Danh sách id tăng dần để đảm bảo thứ tự duyệt ổn định
    std::vector<int> nodeIds() const;
    std::vector<int> edgeIds() const;

    int maxNodeId() const;
    void resetBuildState();
};

#endif // GRAPH_H
