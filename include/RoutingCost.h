#ifndef ROUTING_COST_H
#define ROUTING_COST_H

#include "Edge.h"
#include "Packet.h"
#include "Dijkstra.h"   // for WeightFn

// ============================================================
// RoutingCost.h 
// Trước đây Dijkstra định tuyến theo weight "tĩnh" (unitWeight
// hoặc lengthWeight) - không quan tâm tải hiện tại của dây hay loại gói
// tin. Bây giờ cần Dijkstra định tuyến "động": cùng 1 mạng nhưng gói tin
// REAL_TIME và BULK_DATA có thể được định tuyến theo 2 đường khác nhau,
// tùy theo cạnh nào đang bị nghẽn.
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
// - Không bao giờ trả NaN (có chặn phòng thủ ở cuối hàm).
//
// LƯU Ý: không cộng dProc (độ trễ xử lý tại node kế tiếp) vào đây, vì
// WeightFn chỉ nhận được Edge (không có thông tin đang đi theo hướng
// u->v hay v->u, nên không xác định được "node kế tiếp" là node nào).
// dProc vẫn được cộng đầy đủ khi tính TỔNG độ trễ thực tế của gói tin
// (DelayModel::calculateTotalDelay, dùng trong Simulator::sendPacket) -
// chỉ không tham gia vào việc Dijkstra CHỌN đường, vì nó gần như cố định
// theo node đích, không giúp so sánh giữa các lựa chọn đường khác nhau.
double calculateRoutingCost(const Packet& packet, const Edge& edge);

// Đóng gói calculateRoutingCost cho 1 Packet cụ thể thành 1
// WeightFn, để Dijkstra chỉ cần gọi weight(e) như các weight tĩnh trước
// đây (unitWeight, lengthWeight) - Dijkstra.cpp KHÔNG cần biết gì về
// QoS/tắc nghẽn/MTU, giữ đúng nguyên tắc tách biệt thuật toán và nghiệp vụ.
WeightFn makeWeightFn(const Packet& packet);

} // namespace RoutingCost

#endif // ROUTING_COST_H
