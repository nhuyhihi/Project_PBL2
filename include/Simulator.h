#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <string>
#include <vector>
#include "Graph.h"
#include "Scenario.h"
#include "Packet.h"
#include "PathResult.h"

// Kết quả mô phỏng truyền gói tin
struct SimulationResult {
    bool success = true;
    double totalDelay = 0.0;
    std::string failReason = "";
    int fragmentCount = 1;
};

class Simulator {
public:
    // Áp dụng sự kiện vào đồ thị
    static void applyEvent(Graph& graph, const ScenarioEvent& event);

    // Đánh giá kết quả truyền gói tin (Read-Only, không làm thay đổi trạng thái tải của Graph)
    static SimulationResult evaluatePacket(const Packet& packet, const PathResult& path, const Graph& graph);

    // Gửi gói tin thực tế (Tính độ trễ qua evaluatePacket rồi cập nhật tải trên Graph)
    static SimulationResult sendPacket(const Packet& packet, const PathResult& path, Graph& graph);

    // Cập nhật trạng thái tải sau khi gửi
    static void updateNetworkState(Edge& edge, double dTrans);

    // Điều phối toàn bộ một kịch bản sự kiện & Logging ra file .txt
    static void runScenarioSimulation(Graph& graph,
                                     const std::vector<Packet>& packets,
                                     const Scenario& scenario,
                                     const std::string& logFilePath = "");
};

#endif // SIMULATOR_H
