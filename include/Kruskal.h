#ifndef KRUSKAL_H
#define KRUSKAL_H

#include <functional>
#include <vector>
#include "Graph.h"
#include "DSU.h"

// ============================================================
// Kruskal.h - Giai đoạn XÂY HẠ TẦNG (bước 1-4 của quy trình).
//
//   1. Sắp xếp các dây ứng viên theo chi phí lắp đặt tăng dần
//   2. Duyệt từng dây, chỉ chọn nếu:
//        - dây đang hoạt động (isUp) và không vượt độ dài tối đa của loại cáp
//        - không tạo chu trình (DSU)
//        - hai đầu còn cổng trống (usedPorts < totalPorts)
//   3. Chọn xong: usedPorts hai đầu +1, isBuilt = true
//   4. Thêm k dây DỰ PHÒNG để mạng có đường thay thế khi dây đứt
//
// LƯU Ý KHI BẢO VỆ: vì có ràng buộc cổng, kết quả là THAM LAM CÓ RÀNG BUỘC
// (degree-constrained greedy), không còn là MST thuần túy. Nếu không có ràng
// buộc cổng thì đúng là MST tối ưu của Kruskal.
// ============================================================

// Hàm tính chi phí lắp đặt của một dây (đơn vị ĐV).
using CostFn = std::function<double(const Edge&)>;

// Số dây dự phòng mặc định
constexpr int DEFAULT_BACKUP_LINKS = 2;

// Kết quả xây cây khung
struct MSTResult {
    std::vector<int> edgeIds;          // các edge được chọn, theo thứ tự chọn
    double           totalCost = 0.0;  // tổng chi phí lắp đặt các edge đó
    bool             connected = false; // true nếu nối được TẤT CẢ node (đủ V-1 cạnh)
};

// Lý do một edge bị loại (dùng để giải thích trên giao diện và báo cáo)
enum class RejectReason { NONE, LINK_DOWN, TOO_LONG, CYCLE, NO_PORT };
const char* toString(RejectReason r);

struct Rejection {
    int          edgeId;
    RejectReason reason;
};

// So sánh hai edge để sắp xếp: chi phí tăng dần, bằng nhau thì edgeId nhỏ hơn trước.
bool compareEdges(const Edge& a, const Edge& b, const CostFn& cost);

// Kiểm tra một edge có chọn được lúc này không. Trả NONE nếu được, ngược lại là lý do.
// Thứ tự kiểm tra: down -> quá dài -> chu trình -> hết cổng.
RejectReason checkEdge(const Graph& g, const Edge& e, DSU& dsu);

// Xây cây khung. Reset trạng thái xây dựng của g trước, nên gọi lại nhiều lần vẫn đúng.
// Cập nhật g: usedPorts của node, isBuilt của edge.
// rejections (tùy chọn): nếu khác nullptr sẽ nhận danh sách edge bị loại + lý do.
MSTResult buildMST(Graph& g, const CostFn& cost,
                   std::vector<Rejection>* rejections = nullptr);

// Kết quả chọn dây dự phòng
struct BackupResult {
    std::vector<int> edgeIds;              // các edge dự phòng đã chọn
    double           totalCost = 0.0;      // tổng chi phí lắp đặt các dây dự phòng
    int              treeEdges = 0;        // số cạnh trong cây
    int              protectedTreeEdges = 0; // số cạnh cây CÓ đường thay thế sau khi thêm backup
};

// Chọn tối đa k dây dự phòng theo chiến lược tham lam bảo vệ cạnh cây.
// Cập nhật g: isBuilt = true, isBackup = true, usedPorts +1 hai đầu.
BackupResult selectBackupLinks(Graph& g, const MSTResult& mst,
                               int k,
                               const CostFn& cost);

#endif // KRUSKAL_H
