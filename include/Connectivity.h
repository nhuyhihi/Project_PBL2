#ifndef CONNECTIVITY_H
#define CONNECTIVITY_H

#include <vector>
#include <string>
#include "Graph.h"

// ============================================================
// Connectivity.h - Tuần 5, Task A5.2 và A5.5 (phần của Quyến).
//
// A5.2: kiểm tra hai node còn liên lạc được với nhau không bằng BFS,
//       bỏ qua các dây đang đứt (isUp = false).
// A5.5: chỉ số chịu lỗi (fault-tolerance) của mạng:
//
//          tỷ lệ cặp node còn liên lạc = số cặp còn liên thông / tổng số cặp
//
//       và so sánh "chỉ có cây khung (MST)" với "MST + cáp dự phòng"
//       khi lần lượt cho từng dây trong cây khung bị đứt.
//
// Module này CHỈ ĐỌC Graph, không sửa trạng thái dây nào (không đổi isUp,
// không đổi isBuilt), nên gọi bao nhiêu lần cũng không làm mạng thay đổi.
// ============================================================
namespace Connectivity {

// ------------------------------------------------------------
// A5.2 - Liên thông
// ------------------------------------------------------------

// Một dây được coi là "đi được" khi:
//   - đang hoạt động (isUp = true), và
//   - nếu onlyBuilt = true: dây đó đã thực sự được lắp (isBuilt = true),
//     giống quy ước onlyBuilt của Dijkstra (chỉ đi trên MST + backup).
bool isEdgeUsable(const Edge& e, bool onlyBuilt);

// Danh sách node đi tới được từ source (gồm cả source), sắp tăng dần.
// Trả rỗng nếu source không tồn tại.
std::vector<int> reachableFrom(const Graph& g, int source, bool onlyBuilt = true);

// true nếu từ source đi tới được destination (BFS, bỏ qua dây đứt).
// source == destination (và node tồn tại) -> true.
// Một trong hai node không tồn tại -> false.
bool isConnected(const Graph& g, int source, int destination, bool onlyBuilt = true);

// true nếu TOÀN BỘ mạng liên thông (mọi node đều tới được nhau).
// Đồ thị rỗng hoặc chỉ 1 node -> true.
bool isNetworkConnected(const Graph& g, bool onlyBuilt = true);

// Chia mạng thành các thành phần liên thông (mỗi phần là danh sách node,
// sắp tăng dần; các phần sắp theo node nhỏ nhất).
std::vector<std::vector<int>> connectedComponents(const Graph& g, bool onlyBuilt = true);

// ------------------------------------------------------------
// A5.5 - Chỉ số chịu lỗi
// ------------------------------------------------------------

struct PairConnectivity {
    long long totalPairs     = 0;   // n*(n-1)/2, n = số node
    long long connectedPairs = 0;   // số cặp còn liên lạc được
    double    ratio          = 1.0; // connectedPairs / totalPairs (mạng < 2 node -> 1.0)
};

// Tỷ lệ cặp node còn liên lạc trên trạng thái HIỆN TẠI của graph.
PairConnectivity pairConnectivity(const Graph& g, bool onlyBuilt = true);

// Tỷ lệ cặp node còn liên lạc khi CHỈ dùng các dây trong usableEdgeIds
// (vẫn bỏ qua dây đang đứt), và giả định thêm các dây trong failedEdgeIds
// bị đứt. Không sửa graph - dùng để "thử" nhiều tình huống đứt cáp.
PairConnectivity pairConnectivityWith(const Graph& g,
                                      const std::vector<int>& usableEdgeIds,
                                      const std::vector<int>& failedEdgeIds = {});

// Một dòng trong bảng so sánh: dây cây khung nào đứt, và tỷ lệ còn lại
// trong 2 cấu hình mạng.
struct FailureComparisonRow {
    int    failedEdgeId    = -1;
    int    u               = -1;
    int    v               = -1;
    double ratioMstOnly    = 0.0;  // chỉ có cây khung
    double ratioWithBackup = 0.0;  // cây khung + cáp dự phòng
};

struct FaultToleranceReport {
    std::vector<FailureComparisonRow> rows;  // mỗi dây cây khung đứt 1 lần
    double avgMstOnly        = 1.0;          // trung bình các dòng
    double avgWithBackup     = 1.0;
    int    survivedMstOnly    = 0;           // số lần mạng VẪN liên thông hoàn toàn
    int    survivedWithBackup = 0;
};

// So sánh MST và MST + backup: lần lượt giả định TỪNG dây trong mstEdgeIds
// bị đứt (mỗi lần 1 dây), tính tỷ lệ cặp node còn liên lạc trong cả 2 cấu hình.
FaultToleranceReport compareSingleFailures(const Graph& g,
                                           const std::vector<int>& mstEdgeIds,
                                           const std::vector<int>& backupEdgeIds);

// In bảng kết quả ra màn hình (để chụp đưa vào báo cáo).
void printFaultToleranceReport(const FaultToleranceReport& report);

} // namespace Connectivity

#endif // CONNECTIVITY_H
