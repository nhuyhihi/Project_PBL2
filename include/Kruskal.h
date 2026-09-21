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
// Kruskal không tự biết công thức, để nhóm đổi được công thức mà không sửa thuật toán.
// Khi CostModel (bạn B) xong thì truyền hàm calculateCablingCost vào đây.
using CostFn = std::function<double(const Edge&)>;

// Công thức tạm dùng khi chưa có CostModel:
//   length * unitPrice * terrainFactor + equipmentCost + maintenanceCost
// (chưa có installationCost vì struct Edge hiện tại không có field này)
double defaultCablingCost(const Edge& e);

// Số dây dự phòng mặc định
constexpr int DEFAULT_BACKUP_LINKS = 2;

// Kết quả xây cây khung (khớp task board mục 3.6)
struct MSTResult {
    std::vector<int> edgeIds;          // các edge được chọn, theo thứ tự chọn
    double           totalCost = 0.0;  // tổng chi phí lắp đặt các edge đó (không tính thiết bị)
    bool             connected = false; // true nếu nối được TẤT CẢ node (đủ V-1 cạnh)
};

// Lý do một edge bị loại (dùng để giải thích trong báo cáo)
enum class RejectReason { NONE, LINK_DOWN, TOO_LONG, CYCLE, NO_PORT };
const char* toString(RejectReason r);

struct Rejection {
    int          edgeId;
    RejectReason reason;
};

// So sánh hai edge để sắp xếp: chi phí tăng dần, bằng nhau thì edgeId nhỏ hơn trước.
// Tie-breaker theo id giúp hai lần chạy cùng dữ liệu cho CÙNG kết quả.
bool compareEdges(const Edge& a, const Edge& b, const CostFn& cost = defaultCablingCost);

// Kiểm tra một edge có chọn được lúc này không. Trả NONE nếu được, ngược lại là lý do.
// Thứ tự kiểm tra: down -> quá dài -> chu trình -> hết cổng.
// (dsu không const vì find() có path compression)
RejectReason checkEdge(const Graph& g, const Edge& e, DSU& dsu);
inline bool canSelectEdge(const Graph& g, const Edge& e, DSU& dsu) {
    return checkEdge(g, e, dsu) == RejectReason::NONE;
}

// Xây cây khung. Reset trạng thái xây dựng của g trước, nên gọi lại nhiều lần vẫn đúng.
// Cập nhật g: usedPorts của node, isBuilt của edge.
// rejections (tùy chọn): nếu khác nullptr sẽ nhận danh sách edge bị loại + lý do.
MSTResult buildMST(Graph& g, const CostFn& cost = defaultCablingCost,
                   std::vector<Rejection>* rejections = nullptr);

// Kết quả chọn dây dự phòng
struct BackupResult {
    std::vector<int> edgeIds;              // các edge dự phòng đã chọn
    double           totalCost = 0.0;      // tổng chi phí lắp đặt các dây dự phòng
    int              treeEdges = 0;        // số cạnh trong cây
    int              protectedTreeEdges = 0; // số cạnh cây CÓ đường thay thế sau khi thêm backup
};

// Chọn tối đa k dây dự phòng. Chiến lược (tham lam theo độ bảo vệ):
//   - Xét các edge chưa xây, hợp lệ, còn cổng, theo thứ tự compareEdges (rẻ trước)
//   - Một edge nối u-v "bảo vệ" các cạnh cây nằm trên đường u->v trong cây:
//     nếu một cạnh đó đứt, edge này nối lại hai phía
//   - Chỉ chọn edge bảo vệ THÊM ít nhất một cạnh cây chưa được bảo vệ
//     (không chọn dây tốn tiền mà không thêm lợi gì)
// Có thể trả về ít hơn k edge nếu không còn edge nào thêm được sự bảo vệ.
// Cạnh cây không thể bảo vệ (vd host chỉ có 1 cổng) không tính vào protectedTreeEdges.
// Cập nhật g: isBuilt = true, isBackup = true, usedPorts +1 hai đầu.
BackupResult selectBackupLinks(Graph& g, const MSTResult& mst,
                               int k = DEFAULT_BACKUP_LINKS,
                               const CostFn& cost = defaultCablingCost);

// Tổng chi phí lắp đặt của một tập edge
double sumCablingCost(const Graph& g, const std::vector<int>& edgeIds,
                      const CostFn& cost = defaultCablingCost);

#endif // KRUSKAL_H
