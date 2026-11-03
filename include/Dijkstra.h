#ifndef DIJKSTRA_H
#define DIJKSTRA_H

#include <functional>
#include "Graph.h"
#include "PathResult.h"

// ============================================================
// Dijkstra.h - Thuật toán định tuyến tìm đường đi ngắn nhất theo trọng số động
// ============================================================
using WeightFn = std::function<double(const Edge&)>;

// weight = độ dài vật lý của cáp (trọng số mặc định)
double lengthWeight(const Edge& e);

// Tìm đường đi rẻ nhất từ source đến destination.
// Luôn bỏ qua cạnh đang isUp = false (dây đứt).
// onlyBuilt: khi true, CHỈ đi trên các cạnh đã isBuilt = true (MST + Backup).
PathResult dijkstra(const Graph& g, int source, int destination,
                     const WeightFn& weight = lengthWeight,
                     bool onlyBuilt = false);

// Quá tải tiện lợi: Định tuyến trực tiếp cho một gói tin Packet cụ thể theo QoS
struct Packet;
PathResult dijkstra(const Graph& g, int source, int destination,
                     const Packet& packet,
                     bool onlyBuilt = true);

#endif // DIJKSTRA_H
