#ifndef DELAYMODEL_H
#define DELAYMODEL_H

#include "Edge.h"
#include "Node.h"
#include "Packet.h"

namespace DelayModel {

    //Độ trễ truyền tải 
    double calcTransmissionDelay(int sizeBytes, double bandwidthMbps);

    //Độ trễ lan truyền
    double calcPropagationDelay(double length, double propagationSpeed);

    //Độ trễ xử lý tại Node đích
    double calcProcessingDelay(const Node& nextNode);

    //Độ trễ hàng đợi
    double calcQueueDelay(double baseQueueDelay, double currentLoad);

    //Độ trễ cơ sở tổng hợp
    double calcBaseDelay(const Packet& packet, const Edge& edge, const Node& nextNode);

    //Tính số mảnh dữ liệu
    // Trả về số mảnh. Nếu không cho phép phân mảnh và gói tin quá lớn, trả về 0.
    int calcFragmentCount(int sizeBytes, int mtu, bool canFragment);

    //TCP / UDP Protocol Overhead
    double calcProtocolOverhead(const Packet& packet, double oneWayDelay);

    // Tính tổng thể khi gói tin đi qua 1 cạnh
    double calculateTotalDelay(const Packet& packet, const Edge& edge, const Node& nextNode);
}

#endif // DELAYMODEL_H
