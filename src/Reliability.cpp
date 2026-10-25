#include "Reliability.h"
#include "Validation.h"
#include <queue>
#include <unordered_set>
#include <algorithm>

namespace Reliability {

    // Helper: Tìm các node thuộc thành phần liên thông chứa startNode trong cây MST khi bỏ qua failedEdgeId
    static std::unordered_set<int> getComponentNodes(const Graph& graph, 
                                                    const std::vector<int>& mstTreeEdges, 
                                                    int failedEdgeId, 
                                                    int startNode) {
        std::unordered_set<int> comp;
        std::queue<int> q;

        comp.insert(startNode);
        q.push(startNode);

        while (!q.empty()) {
            int u = q.front();
            q.pop();

            // Duyệt qua các lân cận nhưng CHỈ đi qua các cạnh thuộc MST (trừ cạnh bị đứt failedEdgeId)
            for (int edgeId : graph.getNeighbors(u)) {
                if (edgeId == failedEdgeId) continue;

                // Kiểm tra xem edgeId này có nằm trong cây MST không
                bool inMST = false;
                for (int tId : mstTreeEdges) {
                    if (tId == edgeId) {
                        inMST = true;
                        break;
                    }
                }
                if (!inMST) continue;

                int v = graph.otherEndpoint(u, edgeId);
                if (comp.find(v) == comp.end()) {
                    comp.insert(v);
                    q.push(v);
                }
            }
        }
        return comp;
    }

    std::vector<BackupCandidate> findAllBackupsForTreeEdge(const Graph& graph,
                                                          const std::vector<int>& mstTreeEdges,
                                                          int failedTreeEdgeId,
                                                          const CostFn& cost) {
        std::vector<BackupCandidate> candidates;

        // Kiểm tra failedTreeEdgeId có thực sự nằm trong MST không
        bool found = false;
        for (int tId : mstTreeEdges) {
            if (tId == failedTreeEdgeId) {
                found = true;
                break;
            }
        }
        if (!found) return candidates;

        const Edge& failedEdge = graph.getEdge(failedTreeEdgeId);

        // 1. Tách cây khi remove failedTreeEdge: Tìm 2 thành phần liên thông
        // Nhánh A chứa failedEdge.u, Nhánh B chứa failedEdge.v
        std::unordered_set<int> compA = getComponentNodes(graph, mstTreeEdges, failedTreeEdgeId, failedEdge.u);
        std::unordered_set<int> compB = getComponentNodes(graph, mstTreeEdges, failedTreeEdgeId, failedEdge.v);

        // 2. Tìm tất cả các cạnh trong đồ thị (không nằm trong MST) nối giữa 1 node thuộc compA và 1 node thuộc compB
        for (int edgeId : graph.edgeIds()) {
            if (edgeId == failedTreeEdgeId) continue;

            // Bỏ qua các cạnh khác của MST
            bool inMST = false;
            for (int tId : mstTreeEdges) {
                if (tId == edgeId) {
                    inMST = true;
                    break;
                }
            }
            if (inMST) continue;

            const Edge& e = graph.getEdge(edgeId);

            // Kiểm tra tính khả thi: cáp phải còn hoạt động (isUp) và thỏa mãn độ dài vật lý
            if (!e.isUp || !Validation::isStaticFeasible(e)) continue;

            // Cạnh này có nối giữa hai phía compA và compB không?
            bool connectsAB = (compA.count(e.u) && compB.count(e.v)) ||
                             (compA.count(e.v) && compB.count(e.u));

            if (connectsAB) {
                BackupCandidate cand;
                cand.backupEdgeId = edgeId;
                cand.cost = cost ? cost(e) : 0.0;
                
                if (e.isBuilt && e.isBackup) {
                    cand.reason = "Cap du phong da duoc xay dung (isBuilt=true, isBackup=true) noi hai nhanh " 
                                + std::to_string(e.u) + " va " + std::to_string(e.v);
                } else if (e.isBuilt) {
                    cand.reason = "Cap mang da xay dung noi giua " + std::to_string(e.u) + " va " + std::to_string(e.v);
                } else {
                    cand.reason = "Cap ung vien tiem nang ngoai cay noi giua " + std::to_string(e.u) + " va " + std::to_string(e.v);
                }

                candidates.push_back(cand);
            }
        }

        // Sắp xếp ưu tiên: cáp đã xây (isBuilt) lên trước, sau đó tới chi phí rẻ hơn
        std::sort(candidates.begin(), candidates.end(), [&](const BackupCandidate& a, const BackupCandidate& b) {
            const Edge& ea = graph.getEdge(a.backupEdgeId);
            const Edge& eb = graph.getEdge(b.backupEdgeId);
            if (ea.isBuilt != eb.isBuilt) return ea.isBuilt > eb.isBuilt;
            if (a.cost != b.cost) return a.cost < b.cost;
            return a.backupEdgeId < b.backupEdgeId;
        });

        return candidates;
    }

    BackupCandidate findBackupForTreeEdge(const Graph& graph, 
                                          const std::vector<int>& mstTreeEdges, 
                                          int failedTreeEdgeId,
                                          const CostFn& cost) {
        std::vector<BackupCandidate> all = findAllBackupsForTreeEdge(graph, mstTreeEdges, failedTreeEdgeId, cost);
        if (all.empty()) {
            return {-1, 0.0, "Khong tim thay bat ky cap du phong nao co the noi lai hai phan bi chia cat (Canh cau nguy cap / Critical Bridge)"};
        }
        return all[0];
    }

} // namespace Reliability
