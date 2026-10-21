#include "Simulator.h"
#include "DelayModel.h"
#include "Config.h"
#include <algorithm>

NetworkSnapshot Simulator::snapshotNetworkState(const Graph& graph) {
    NetworkSnapshot snap;

    for (int edgeId : graph.edgeIds()) {
        const Edge& edge = graph.getEdge(edgeId);
        
        snap.isUp[edgeId] = edge.isUp;
        snap.load[edgeId] = edge.currentLoad;
        snap.queueDelay[edgeId] = edge.queueDelay;
    }
    
    return snap;
}

void Simulator::applyEvent(Graph& graph, const ScenarioEvent& event) {
    try {
        Edge& targetEdge = graph.getEdge(event.targetId);

        switch (event.eventType) {
            case EventType::LOAD_UPDATE:
                targetEdge.currentLoad = event.value;
                break;

            case EventType::DELAY_UPDATE:
                targetEdge.queueDelay = event.value;
                break;

            default:
                break;
        }
    } catch (const std::exception& e) {
    }
}


SimulationResult Simulator::sendPacket(const Packet& packet, const PathResult& path, Graph& graph) {
    SimulationResult result;
    
    if (!path.reachable || path.edges.empty()) {
        result.success = false;
        result.failReason = "NO_ROUTE_AVAILABLE";
        return result;
    }

    for (size_t i = 0; i < path.edges.size(); ++i) {
        int edgeId = path.edges[i];
        try {
            Edge& edge = graph.getEdge(edgeId);
            
            int nextNodeId = (i + 1 < path.nodes.size()) ? path.nodes[i + 1] : graph.otherEndpoint(path.nodes[i], edgeId);
            const Node& nextNode = graph.getNode(nextNodeId);

            int fragments = DelayModel::calcFragmentCount(packet.sizeBytes, edge.mtu, packet.canFragment);
            if (fragments == 0) { 
                result.success = false;
                result.failReason = "DF_BLOCKED_BY_MTU_AT_EDGE_" + std::to_string(edgeId);
                return result; 
            }
            result.fragmentCount = std::max(result.fragmentCount, fragments);

            double edgeDelay = DelayModel::calculateTotalDelay(packet, edge, nextNode);
            result.totalDelay += edgeDelay;

            double dTrans = DelayModel::calcTransmissionDelay(packet.sizeBytes, edge.bandwidthMbps);
            
            updateNetworkState(edge, dTrans);

        } catch (const std::exception& e) {
            result.success = false;
            result.failReason = "EDGE_NOT_FOUND_ERROR";
            return result;
        }
    }

    if (packet.protocol == Protocol::TCP) {
        result.totalDelay += DelayModel::calcProtocolOverhead(packet, result.totalDelay);
    }

    return result;
}

void Simulator::updateNetworkState(Edge& edge, double dTrans) {
    double loadWindow = 1.0; 
    double loadDecay = 0.99;
    
    double loadDelta = dTrans / loadWindow; 
    
    edge.currentLoad = std::max(0.0, std::min(1.0, edge.currentLoad + loadDelta));
    
    edge.currentLoad *= loadDecay;
}
