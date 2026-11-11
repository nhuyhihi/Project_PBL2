#ifndef PATH_RESULT_H
#define PATH_RESULT_H

#include <vector>

// Kết quả tìm đường đi ngắn nhất từ source đến destination
struct PathResult {
    bool reachable = false;
    std::vector<int> nodes;
    std::vector<int> edges;
    double totalCost = 0.0;
    int    hopCount   = 0;
};

#endif // PATH_RESULT_H
