#ifndef FAILURE_HANDLER_H
#define FAILURE_HANDLER_H

#include <string>
#include <vector>
#include "Graph.h"
#include "Types.h"
#include "PathResult.h"
#include "Dijkstra.h"        // WeightFn
#include "Connectivity.h"

// ============================================================
// FailureHandler.h - Tuần 5, Task A5.1, A5.3, A5.4 (phần của Quyến).
//
//   LINK_DOWN
//      ↓
//   Kiểm tra path cũ
//      ↓
//   Reroute nếu cần (chạy lại Dijkstra)
//      ↓
//   Không có route → DROP + lý do
//
// Cách tính chi phí đường đi KHÔNG cố định ở đây: hàm nhận WeightFn giống
// Dijkstra (truyền lengthWeight, hoặc chi phí QoS của Tuần 4), nên module
// này không phụ thuộc QoS/MTU/tắc nghẽn.
// ============================================================
namespace FailureHandler {

// ------------------------------------------------------------
// A5.1 - Trạng thái dây
// ------------------------------------------------------------

// Đặt trạng thái 1 dây: up = false (LINK_DOWN) -> isUp = false,
//                        up = true  (LINK_UP)   -> isUp = true.
// Trả false nếu edgeId không tồn tại (không làm gì).
bool setLinkState(Graph& g, int edgeId, bool up);

// Tiện dụng cho sự kiện trong kịch bản: chỉ nhận LINK_DOWN / LINK_UP.
// Loại sự kiện khác hoặc edgeId không tồn tại -> trả false, không làm gì.
bool applyLinkEvent(Graph& g, EventType type, int edgeId);

// ------------------------------------------------------------
// A5.3 - Reroute
// ------------------------------------------------------------

enum class RerouteAction {
    KEEP,       // đường cũ vẫn dùng được -> giữ nguyên
    REROUTED,   // đường cũ hỏng -> Dijkstra tìm được đường mới
    DROPPED     // đường cũ hỏng và không còn đường nào -> hủy gói tin
};
const char* toString(RerouteAction a);

struct RerouteResult {
    RerouteAction    action = RerouteAction::KEEP;
    PathResult       path;          // đường đang dùng sau khi xử lý (rỗng nếu DROPPED)
    std::vector<int> brokenEdges;   // các dây trên đường cũ không còn đi được
    std::string      reason;        // giải thích (đặc biệt khi DROPPED)
};

// Đường đi còn hợp lệ không: phải reachable, và MỌI dây trên đường đi
// đều còn tồn tại, đang hoạt động (isUp), và (nếu onlyBuilt) đã được lắp.
// brokenEdges (nếu khác nullptr) nhận danh sách dây hỏng trên đường đi.
bool isPathValid(const Graph& g, const PathResult& path, bool onlyBuilt = true,
                 std::vector<int>* brokenEdges = nullptr);

// old path -> check edge status -> còn hợp lệ? YES: giữ / NO: chạy Dijkstra lại.
// Không còn đường -> DROPPED, reason phân biệt 2 trường hợp:
//   - source và destination đã bị tách rời (mất liên thông), hoặc
//   - vẫn liên thông nhưng mọi đường đều có chi phí vô cực với gói tin này
//     (ví dụ gói tin không được phân mảnh mà vượt MTU).
RerouteResult rerouteIfNeeded(const Graph& g, const PathResult& oldPath,
                              int source, int destination,
                              const WeightFn& weight, bool onlyBuilt = true);

// ------------------------------------------------------------
// A5.4 - Xử lý trọn vẹn 1 sự kiện đứt / phục hồi cáp
// ------------------------------------------------------------

struct FailureReport {
    int  edgeId       = -1;
    int  u            = -1;
    int  v            = -1;
    bool applied      = false;   // false nếu edgeId không tồn tại
    bool linkUpBefore = true;    // trạng thái dây trước sự kiện
    bool linkUpAfter  = true;    // trạng thái dây sau sự kiện

    PathResult    oldPath;
    RerouteResult reroute;

    bool sourceDestConnected = false;  // source và destination còn liên lạc?
    bool networkConnected    = false;  // toàn mạng còn liên thông?
    int  componentCount      = 0;      // mạng bị chia thành mấy phần
    Connectivity::PairConnectivity pairs;  // tỷ lệ cặp node còn liên lạc
};

// Các bước: Apply failure -> Update state -> Check old path ->
//           Rerun Dijkstra (nếu cần) -> Check connectivity -> Report.
FailureReport handleFailureEvent(Graph& g, int edgeId, bool up,
                                 const PathResult& oldPath,
                                 int source, int destination,
                                 const WeightFn& weight, bool onlyBuilt = true);

// In báo cáo dạng: OLD PATH -> FAILED EDGE -> NEW PATH / DROP + lý do.
void printFailureReport(const FailureReport& report);

} // namespace FailureHandler

#endif // FAILURE_HANDLER_H
