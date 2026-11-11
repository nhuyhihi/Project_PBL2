#ifndef ROUTING_COST_H
#define ROUTING_COST_H

#include "Edge.h"
#include "Packet.h"
#include "Dijkstra.h"   // for WeightFn

// Tính chi phí định tuyến động theo QoS và trạng thái mạng
namespace RoutingCost {

// Hệ số phạt bậc thang theo tải trọng hiện tại của cạnh
double congestionPenalty(double load);

// Tính chi phí định tuyến một cạnh cho gói tin (trả Config::INF nếu đứt hoặc vi phạm DF)
double calculateRoutingCost(const Packet& packet, const Edge& edge);

// Đóng gói hàm chi phí định tuyến cho Dijkstra
WeightFn makeWeightFn(const Packet& packet);

} // namespace RoutingCost

#endif // ROUTING_COST_H
