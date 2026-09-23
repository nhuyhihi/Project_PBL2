#ifndef DIJKSTRA_H
#define DIJKSTRA_H

#include <functional>
#include "Graph.h"
#include "PathResult.h"

// ============================================================
// Dijkstra.h - Tuần 3, Task A3.1 -> A3.5.
//
// Tìm đường đi "rẻ nhất" (theo weight) từ 1 node nguồn đến 1 node đích,
// trên đồ thị vô hướng Graph đã có (Node/Edge/adjacency của Tuần 1-2).
//
// weight KHÔNG cố định là gì: A3.2 yêu cầu test lần lượt với
//   1. weight = 1        (giống BFS, đếm số hop)
//   2. weight = length   (khoảng cách vật lý)
// Sau này khi ghép với DelayModel (module của Ý) chỉ cần đổi WeightFn
// truyền vào, không phải sửa lại thuật toán - giống cách Kruskal nhận CostFn.
// ============================================================
using WeightFn = std::function<double(const Edge&)>;

// weight = 1 cho mọi cạnh -> Dijkstra chạy giống BFS, dist = số hop
double unitWeight(const Edge& e);

// weight = độ dài vật lý của cáp
double lengthWeight(const Edge& e);

// Tìm đường đi rẻ nhất từ source đến destination.
// Trả PathResult.reachable = false nếu không tới được (đồ thị rời rạc,
// source/destination không tồn tại, hoặc không có đường).
//
// Luôn bỏ qua cạnh đang isUp = false (dây đứt) - không thể đi qua dây hỏng.
//
// onlyBuilt: khi true, CHỈ đi trên các cạnh đã isBuilt = true (đã được Kruskal
// chọn xây, tức MST + Backup). Đây là chế độ đúng cho việc định tuyến gói tin
// thật, vì gói tin chỉ có thể đi trên dây cáp ĐÃ LẮP, không phải trên toàn bộ
// dây ứng viên. Để false khi muốn khảo sát trên đồ thị ứng viên đầy đủ
// (ví dụ so sánh trước khi quyết định xây gì).
PathResult dijkstra(const Graph& g, int source, int destination,
                     const WeightFn& weight = lengthWeight,
                     bool onlyBuilt = false);

#endif // DIJKSTRA_H
