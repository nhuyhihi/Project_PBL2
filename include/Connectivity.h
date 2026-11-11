#ifndef CONNECTIVITY_H
#define CONNECTIVITY_H

#include <vector>
#include <string>
#include "Graph.h"

// Kiểm tra tính liên thông (BFS) và chỉ số chịu lỗi trên đồ thị (chỉ đọc)
namespace Connectivity {

// Kiểm tra một dây có đi được không (đang bật và thỏa điều kiện onlyBuilt)
bool isEdgeUsable(const Edge& e, bool onlyBuilt);

// Danh sách node đi tới được từ source (sắp tăng dần)
std::vector<int> reachableFrom(const Graph& g, int source, bool onlyBuilt = true);

// Kiểm tra source có tới được destination không
bool isConnected(const Graph& g, int source, int destination, bool onlyBuilt = true);

// Kiểm tra toàn bộ đồ thị có liên thông không
bool isNetworkConnected(const Graph& g, bool onlyBuilt = true);

// Liệt kê các thành phần liên thông (sắp tăng dần)
std::vector<std::vector<int>> connectedComponents(const Graph& g, bool onlyBuilt = true);

// Thống kê tỷ lệ cặp nút còn duy trì liên lạc
struct PairConnectivity {
    long long totalPairs     = 0;
    long long connectedPairs = 0;
    double    ratio          = 1.0;
};

PairConnectivity pairConnectivity(const Graph& g, bool onlyBuilt = true);

// Tính tỷ lệ cặp nút liên lạc khi chỉ dùng usableEdgeIds và loại failedEdgeIds
PairConnectivity pairConnectivityWith(const Graph& g,
                                      const std::vector<int>& usableEdgeIds,
                                      const std::vector<int>& failedEdgeIds = {});

// Kết quả so sánh khả năng chịu lỗi khi từng cạnh cây khung bị đứt
struct FailureComparisonRow {
    int    failedEdgeId    = -1;
    int    u               = -1;
    int    v               = -1;
    double ratioMstOnly    = 0.0;
    double ratioWithBackup = 0.0;
};

struct FaultToleranceReport {
    std::vector<FailureComparisonRow> rows;
    double avgMstOnly        = 1.0;
    double avgWithBackup     = 1.0;
    int    survivedMstOnly    = 0;
    int    survivedWithBackup = 0;
};

// So sánh độ bền vững giữa chỉ MST và MST + cáp dự phòng
FaultToleranceReport compareSingleFailures(const Graph& g,
                                           const std::vector<int>& mstEdgeIds,
                                           const std::vector<int>& backupEdgeIds);

void printFaultToleranceReport(const FaultToleranceReport& report);

} // namespace Connectivity

#endif // CONNECTIVITY_H
