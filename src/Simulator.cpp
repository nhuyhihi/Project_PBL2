#include "Simulator.h"
#include "DelayModel.h"
#include "Config.h" // Chứa LOAD_WINDOW_SECONDS và LOAD_DECAY
#include <algorithm>

// Task B4.2: snapshotNetworkState()
// Mục đích: Dijkstra chạy trên trạng thái cố định trong một lần tính.
NetworkSnapshot Simulator::snapshotNetworkState(const Graph& graph) {
    NetworkSnapshot snap;
    
    // Copy isUp, load, queueDelay từ graph sang snapshot
    for (int edgeId : graph.edgeIds()) {
        const Edge& edge = graph.getEdge(edgeId);
        
        snap.isUp[edgeId] = edge.isUp;
        snap.load[edgeId] = edge.currentLoad;
        snap.queueDelay[edgeId] = edge.queueDelay;
    }
    
    return snap;
}

// Task B4.4: applyEvent()
// Mục đích: Áp dụng các sự kiện thay đổi trạng thái mạng (Load, Delay)
void Simulator::applyEvent(Graph& graph, const ScenarioEvent& event) {
    try {
        // Lấy cạnh bị tác động (Sẽ ném std::runtime_error nếu id không tồn tại)
        Edge& targetEdge = graph.getEdge(event.targetId);

        switch (event.eventType) {
            case EventType::LOAD_UPDATE:
                // Đảm bảo load không vượt quá 1.0 và không nhỏ hơn 0.0
                targetEdge.currentLoad = std::max(0.0, std::min(1.0, event.value));
                break;

            case EventType::DELAY_UPDATE:
                // Đảm bảo delay không âm
                targetEdge.queueDelay = std::max(0.0, event.value);
                break;

            default:
                // Các sự kiện khác (ví dụ LINK_DOWN, LINK_UP) sẽ được xử lý ở Tuần 5
                break;
        }
    } catch (const std::exception& e) {
        // Bỏ qua hoặc log lỗi nếu cố gắng tác động lên một cạnh không tồn tại
        // Nhất quán với cách bắt lỗi của Graph::getEdge
    }
}

// Task B4.5: sendPacket()
SimulationResult Simulator::sendPacket(const Packet& packet, const PathResult& path, Graph& graph) {
    SimulationResult result = {true, 0.0, "", 1};
    
    // Nếu Dijkstra trả về không có đường đi (reachable = false)
    if (!path.reachable || path.edges.empty()) {
        result.success = false;
        result.failReason = "NO_ROUTE_AVAILABLE";
        return result;
    }

    // Duyệt qua từng cạnh trên đường đi
    for (size_t i = 0; i < path.edges.size(); ++i) {
        int edgeId = path.edges[i];
        try {
            Edge& edge = graph.getEdge(edgeId);
            
            // Tìm nextNode (sử dụng node tiếp theo trong path.nodes nếu có)
            // Lấy ID node tiếp theo để DelayModel tính toán Processing Delay
            int nextNodeId = (i + 1 < path.nodes.size()) ? path.nodes[i + 1] : graph.otherEndpoint(path.nodes[i], edgeId);
            const Node& nextNode = graph.getNode(nextNodeId);

            // Kiểm tra MTU & Tính số mảnh bằng hàm có sẵn của DelayModel (Task B3.7)
            int fragments = DelayModel::calcFragmentCount(packet.sizeBytes, edge.mtu, packet.canFragment);
            if (fragments == 0) { // Trả về 0 nghĩa là không cho phép phân mảnh và quá kích thước
                result.success = false;
                result.failReason = "DF_BLOCKED_BY_MTU_AT_EDGE_" + std::to_string(edgeId);
                return result; 
            }
            // Cập nhật fragmentCount (mảnh lớn nhất trên toàn bộ đường đi)
            result.fragmentCount = std::max(result.fragmentCount, fragments);

            // CHÚ Ý DRY: Tính TẤT CẢ các loại delay thông qua hàm tổng hợp của DelayModel
            double edgeDelay = DelayModel::calculateTotalDelay(packet, edge, nextNode);
            result.totalDelay += edgeDelay;

            // CHÚ Ý DRY: Tính lại dTrans để phục vụ updateNetworkState 
            double dTrans = DelayModel::calcTransmissionDelay(packet.sizeBytes, edge.bandwidthMbps);
            
            // Cập nhật tải 
            updateNetworkState(edge, dTrans);

        } catch (const std::exception& e) {
            result.success = false;
            result.failReason = "EDGE_NOT_FOUND_ERROR";
            return result;
        }
    }

    // Cộng TCP overhead một lần duy nhất vào tổng hành trình (Task B3.8)
    if (packet.protocol == Protocol::TCP) {
        // Dùng totalDelay vừa tính được để ước tính RTT (Round Trip Time)
        result.totalDelay += DelayModel::calcProtocolOverhead(packet, result.totalDelay);
    }

    return result;
}

// Task B4.6: updateNetworkState()
void Simulator::updateNetworkState(Edge& edge, double dTrans) {
    // Sử dụng giá trị từ Config, hoặc fallback về 1.0/0.99 nếu Config không có
    double loadWindow = 1.0; 
    double loadDecay = 0.99;
    
    // loadDelta = dTrans / LOAD_WINDOW_SECONDS
    double loadDelta = dTrans / loadWindow; 
    
    // currentLoad = min(1.0, currentLoad + loadDelta)
    edge.currentLoad = std::max(0.0, std::min(1.0, edge.currentLoad + loadDelta));
    
    // Có thể decay: currentLoad *= LOAD_DECAY
    edge.currentLoad *= loadDecay;
}
