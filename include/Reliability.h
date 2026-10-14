#ifndef RELIABILITY_H
#define RELIABILITY_H

#include "Graph.h"
#include <string>
#include <vector>

// Kết quả phân tích cáp dự phòng cứu một cạnh cây khung (Tree Edge)
struct BackupCandidate {
    int backupEdgeId;      // ID của dây dự phòng
    double cost;           // Chi phí lắp đặt của dây này
    std::string reason;    // Lý do chọn / phân tích liên thông
};

namespace Reliability {

    // Task B5.2: Phân tích và tìm cáp dự phòng thay thế cho một cạnh trong cây khung (Tree Edge)
    // - Tách graph (hoặc cây MST) khi giả định tree edge bị đứt
    // - Xác định hai thành phần liên thông bị tách ra
    // - Tìm các cạnh dự phòng đã xây (hoặc ứng viên ngoài cây) kết nối hai phía
    // - Trả về cạnh backup tối ưu nhất (hoặc backupEdgeId = -1 nếu không có đường thay thế)
    BackupCandidate findBackupForTreeEdge(const Graph& graph, 
                                          const std::vector<int>& mstTreeEdges, 
                                          int failedTreeEdgeId);

    // Tìm tất cả các cạnh dự phòng có thể thay thế khi một tree edge cụ thể bị đứt
    std::vector<BackupCandidate> findAllBackupsForTreeEdge(const Graph& graph,
                                                          const std::vector<int>& mstTreeEdges,
                                                          int failedTreeEdgeId);

} // namespace Reliability

#endif // RELIABILITY_H
