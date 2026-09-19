#ifndef NETWORK_GRAPH_H
#define NETWORK_GRAPH_H

#include <vector>
#include <queue>
#include <iostream>
#include <climits>
#include <string>

using namespace std;

struct Edge {
    int u, v;
    long long w; 

    bool operator<(const Edge &other) const;
};

class DSU {
private:
    vector<int> parent, rankValue;

public:
    DSU(int n);
    int find(int x);
    bool unite(int a, int b);
};


class NetworkGraph {
private:
    int numNodes;
    int numEdges;
    bool isDirected;

    // Danh sách kề cho Dijkstra (lưu đỉnh kề và trọng số)
    vector<vector<pair<int, long long>>> adj;

    // Danh sách cạnh cho Kruskal
    vector<Edge> edgeList;

public:
    // Constructor khởi tạo đồ thị
    NetworkGraph(int n, bool directed = false);

    // Thêm một liên kết mạng (cạnh)
    void addEdge(int u, int v, long long w);

    // Tìm đường đi ngắn nhất từ đỉnh source tới đích destination
    void dijkstra(int source, int destination);

    // Hỗ trợ in đường đi
    void printPath(int source, int destination, const vector<int>& trace);

    // Tìm cây khung nhỏ nhất (MST)
    vector<Edge> kruskal();

    // Đọc cấu hình mạng từ file
    void loadFromFile(const string& filename);
};

#endif // NETWORK_GRAPH_H
