#ifndef GRAPH_H
#define GRAPH_H

#include <vector>
#include <unordered_map>
#include <stdexcept>
#include "Node.h"
#include "Edge.h"

class Graph {
private:
    std::unordered_map<int, Node> nodes;
    std::unordered_map<int, Edge> edges;
    
    // adjacency[nodeId] = danh sách các edgeId kề
    std::unordered_map<int, std::vector<int>> adjacency;

public:
    Graph();
    ~Graph();

    // Thêm Node vào đồ thị
    void addNode(const Node& node);
    
    // Thêm Edge vào đồ thị
    void addEdge(const Edge& edge);
    
    // Lấy thông tin Node
    Node getNode(int nodeId) const;
    
    // Lấy thông tin Edge
    Edge getEdge(int edgeId) const;
    
    // Lấy danh sách các edgeId kề với một nodeId
    std::vector<int> getNeighbors(int nodeId) const;
    
    // Trả về nodeId ở đầu kia của cạnh khi biết một nodeId và edgeId
    int otherEndpoint(int nodeId, int edgeId) const;

    // Xây dựng hoặc cập nhật lại adjacency list
    void buildAdjacency();

    // Số lượng node
    int V() const;
    
    // Số lượng edge
    int E() const;
};

#endif // GRAPH_H
