#include "FailureHandler.h"

#include <iostream>
#include <iomanip>

namespace FailureHandler {

namespace {

std::string pathToString(const PathResult& p) {
    if (!p.reachable) return "(khong co duong)";
    std::string s;
    for (size_t i = 0; i < p.nodes.size(); ++i) {
        s += std::to_string(p.nodes[i]);
        if (i + 1 < p.nodes.size()) s += " -> ";
    }
    return s;
}

std::string edgesToString(const std::vector<int>& ids) {
    std::string s;
    for (size_t i = 0; i < ids.size(); ++i) {
        s += "Edge #" + std::to_string(ids[i]);
        if (i + 1 < ids.size()) s += ", ";
    }
    return s;
}

} // namespace

// ------------------------------------------------------------
// A5.1
// ------------------------------------------------------------

bool setLinkState(Graph& g, int edgeId, bool up) {
    if (!g.hasEdge(edgeId)) return false;
    g.getEdge(edgeId).isUp = up;
    return true;
}

bool applyLinkEvent(Graph& g, EventType type, int edgeId) {
    switch (type) {
        case EventType::LINK_DOWN: return setLinkState(g, edgeId, false);
        case EventType::LINK_UP:   return setLinkState(g, edgeId, true);
        default:                   return false;   // LOAD_UPDATE / DELAY_UPDATE không thuộc phần này
    }
}

// ------------------------------------------------------------
// A5.3
// ------------------------------------------------------------

const char* toString(RerouteAction a) {
    switch (a) {
        case RerouteAction::KEEP:     return "KEEP";
        case RerouteAction::REROUTED: return "REROUTED";
        case RerouteAction::DROPPED:  return "DROPPED";
    }
    return "UNKNOWN";
}

bool isPathValid(const Graph& g, const PathResult& path, bool onlyBuilt,
                 std::vector<int>* brokenEdges) {
    if (!path.reachable) return false;

    bool valid = true;
    for (int edgeId : path.edges) {
        bool ok = g.hasEdge(edgeId) &&
                  Connectivity::isEdgeUsable(g.getEdge(edgeId), onlyBuilt);
        if (!ok) {
            valid = false;
            if (brokenEdges) brokenEdges->push_back(edgeId);
            else break;                       // không cần liệt kê thì dừng sớm
        }
    }
    return valid;
}

RerouteResult rerouteIfNeeded(const Graph& g, const PathResult& oldPath,
                              int source, int destination,
                              const WeightFn& weight, bool onlyBuilt) {
    RerouteResult r;

    // 1. Đường cũ còn đi được -> giữ nguyên (dây đứt nằm ngoài đường đi)
    if (isPathValid(g, oldPath, onlyBuilt, &r.brokenEdges)) {
        r.action = RerouteAction::KEEP;
        r.path   = oldPath;
        r.reason = "Duong cu van hop le, khong can dinh tuyen lai";
        return r;
    }

    // 2. Đường cũ hỏng (hoặc trước đó đang không có đường) -> chạy lại Dijkstra
    PathResult newPath = dijkstra(g, source, destination, weight, onlyBuilt);
    if (newPath.reachable) {
        r.action = RerouteAction::REROUTED;
        r.path   = newPath;
        r.reason = r.brokenEdges.empty()
                       ? "Truoc do khong co duong, nay da tim duoc duong moi"
                       : "Duong cu di qua " + edgesToString(r.brokenEdges) +
                         " da hong, chuyen sang duong moi";
        return r;
    }

    // 3. Không còn đường nào -> DROP, giải thích lý do
    r.action = RerouteAction::DROPPED;
    r.path   = PathResult{};
    if (!Connectivity::isConnected(g, source, destination, onlyBuilt)) {
        r.reason = "NO_ROUTE: node " + std::to_string(source) + " va node " +
                   std::to_string(destination) + " da bi tach roi (mat lien thong)";
    } else {
        r.reason = "NO_ROUTE: van lien thong nhung moi duong deu khong hop le voi goi tin "
                   "(vi du vuot MTU ma khong duoc phan manh)";
    }
    return r;
}

// ------------------------------------------------------------
// A5.4
// ------------------------------------------------------------

FailureReport handleFailureEvent(Graph& g, int edgeId, bool up,
                                 const PathResult& oldPath,
                                 int source, int destination,
                                 const WeightFn& weight, bool onlyBuilt) {
    FailureReport rep;
    rep.edgeId  = edgeId;
    rep.oldPath = oldPath;

    // 1. Apply failure + 2. Update state (ghi lại trạng thái trước / sau)
    if (g.hasEdge(edgeId)) {
        const Edge& e    = g.getEdge(edgeId);
        rep.u            = e.u;
        rep.v            = e.v;
        rep.linkUpBefore = e.isUp;
        rep.applied      = setLinkState(g, edgeId, up);
        rep.linkUpAfter  = g.getEdge(edgeId).isUp;
    }

    // 3. Check old path + 4. Rerun Dijkstra nếu cần
    if (rep.applied) {
        rep.reroute = rerouteIfNeeded(g, oldPath, source, destination, weight, onlyBuilt);
    } else {
        rep.reroute.action = RerouteAction::KEEP;
        rep.reroute.path   = oldPath;
        rep.reroute.reason = "Edge #" + std::to_string(edgeId) + " khong ton tai, bo qua su kien";
    }

    // 5. Check connectivity
    rep.sourceDestConnected = Connectivity::isConnected(g, source, destination, onlyBuilt);
    auto comps              = Connectivity::connectedComponents(g, onlyBuilt);
    rep.componentCount      = static_cast<int>(comps.size());
    rep.networkConnected    = rep.componentCount <= 1;
    rep.pairs               = Connectivity::pairConnectivity(g, onlyBuilt);

    // 6. Report: trả về rep, in bằng printFailureReport()
    return rep;
}

void printFailureReport(const FailureReport& rep) {
    std::cout << "\n=== BAO CAO SU CO ===\n";
    if (!rep.applied) {
        std::cout << rep.reroute.reason << "\n";
        return;
    }

    std::cout << "OLD PATH    : " << pathToString(rep.oldPath) << "\n";
    std::cout << (rep.linkUpAfter ? "RECOVERED   : " : "FAILED EDGE : ")
              << "Edge #" << rep.edgeId << " (" << rep.u << " - " << rep.v << ") "
              << (rep.linkUpAfter ? "LINK_UP" : "LINK_DOWN") << "\n";

    switch (rep.reroute.action) {
        case RerouteAction::KEEP:
            std::cout << "NEW PATH    : giu nguyen " << pathToString(rep.reroute.path) << "\n";
            break;
        case RerouteAction::REROUTED:
            std::cout << "NEW PATH    : " << pathToString(rep.reroute.path) << "\n";
            break;
        case RerouteAction::DROPPED:
            std::cout << "NEW PATH    : DROP\n";
            break;
    }
    std::cout << "Ket qua     : " << toString(rep.reroute.action)
              << " - " << rep.reroute.reason << "\n";

    std::cout << "Lien thong  : " << (rep.networkConnected ? "toan mang lien thong"
                                                           : "mang bi chia thanh " +
                                                             std::to_string(rep.componentCount) + " phan")
              << ", con " << std::fixed << std::setprecision(1) << rep.pairs.ratio * 100
              << "% cap node lien lac duoc\n";
}

} // namespace FailureHandler
