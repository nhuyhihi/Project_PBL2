#include "DelayModel.h"
#include "Config.h"
#include "RoutingCost.h"

namespace DelayModel {

    double calcTransmissionDelay(int sizeBytes, double bandwidthMbps) {
        if (bandwidthMbps <= 0) return Config::INF;
        // Công thức: (sizeBytes * 8) / (bandwidthMbps * 1e6)
        return (sizeBytes * 8.0) / (bandwidthMbps * 1000000.0);
    }

    double calcPropagationDelay(double length, double propagationSpeed) {
        if (propagationSpeed <= 0) return 0.0;
        return length / propagationSpeed;
    }

    double calcProcessingDelay(const Node& nextNode) {
        return nextNode.processingTime;
    }

    double calcQueueDelay(double baseQueueDelay, double currentLoad) {
        return baseQueueDelay * RoutingCost::congestionPenalty(currentLoad);
    }

    double calcBaseDelay(const Packet& packet, const Edge& edge, const Node& nextNode) {
        if (!edge.isUp) return Config::INF;

        double dTrans = calcTransmissionDelay(packet.sizeBytes, edge.bandwidthMbps);
        double propSpeed = (edge.mediaType == MediaType::WIRELESS) ?
                           Config::SPEED_OF_LIGHT_AIR : Config::SPEED_OF_LIGHT_FIBER;
        double dProp = calcPropagationDelay(edge.length, propSpeed);
        double dProc = calcProcessingDelay(nextNode);
        double dQueue = calcQueueDelay(edge.queueDelay, edge.currentLoad);

        return dTrans + dProp + dProc + dQueue;
    }

    int calcFragmentCount(int sizeBytes, int mtu, bool canFragment) {
        if (sizeBytes <= mtu) return 1;
        if (!canFragment) return 0; // DF bit is set, packet drops

        int payload = mtu - 20;
        if (payload <= 0) return 0;

        int n = sizeBytes / payload;
        if (sizeBytes % payload != 0) n++;
        return n;
    }

    double calcProtocolOverhead(const Packet& packet, double oneWayDelay) {
        if (packet.protocol == Protocol::TCP && packet.newConnection) {
            return 2.0 * oneWayDelay;
        }
        return 0.0;
    }

    double calculateHopDelay(const Packet& packet, const Edge& edge, const Node& nextNode) {
        if (!edge.isUp) return Config::INF;

        int fragments = calcFragmentCount(packet.sizeBytes, edge.mtu, packet.canFragment);
        if (fragments == 0) return Config::INF;

        double baseDelay = calcBaseDelay(packet, edge, nextNode);
        if (baseDelay == Config::INF) return Config::INF;

        double dTrans = calcTransmissionDelay(packet.sizeBytes, edge.bandwidthMbps);
        return baseDelay + dTrans * (fragments - 1);
    }

}
