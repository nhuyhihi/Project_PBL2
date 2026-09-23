#include "Reporter.h"
#include "CostModel.h"
#include "Config.h"
#include <iostream>
#include <iomanip>
#include <string>
#include "DelayModel.h"

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

void printPacketRouteReport(const Graph& graph, const std::vector<Packet>& packets, const WeightFn& weight) {
    std::cout << "\n==============================================================\n";
    std::cout << "       BAO CAO DINH TUYEN + DO TRE GOI TIN (DIJKSTRA)          \n";
    std::cout << "==============================================================\n";

    for (const Packet& pkt : packets) {
        std::cout << "\nPacket #" << pkt.id << ": " << pkt.source << " -> " << pkt.destination
                  << "  (" << pkt.sizeBytes << " bytes)\n";

        if (!graph.hasNode(pkt.source) || !graph.hasNode(pkt.destination)) {
            std::cout << "  => Khong hop le: node nguon hoac dich khong ton tai.\n";
            continue;
        }

        // Buoc 1: dung Dijkstra de tim duong di.
        // onlyBuilt = true: goi tin CHI duoc di tren day da thuc su duoc
        // Kruskal chon xay (MST + Backup)
        PathResult path = dijkstra(graph, pkt.source, pkt.destination, weight, /*onlyBuilt=*/true);
        if (!path.reachable) {
            std::cout << "  => KHONG CO DUONG DI (mang bi chia cat)\n";
            continue;
        }

        std::cout << "  Duong di (" << path.hopCount << " chang): ";
        for (std::size_t i = 0; i < path.nodes.size(); ++i) {
            std::cout << path.nodes[i];
            if (i + 1 < path.nodes.size()) std::cout << " -> ";
        }
        std::cout << "\n";

        // Buoc 2: di doc duong do, cong don do tre tung chang
        double totalDelay = 0.0;
        bool dropped = false;
        int cur = pkt.source;
        for (int edgeId : path.edges) {
            const Edge& e = graph.getEdge(edgeId);
            int nextId = graph.otherEndpoint(cur, edgeId);
            const Node& nextNode = graph.getNode(nextId);

            double d = DelayModel::calculateTotalDelay(pkt, e, nextNode);
            if (d == Config::INF) {
                std::cout << "  => Goi tin bi ROT tai edge " << edgeId
                          << " (day dut hoac qua kho MTU khong the phan manh)\n";
                dropped = true;
                break;
            }
            totalDelay += d;
            cur = nextId;
        }

        if (!dropped) {
            std::cout << "  Tong do tre uoc tinh : " << std::fixed << std::setprecision(6)
                      << totalDelay << " s\n";
        }
    }
    std::cout << "==============================================================\n";
}

} // namespace Reporter
