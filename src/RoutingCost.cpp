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
    if (!edge.isUp) return Config::INF;

    int fragments = DelayModel::calcFragmentCount(packet.sizeBytes, edge.mtu, packet.canFragment);
    if (fragments == 0) return Config::INF;

    const Config::QoSWeight* w = nullptr;
    switch (packet.type) {
        case PacketType::REAL_TIME: w = &Config::QOS_WEIGHT_REAL_TIME; break;
        case PacketType::BULK_DATA: w = &Config::QOS_WEIGHT_BULK_DATA; break;
        case PacketType::CONTROL:   w = &Config::QOS_WEIGHT_CONTROL;   break;
        default:                    w = &Config::QOS_WEIGHT_CONTROL;   break;
    }

    // Tổng hợp các thành phần độ trễ kèm trọng số QoS và phạt phân mảnh
    double dTrans = DelayModel::calcTransmissionDelay(packet.sizeBytes, edge.bandwidthMbps);
    double propSpeed = (edge.mediaType == MediaType::WIRELESS)
                            ? Config::SPEED_OF_LIGHT_AIR
                            : Config::SPEED_OF_LIGHT_FIBER;
    double dProp = DelayModel::calcPropagationDelay(edge.length, propSpeed);
    double dQueue = edge.queueDelay * congestionPenalty(edge.currentLoad);
    double fragmentPenalty = (fragments > 1)
                                  ? (fragments - 1) * Config::FRAGMENT_PENALTY_PER_EXTRA
                                  : 0.0;

    double cost = w->wTrans * dTrans + w->wProp * dProp + w->wQueue * dQueue + fragmentPenalty;
    return cost;
}

WeightFn makeWeightFn(const Packet& packet) {
    return [packet](const Edge& e) {
        return calculateRoutingCost(packet, e);
    };
}

} // namespace RoutingCost
