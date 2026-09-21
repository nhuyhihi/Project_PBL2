#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>
#include "Graph.h"
#include "IOManager.h"
#include "CostModel.h"
#include "Config.h"

// Hàm tiện ích để so sánh số thực
bool isClose(double a, double b) {
    return std::abs(a - b) < 1e-5;
}

int main() {
    std::cout << "========================================\n";
    std::cout << " TEST MODULE COSTBREAKDOWN (File Input) \n";
    std::cout << "========================================\n\n";

    Graph graph;
    
    // Đọc dữ liệu từ file input chuẩn (data/nodes.txt và data/edges.txt)
    if (!IOManager::loadNodes("data/nodes.txt", graph)) {
        std::cerr << "Loi: Khong load duoc data/nodes.txt\n";
        return 1;
    }
    if (!IOManager::loadEdges("data/edges.txt", graph)) {
        std::cerr << "Loi: Khong load duoc data/edges.txt\n";
        return 1;
    }

    std::cout << "\nLoad thanh cong " << graph.V() << " Nodes va " 
              << graph.E() << " Edges tu file input.\n\n";

    // Đáp án chi phí kỳ vọng (tương ứng với e0 đến e8 trong data/edges.txt)
    std::vector<double> expectedCosts = {2098.0, 2598.0, 2798.0, 3098.0, 1598.0, 9098.0, 115.0, 135.0, 295.0};
    
    bool allPassed = true;

    for (int i = 0; i < graph.E(); ++i) {
        Edge e = graph.getEdge(i);
        double expected = expectedCosts[i];
        CostBreakdown cb = CostModel::calculateCablingCost(e);

        std::cout << "Kiem tra Edge e" << e.id << " (" << (e.mediaType == MediaType::FIBER ? "FIBER" : "COPPER") << "): \n";
        std::cout << "  - Length: " << e.length << "m, Terrain: " << e.terrainFactor << "\n";
        std::cout << "  - Breakdown: Install=" << cb.install 
                  << ", Material=" << cb.material 
                  << ", Equip=" << cb.equipment 
                  << ", Maint=" << cb.maintenance << "\n";
        std::cout << "  - Total Calculated: " << cb.total << " DV\n";
        std::cout << "  - Expected Cost   : " << expected << " DV\n";
        
        if (isClose(cb.total, expected)) {
            std::cout << "  => [PASS]\n\n";
        } else {
            std::cout << "  => [FAIL]\n\n";
            allPassed = false;
        }
    }

    // --- TEST CẠNH KHÔNG ĐI ĐƯỢC (INF) ---
    Edge brokenEdge = graph.getEdge(0);
    brokenEdge.isUp = false; // Báo hiệu đứt cáp
    CostBreakdown cbBroken = CostModel::calculateCablingCost(brokenEdge);
    
    std::cout << "Kiem tra canh bi dut (isUp = false):\n";
    std::cout << "  - Total Calculated: " << cbBroken.total << "\n";
    std::cout << "  - Expected Cost   : INF\n";
    if (cbBroken.total == Config::INF) {
        std::cout << "  => [PASS]\n\n";
    } else {
        std::cout << "  => [FAIL]\n\n";
        allPassed = false;
    }

    if (allPassed) {
        std::cout << ">>> TAT CA CAC TEST TU FILE INPUT DEU CHAY DUNG 100%! <<<\n";
    } else {
        std::cout << ">>> CO TEST FAIL, VUI LONG KIEM TRA LAI! <<<\n";
    }

    return 0;
}
