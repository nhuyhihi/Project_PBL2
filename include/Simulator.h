#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <string>
#include <vector>
#include "Graph.h"
#include "Scenario.h"
#include "Packet.h"
#include "PathResult.h"

// Kết quả mô phỏng truyền gói tin trên lộ trình
struct SimulationResult {
    bool success = true;
    double totalDelay = 0.0;
    std::string failReason = "";
    int fragmentCount = 1;
};

// Bộ điều phối mô phỏng sự kiện mạng và đánh giá truyền gói tin
class Simulator {
public:
    static void applyEvent(Graph& graph, const ScenarioEvent& event);

    // Đánh giá truyền gói tin ở chế độ chỉ đọc (không thay đổi currentLoad)
    static SimulationResult evaluatePacket(const Packet& packet, const PathResult& path, const Graph& graph);

    // Gửi gói tin thực tế và cập nhật tải lên đồ thị
    static SimulationResult sendPacket(const Packet& packet, const PathResult& path, Graph& graph);

    static void updateNetworkState(Edge& edge, double dTrans);

    // Chạy toàn bộ kịch bản theo các mốc thời gian và xuất log nếu có đường dẫn
    static void runScenarioSimulation(Graph& graph,
                                     const std::vector<Packet>& packets,
                                     const Scenario& scenario,
                                     const std::string& logFilePath = "");
};

#endif // SIMULATOR_H
