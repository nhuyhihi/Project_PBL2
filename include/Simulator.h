#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <unordered_map>
#include <string>
#include "Graph.h"
#include "Scenario.h"
#include "Packet.h"
#include "PathResult.h"

// Task B4.5: Kết quả mô phỏng truyền gói tin
struct SimulationResult {
    bool success;
    double totalDelay;
    std::string failReason;
    int fragmentCount;
};

// Task B4.1: NetworkSnapshot
// Lưu trữ bản sao trạng thái của mạng tại một thời điểm
struct NetworkSnapshot {
    std::unordered_map<int, bool> isUp;
    std::unordered_map<int, double> load;
    std::unordered_map<int, double> queueDelay;
};

class Simulator {
public:
    // Task B4.2: Hàm tạo snapshot
    static NetworkSnapshot snapshotNetworkState(const Graph& graph);
    
    // Task B4.4: Hàm áp dụng sự kiện vào đồ thị
    static void applyEvent(Graph& graph, const ScenarioEvent& event);
    
    // Task B4.5: Gửi gói tin
    static SimulationResult sendPacket(const Packet& packet, const PathResult& path, Graph& graph);
    
    // Task B4.6: Cập nhật trạng thái tải sau khi gửi
    static void updateNetworkState(Edge& edge, double dTrans);
};

#endif // SIMULATOR_H
