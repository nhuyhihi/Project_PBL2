#ifndef ROUTING_COST_H
#define ROUTING_COST_H

#include "Edge.h"
#include "Packet.h"
#include "Dijkstra.h"   // for WeightFn

// ============================================================
// RoutingCost.h - Trọng số định tuyến động theo QoS và trạng thái mạng
// ============================================================
namespace RoutingCost {

// Hệ số phạt tăng theo tải hiện tại của cạnh (load trong [0,1]).
double congestionPenalty(double load);

// Chi phí định tuyến "động" của 1 cạnh, phụ thuộc loại
// gói tin (QoS) và trạng thái tức thời của cạnh (currentLoad, queueDelay).
//
//   cost = wTrans*dTrans + wProp*dProp + wQueue*dQueue*congestionPenalty
//          + fragmentPenalty
//
// - Nếu cạnh đang đứt (isUp = false) -> trả Config::INF.
// - Nếu gói tin DF (canFragment = false) mà vượt MTU -> trả Config::INF.
double calculateRoutingCost(const Packet& packet, const Edge& edge);

// Đóng gói calculateRoutingCost cho 1 Packet cụ thể thành 1 WeightFn cho Dijkstra
WeightFn makeWeightFn(const Packet& packet);

} // namespace RoutingCost

#endif // ROUTING_COST_H
