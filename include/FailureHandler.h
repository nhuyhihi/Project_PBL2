#ifndef FAILURE_HANDLER_H
#define FAILURE_HANDLER_H

#include <string>
#include <vector>
#include "Graph.h"
#include "Types.h"
#include "PathResult.h"
#include "Dijkstra.h"        // WeightFn
#include "Connectivity.h"

// Xử lý sự cố đứt/phục hồi liên kết, tái định tuyến và kiểm tra liên thông
namespace FailureHandler {

// Cập nhật trạng thái đóng/ngắt của tuyến cáp
bool setLinkState(Graph& g, int edgeId, bool up);
bool applyLinkEvent(Graph& g, EventType type, int edgeId);

enum class RerouteAction {
    KEEP,
    REROUTED,
    DROPPED
};
const char* toString(RerouteAction a);

struct RerouteResult {
    RerouteAction    action = RerouteAction::KEEP;
    PathResult       path;
    std::vector<int> brokenEdges;
    std::string      reason;
};

// Kiểm tra đường đi hiện tại còn dùng được không
bool isPathValid(const Graph& g, const PathResult& path, bool onlyBuilt = true,
                 std::vector<int>* brokenEdges = nullptr);

// Giữ đường cũ nếu còn hợp lệ, ngược lại chạy lại Dijkstra để chuyển hướng hoặc DROP
RerouteResult rerouteIfNeeded(const Graph& g, const PathResult& oldPath,
                              int source, int destination,
                              const WeightFn& weight, bool onlyBuilt = true);

// Báo cáo tổng hợp sau một sự kiện thay đổi trạng thái cáp
struct FailureReport {
    int  edgeId       = -1;
    int  u            = -1;
    int  v            = -1;
    bool applied      = false;
    bool linkUpBefore = true;
    bool linkUpAfter  = true;

    PathResult    oldPath;
    RerouteResult reroute;

    bool sourceDestConnected = false;
    bool networkConnected    = false;
    int  componentCount      = 0;
    Connectivity::PairConnectivity pairs;
};

// Thực hiện trọn vẹn quy trình xử lý sự cố và đánh giá liên thông
FailureReport handleFailureEvent(Graph& g, int edgeId, bool up,
                                 const PathResult& oldPath,
                                 int source, int destination,
                                 const WeightFn& weight, bool onlyBuilt = true);

void printFailureReport(const FailureReport& report);

} // namespace FailureHandler

#endif // FAILURE_HANDLER_H
