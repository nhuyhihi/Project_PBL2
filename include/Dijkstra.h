#ifndef DIJKSTRA_H
#define DIJKSTRA_H

#include <functional>
#include "Graph.h"
#include "PathResult.h"

// Thuật toán Dijkstra tìm đường đi ngắn nhất theo trọng số
using WeightFn = std::function<double(const Edge&)>;

double lengthWeight(const Edge& e);

// Tìm đường đi rẻ nhất từ source đến destination (bỏ qua cạnh isUp = false)
PathResult dijkstra(const Graph& g, int source, int destination,
                     const WeightFn& weight = lengthWeight,
                     bool onlyBuilt = false);

// Tìm đường đi tối ưu theo loại dịch vụ QoS của gói tin
struct Packet;
PathResult dijkstra(const Graph& g, int source, int destination,
                     const Packet& packet,
                     bool onlyBuilt = true);

#endif // DIJKSTRA_H
