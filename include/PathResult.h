#ifndef PATH_RESULT_H
#define PATH_RESULT_H

#include <vector>

// ============================================================
// PathResult - kết quả của một lần tìm đường (Dijkstra) từ node
// nguồn (source) đến node đích (destination).
// ============================================================
struct PathResult {
    bool reachable = false;     // true nếu tới được destination

    // Đường đi, theo thứ tự source -> destination (đã đảo chiều sau khi dựng lại)
    std::vector<int> nodes;     // danh sách nodeId trên đường đi
    std::vector<int> edges;     // danh sách edgeId trên đường đi (edges.size() == nodes.size() - 1)

    double totalCost = 0.0;     // tổng trọng số (weight) của đường đi
    int    hopCount   = 0;      // số cạnh đã đi qua, luôn bằng edges.size()
};

#endif // PATH_RESULT_H
