#ifndef REPORTER_H
#define REPORTER_H

#include "Graph.h"
#include "Packet.h"
#include "Dijkstra.h"

#include "CostModel.h"

namespace Reporter {
    // 1. Bài toán Quy hoạch đi dây (Kruskal)
    // In báo cáo chi tiết về kết quả cây khung nhỏ nhất (MST) và các cáp dự phòng.
    // So sánh chi phí tổng thể giữa: Xây tất cả, Chỉ xây MST, và Xây MST + Backup.
    void printMSTReport(const Graph& graph, const CostModel& costModel);

    // 2. In Bảng định tuyến (Routing Table) cho toàn bộ các cặp đỉnh trong mạng
    void printFullRoutingTable(const Graph& graph, const WeightFn& weight = lengthWeight);

    // 3. Bài toán Mô phỏng truyền gói tin (Data Transmission & Delay)
    // Voi moi Packet, dung Dijkstra tim duong di, roi cong don do tre tung chang.
    void printPacketRouteReport(const Graph& graph, const std::vector<Packet>& packets, const WeightFn& weight = lengthWeight);
}

#endif
