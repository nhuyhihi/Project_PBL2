#ifndef REPORTER_H
#define REPORTER_H

#include "Graph.h"
#include "Packet.h"
#include "Dijkstra.h"

namespace Reporter {
    // 1. Bài toán Quy hoạch đi dây (Kruskal)
    // In báo cáo chi tiết về kết quả cây khung nhỏ nhất (MST) và các cáp dự phòng.
    // So sánh chi phí tổng thể giữa: Xây tất cả, Chỉ xây MST, và Xây MST + Backup.
    void printMSTReport(const Graph& graph);

    // 2. Bài toán Định tuyến gói tin (Dijkstra)
    // Voi moi Packet, dung Dijkstra tim duong di, roi cong don do tre tung chang.
    void printPacketRouteReport(const Graph& graph, const std::vector<Packet>& packets, const WeightFn& weight = lengthWeight);
}

#endif
