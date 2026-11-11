#ifndef KRUSKAL_H
#define KRUSKAL_H

#include <functional>
#include <vector>
#include "Graph.h"
#include "DSU.h"

// Quy hoạch hạ tầng cáp tối ưu theo Kruskal có ràng buộc và chọn cáp dự phòng
using CostFn = std::function<double(const Edge&)>;

constexpr int DEFAULT_BACKUP_LINKS = 2;

struct MSTResult {
    std::vector<int> edgeIds;
    double           totalCost = 0.0;
    bool             connected = false;
};

enum class RejectReason { NONE, LINK_DOWN, TOO_LONG, CYCLE, NO_PORT };
const char* toString(RejectReason r);

struct Rejection {
    int          edgeId;
    RejectReason reason;
};

// Sắp xếp cạnh theo chi phí tăng dần (ưu tiên edgeId nhỏ hơn khi bằng nhau)
bool compareEdges(const Edge& a, const Edge& b, const CostFn& cost);

// Kiểm tra một cạnh theo 4 ràng buộc: hoạt động, chiều dài, chu trình, số cổng
RejectReason checkEdge(const Graph& g, const Edge& e, DSU& dsu);

// Xây dựng cây khung tối ưu có ràng buộc và cập nhật isBuilt, usedPorts trên đồ thị
MSTResult buildMST(Graph& g, const CostFn& cost,
                   std::vector<Rejection>* rejections = nullptr);

struct BackupResult {
    std::vector<int> edgeIds;
    double           totalCost = 0.0;
    int              treeEdges = 0;
    int              protectedTreeEdges = 0;
};

// Chọn tối đa k tuyến cáp dự phòng ngoài cây nhằm bảo vệ các cạnh cây khung
BackupResult selectBackupLinks(Graph& g, const MSTResult& mst,
                               int k,
                               const CostFn& cost);

#endif // KRUSKAL_H
