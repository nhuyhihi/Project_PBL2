#ifndef DELAYMODEL_H
#define DELAYMODEL_H

#include "Edge.h"
#include "Node.h"
#include "Packet.h"

namespace DelayModel {

    // Độ trễ truyền tải
    double calcTransmissionDelay(int sizeBytes, double bandwidthMbps);

    // Độ trễ lan truyền
    double calcPropagationDelay(double length, double propagationSpeed);

    // Độ trễ xử lý tại Node đích
    double calcProcessingDelay(const Node& nextNode);

    // Độ trễ hàng đợi
    double calcQueueDelay(double baseQueueDelay, double currentLoad);

    // Độ trễ cơ sở tổng hợp (1 mảnh trên 1 chặng)
    double calcBaseDelay(const Packet& packet, const Edge& edge, const Node& nextNode);

    // Tính số mảnh dữ liệu (Trả về 0 nếu không cho phép phân mảnh và gói tin vượt MTU)
    int calcFragmentCount(int sizeBytes, int mtu, bool canFragment);

    // TCP / UDP Protocol Overhead (1 RTT cho TCP kết nối mới)
    double calcProtocolOverhead(const Packet& packet, double oneWayDelay);

    // Tính độ trễ truyền một chặng/cạnh (đã tính phân mảnh MTU, chưa cộng TCP handshake)
    double calculateHopDelay(const Packet& packet, const Edge& edge, const Node& nextNode);
}

#endif // DELAYMODEL_H
