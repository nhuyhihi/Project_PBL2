#include "RoutingCost.h"
#include "Config.h"
#include "DelayModel.h"
#include <cmath>

namespace RoutingCost {

double congestionPenalty(double load) {
    if (load <= 0.80) return 1.0;
    if (load <= 0.85) return 1.56;
    if (load <= 0.90) return 3.25;
    if (load <= 0.95) return 6.06;
    return 10.00;
}

double calculateRoutingCost(const Packet& packet, const Edge& edge) {
    // Day dang dut -> khong the di qua
    if (!edge.isUp) return Config::INF;

    // Kiem tra phan manh truoc: neu la goi DF (khong cho phan manh) ma
    // vuot qua MTU thi khong the di qua day nay duoc (giong DelayModel).
    int fragments = DelayModel::calcFragmentCount(packet.sizeBytes, edge.mtu, packet.canFragment);
    if (fragments == 0) return Config::INF;

    // Chon bo trong so QoS theo loai goi tin
    const Config::QoSWeight* w = nullptr;
    switch (packet.type) {
        case PacketType::REAL_TIME: w = &Config::QOS_WEIGHT_REAL_TIME; break;
        case PacketType::BULK_DATA: w = &Config::QOS_WEIGHT_BULK_DATA; break;
        case PacketType::CONTROL:   w = &Config::QOS_WEIGHT_CONTROL;   break;
        default:                    w = &Config::QOS_WEIGHT_CONTROL;   break;
    }

    // Tai su dung dung cac cong thuc thanh phan da co san o DelayModel
    // de khong viet trung logic - chi khac o cho nhan them
    // trong so QoS va he so tac nghen khi cong lai thanh 1 "chi phi".
    double dTrans = DelayModel::calcTransmissionDelay(packet.sizeBytes, edge.bandwidthMbps);
    double propSpeed = (edge.mediaType == MediaType::WIRELESS)
                            ? Config::SPEED_OF_LIGHT_AIR
                            : Config::SPEED_OF_LIGHT_FIBER;
    double dProp = DelayModel::calcPropagationDelay(edge.length, propSpeed);
    double dQueue = edge.queueDelay * congestionPenalty(edge.currentLoad);

    // Duoc dung ngay o day (nhan vao dQueue) - cang tai nhieu,
    // dQueue cang bi khuech dai manh, Dijkstra se tu dong tranh duong nay.
    double fragmentPenalty = (fragments > 1)
                                  ? (fragments - 1) * Config::FRAGMENT_PENALTY_PER_EXTRA
                                  : 0.0;

    double cost = w->wTrans * dTrans + w->wProp * dProp + w->wQueue * dQueue + fragmentPenalty;

    // Phong thu: khong de NaN lot qua (vd bandwidthMbps = 0 -> dTrans = INF,
    // INF * trong so van la INF, khong phai NaN, nhung van kiem tra cho chac).
    if (std::isnan(cost)) return Config::INF;

    return cost;
}

WeightFn makeWeightFn(const Packet& packet) {
    return [packet](const Edge& e) {
        return calculateRoutingCost(packet, e);
    };
}

} // namespace RoutingCost
