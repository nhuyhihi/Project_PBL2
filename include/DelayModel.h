#ifndef DELAYMODEL_H
#define DELAYMODEL_H

#include "Edge.h"
#include "Node.h"
#include "Packet.h"

namespace DelayModel {

    // B3.1 Transmission Delay (Độ trễ truyền tải)
    // sizeBytes: Kích thước dữ liệu (Byte)
    // bandwidthMbps: Băng thông (Megabits per second)
    // Return: Giây (s)
    double calcTransmissionDelay(int sizeBytes, double bandwidthMbps);

    // B3.2 Propagation Delay (Độ trễ lan truyền)
    // length: Chiều dài dây (m)
    // propagationSpeed: Tốc độ truyền (m/s)
    // Return: Giây (s)
    double calcPropagationDelay(double length, double propagationSpeed);

    // B3.3 Processing Delay (Độ trễ xử lý tại Node đích)
    // nextNode: Nút mạng tiếp theo sẽ nhận gói tin
    // Return: Giây (s)
    double calcProcessingDelay(const Node& nextNode);

    // B3.4 Queue Delay (Độ trễ hàng đợi)
    // baseQueueDelay: Thời gian chờ cơ bản (lấy từ Edge)
    // currentLoad: Tải hiện tại của dây cáp (0.0 đến 1.0)
    // Return: Giây (s)
    double calcQueueDelay(double baseQueueDelay, double currentLoad);

    // B3.5 Base Delay (Độ trễ cơ sở tổng hợp)
    // Tổng của 4 loại độ trễ trên cho 1 gói tin đi qua 1 cạnh
    double calcBaseDelay(const Packet& packet, const Edge& edge, const Node& nextNode);

    // B3.6 & B3.7 MTU / Fragmentation (Tính số mảnh dữ liệu)
    // Trả về số mảnh. Nếu không cho phép phân mảnh và gói tin quá lớn, trả về 0.
    int calcFragmentCount(int sizeBytes, int mtu, bool canFragment);

    // B3.8 TCP / UDP Protocol Overhead
    // Trả về thời gian RTT (2 * One-way Delay) nếu là kết nối TCP mới, ngược lại trả về 0.
    double calcProtocolOverhead(const Packet& packet, double oneWayDelay);

    // Hàm xử lý độ trễ tổng thể một cách hoàn chỉnh khi gói tin đi qua 1 cạnh
    // Đã tích hợp tính năng Phân mảnh và Bắt tay TCP
    double calculateTotalDelay(const Packet& packet, const Edge& edge, const Node& nextNode);
    
}

#endif // DELAYMODEL_H
