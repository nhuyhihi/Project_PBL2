#include "Connectivity.h"

#include <functional>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <iostream>
#include <iomanip>

namespace Connectivity {

namespace {

using EdgeFilter = std::function<bool(const Edge&)>;

// BFS dùng chung cho mọi hàm trong file: chỉ đi qua các dây mà filter cho phép.
// visited được đánh dấu cho mọi node tới được; trả về danh sách node đó (sắp tăng).
std::vector<int> bfs(const Graph& g, int source, const EdgeFilter& canUse,
                     std::unordered_set<int>& visited) {
    std::vector<int> reached;
    if (!g.hasNode(source) || visited.count(source)) return reached;

    std::queue<int> q;
    q.push(source);
    visited.insert(source);

    while (!q.empty()) {
        int u = q.front();
        q.pop();
        reached.push_back(u);

        for (int edgeId : g.getNeighbors(u)) {
            const Edge& e = g.getEdge(edgeId);
            if (!canUse(e)) continue;               // dây đứt / chưa lắp -> bỏ qua
            int v = g.otherEndpoint(u, edgeId);
            if (v < 0 || visited.count(v)) continue;
            visited.insert(v);
            q.push(v);
        }
    }

    std::sort(reached.begin(), reached.end());
    return reached;
}

std::vector<std::vector<int>> componentsWith(const Graph& g, const EdgeFilter& canUse) {
    std::vector<std::vector<int>> comps;
    std::unordered_set<int> visited;
    for (int id : g.nodeIds()) {                    // nodeIds() đã sắp tăng -> kết quả ổn định
        if (visited.count(id)) continue;
        comps.push_back(bfs(g, id, canUse, visited));
    }
    return comps;
}

PairConnectivity pairsFromComponents(const std::vector<std::vector<int>>& comps, int nodeCount) {
    PairConnectivity r;
    long long n = nodeCount;
    r.totalPairs = n * (n - 1) / 2;
    for (const auto& c : comps) {
        long long s = static_cast<long long>(c.size());
        r.connectedPairs += s * (s - 1) / 2;        // trong 1 thành phần, mọi cặp đều liên lạc được
    }
    r.ratio = (r.totalPairs == 0) ? 1.0
                                  : static_cast<double>(r.connectedPairs) / r.totalPairs;
    return r;
}

} // namespace

// ------------------------------------------------------------
// A5.2
// ------------------------------------------------------------

bool isEdgeUsable(const Edge& e, bool onlyBuilt) {
    if (!e.isUp) return false;
    if (onlyBuilt && !e.isBuilt) return false;
    return true;
}

std::vector<int> reachableFrom(const Graph& g, int source, bool onlyBuilt) {
    std::unordered_set<int> visited;
    return bfs(g, source,
               [onlyBuilt](const Edge& e) { return isEdgeUsable(e, onlyBuilt); },
               visited);
}

bool isConnected(const Graph& g, int source, int destination, bool onlyBuilt) {
    if (!g.hasNode(source) || !g.hasNode(destination)) return false;
    if (source == destination) return true;
    std::vector<int> reached = reachableFrom(g, source, onlyBuilt);
    return std::binary_search(reached.begin(), reached.end(), destination);
}

bool isNetworkConnected(const Graph& g, bool onlyBuilt) {
    return connectedComponents(g, onlyBuilt).size() <= 1;
}

std::vector<std::vector<int>> connectedComponents(const Graph& g, bool onlyBuilt) {
    return componentsWith(g, [onlyBuilt](const Edge& e) { return isEdgeUsable(e, onlyBuilt); });
}

// ------------------------------------------------------------
// A5.5
// ------------------------------------------------------------

PairConnectivity pairConnectivity(const Graph& g, bool onlyBuilt) {
    return pairsFromComponents(connectedComponents(g, onlyBuilt), g.V());
}

PairConnectivity pairConnectivityWith(const Graph& g,
                                      const std::vector<int>& usableEdgeIds,
                                      const std::vector<int>& failedEdgeIds) {
    std::unordered_set<int> allowed(usableEdgeIds.begin(), usableEdgeIds.end());
    for (int id : failedEdgeIds) allowed.erase(id);

    auto comps = componentsWith(g, [&allowed](const Edge& e) {
        return e.isUp && allowed.count(e.id) > 0;
    });
    return pairsFromComponents(comps, g.V());
}

FaultToleranceReport compareSingleFailures(const Graph& g,
                                           const std::vector<int>& mstEdgeIds,
                                           const std::vector<int>& backupEdgeIds) {
    FaultToleranceReport report;

    std::vector<int> withBackup = mstEdgeIds;
    withBackup.insert(withBackup.end(), backupEdgeIds.begin(), backupEdgeIds.end());

    double sumMst = 0.0, sumBackup = 0.0;
    for (int failed : mstEdgeIds) {
        if (!g.hasEdge(failed)) continue;
        const Edge& e = g.getEdge(failed);

        PairConnectivity a = pairConnectivityWith(g, mstEdgeIds, {failed});
        PairConnectivity b = pairConnectivityWith(g, withBackup, {failed});

        FailureComparisonRow row;
        row.failedEdgeId    = failed;
        row.u               = e.u;
        row.v               = e.v;
        row.ratioMstOnly    = a.ratio;
        row.ratioWithBackup = b.ratio;
        report.rows.push_back(row);

        sumMst    += a.ratio;
        sumBackup += b.ratio;
        if (a.connectedPairs == a.totalPairs) report.survivedMstOnly++;
        if (b.connectedPairs == b.totalPairs) report.survivedWithBackup++;
    }

    if (!report.rows.empty()) {
        report.avgMstOnly    = sumMst / report.rows.size();
        report.avgWithBackup = sumBackup / report.rows.size();
    }
    return report;
}

void printFaultToleranceReport(const FaultToleranceReport& report) {
    std::cout << "\n=== CHI SO CHIU LOI: MST vs MST + BACKUP (dut tung day trong cay khung) ===\n";
    std::cout << std::left << std::setw(12) << "Day dut"
              << std::setw(12) << "Noi"
              << std::right << std::setw(14) << "Chi MST"
              << std::setw(18) << "MST + Backup" << "\n";
    std::cout << std::string(56, '-') << "\n";

    std::cout << std::fixed << std::setprecision(1);
    for (const auto& r : report.rows) {
        std::string ends = std::to_string(r.u) + " - " + std::to_string(r.v);
        std::cout << std::left << std::setw(12) << ("Edge #" + std::to_string(r.failedEdgeId))
                  << std::setw(12) << ends
                  << std::right << std::setw(13) << r.ratioMstOnly * 100 << "%"
                  << std::setw(17) << r.ratioWithBackup * 100 << "%" << "\n";
    }

    std::cout << std::string(56, '-') << "\n";
    std::cout << std::left << std::setw(24) << "Trung binh"
              << std::right << std::setw(13) << report.avgMstOnly * 100 << "%"
              << std::setw(17) << report.avgWithBackup * 100 << "%" << "\n";
    std::string total     = std::to_string(report.rows.size());
    std::string mstOk     = std::to_string(report.survivedMstOnly) + "/" + total;
    std::string backupOk  = std::to_string(report.survivedWithBackup) + "/" + total;
    std::cout << std::left << std::setw(24) << "Van lien thong hoan toan"
              << std::right << std::setw(14) << mstOk
              << std::setw(18) << backupOk << "\n";
}

} // namespace Connectivity
