#include "DelayModel.h"
#include "Config.h"

namespace DelayModel {

    double calcTransmissionDelay(int sizeBytes, double bandwidthMbps) {
        if (bandwidthMbps <= 0) return Config::INF;
        // Công thức: (sizeBytes * 8) / (bandwidthMbps * 1e6)
        // Đổi byte ra bit và Mbps ra bps
        return (sizeBytes * 8.0) / (bandwidthMbps * 1000000.0);
    }

    double calcPropagationDelay(double length, double propagationSpeed) {
        if (propagationSpeed <= 0) return 0.0;
        return length / propagationSpeed;
    }

    double calcProcessingDelay(const Node& nextNode) {
        // Trễ do CPU của Router đích xử lý
        return nextNode.processingTime;
    }

    // Hàm phụ trợ tính Penalty nghẽn mạng (Dựa theo mô hình Tuần 4)
    static double congestionPenalty(double load) {
        if (load <= 0.80) return 1.0;
        if (load <= 0.85) return 1.56;
        if (load <= 0.90) return 3.25;
        if (load <= 0.95) return 6.06;
        return 10.00;
    }

    double calcQueueDelay(double baseQueueDelay, double currentLoad) {
        // Trễ hàng đợi bị khuếch đại nếu cáp đang tắc nghẽn
        return baseQueueDelay * congestionPenalty(currentLoad);
    }

    double calcBaseDelay(const Packet& packet, const Edge& edge, const Node& nextNode) {
        if (!edge.isUp) return Config::INF; // Cáp đứt thì không truyền được

        double dTrans = calcTransmissionDelay(packet.sizeBytes, edge.bandwidthMbps);
        
        // Cáp quang/đồng thì truyền theo tốc độ ánh sáng trong thủy tinh/đồng (~2e8)
        // Wireless truyền theo tốc độ ánh sáng trong không khí (~3e8)
        double propSpeed = (edge.mediaType == MediaType::WIRELESS) ? 
                           Config::SPEED_OF_LIGHT_AIR : Config::SPEED_OF_LIGHT_FIBER;
                           
        double dProp = calcPropagationDelay(edge.length, propSpeed);
        
        double dProc = calcProcessingDelay(nextNode);
        double dQueue = calcQueueDelay(edge.queueDelay, edge.currentLoad);

        // Cộng dồn 4 loại trễ
        return dTrans + dProp + dProc + dQueue;
    }

    int calcFragmentCount(int sizeBytes, int mtu, bool canFragment) {
        if (sizeBytes <= mtu) return 1;
        if (!canFragment) return 0; // DF bit is set, packet drops
        
        // Payload mỗi mảnh = mtu - 20 byte header
        int payload = mtu - 20;
        if (payload <= 0) return 0; // Tránh lỗi chia 0 nếu MTU quá nhỏ
        
        // Tính số mảnh: ceil(size / payload)
        int n = sizeBytes / payload;
        if (sizeBytes % payload != 0) n++; // Làm tròn lên
        return n;
    }

    double calcProtocolOverhead(const Packet& packet, double oneWayDelay) {
        if (packet.protocol == Protocol::TCP && packet.newConnection) {
            // TCP 3-way handshake tốn 1 RTT (Round Trip Time = 2 * One-way Delay)
            return 2.0 * oneWayDelay;
        }
        return 0.0;
    }

    double calculateTotalDelay(const Packet& packet, const Edge& edge, const Node& nextNode) {
        if (!edge.isUp) return Config::INF;

        // 1. Kiểm tra phân mảnh
        int fragments = calcFragmentCount(packet.sizeBytes, edge.mtu, packet.canFragment);
        if (fragments == 0) return Config::INF; // Bị rớt gói tin do quá to mà không cho băm

        // 2. Tính Base Delay cơ bản
        double dTrans = calcTransmissionDelay(packet.sizeBytes, edge.bandwidthMbps);
        double propSpeed = (edge.mediaType == MediaType::WIRELESS) ? Config::SPEED_OF_LIGHT_AIR : Config::SPEED_OF_LIGHT_FIBER;
        double dProp = calcPropagationDelay(edge.length, propSpeed);
        double dProc = calcProcessingDelay(nextNode);
        double dQueue = calcQueueDelay(edge.queueDelay, edge.currentLoad);

        // Trễ truyền tải (Transmission) sẽ bị nhân lên theo số lượng mảnh
        // Còn các trễ Prop, Proc, Queue được xem như áp dụng đồng thời dạng Pipeline cho loạt mảnh,
        // nên độ trễ phụ trội chủ yếu nằm ở Transmission. (Công thức đơn giản hóa theo bài).
        double oneWayDelay = (dTrans * fragments) + dProp + dProc + dQueue;

        // 3. Tính Overhead của Giao thức (TCP/UDP)
        double overhead = calcProtocolOverhead(packet, oneWayDelay);

        return oneWayDelay + overhead;
    }

}
