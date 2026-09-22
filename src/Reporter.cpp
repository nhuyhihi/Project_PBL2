#include "Reporter.h"
#include "CostModel.h"
#include "Config.h"
#include <iostream>
#include <iomanip>
#include <string>

namespace Reporter {

void printMSTReport(const Graph& graph) {
    std::cout << "==============================================================\n";
    std::cout << "               BAO CAO KET QUA QUY HOACH CAP                  \n";
    std::cout << "==============================================================\n";
    
    std::cout << std::left 
              << std::setw(8)  << "EdgeID" 
              << std::setw(8)  << "u-v" 
              << std::setw(10) << "Media" 
              << std::setw(15) << "Role" 
              << std::setw(15) << "Cost (DV)" 
              << "\n";
    std::cout << "--------------------------------------------------------------\n";

    double totalBuildAll = 0.0;
    double totalMST = 0.0;
    double totalMSTBackup = 0.0;

    std::vector<int> edgeIds = graph.edgeIds();
    for (int id : edgeIds) {
        const Edge& edge = graph.getEdge(id);
        
        // Bỏ qua các cạnh đứt
        if (!edge.isUp) continue;

        double cost = CostModel::calculateCablingCost(edge).total;
        
        // Build All: cộng tất cả cạnh có thể đi được
        if (cost != Config::INF) {
            totalBuildAll += cost;
        }

        std::string role = "None";
        if (edge.isBackup) {
            role = "Backup";
            totalMSTBackup += cost;
        } else if (edge.isBuilt) {
            role = "MST";
            totalMST += cost;
            totalMSTBackup += cost;
        }

        std::string mediaStr = (edge.mediaType == MediaType::FIBER) ? "FIBER" : 
                               ((edge.mediaType == MediaType::COPPER) ? "COPPER" : "WIRELESS");
        
        std::string uv = std::to_string(edge.u) + "-" + std::to_string(edge.v);

        std::cout << std::left 
                  << std::setw(8)  << edge.id 
                  << std::setw(8)  << uv 
                  << std::setw(10) << mediaStr 
                  << std::setw(15) << role 
                  << std::fixed << std::setprecision(2) << std::setw(15) << cost 
                  << "\n";
    }

    std::cout << "==============================================================\n";
    std::cout << "TONG KET CHI PHI:\n";
    std::cout << "1. Phuong an Xay tat ca cap (Build All)  : " << std::fixed << std::setprecision(2) << totalBuildAll << " DV\n";
    std::cout << "2. Phuong an Toi gian (Chi xay MST)      : " << totalMST << " DV\n";
    std::cout << "3. Phuong an An toan (MST + Backup)      : " << totalMSTBackup << " DV\n";
    std::cout << "==============================================================\n";
}

} // namespace Reporter
