#include "RouteReport.h"
#include "DelayModel.h"
#include "Config.h"
#include <iostream>
#include <iomanip>

namespace RouteReport {

void printPacketRouteReport(const Graph& graph, const std::vector<Packet>& packets, const WeightFn& weight) {
    std::cout << "==============================================================\n";
    std::cout << "       BAO CAO DINH TUYEN + DO TRE GOI TIN (DIJKSTRA)          \n";
    std::cout << "==============================================================\n";

    for (const Packet& pkt : packets) {
        std::cout << "\nPacket #" << pkt.id << ": " << pkt.source << " -> " << pkt.destination
                  << "  (" << pkt.sizeBytes << " bytes)\n";

        if (!graph.hasNode(pkt.source) || !graph.hasNode(pkt.destination)) {
            std::cout << "  => Khong hop le: node nguon hoac dich khong ton tai.\n";
            continue;
        }

        // Buoc 1: dung Dijkstra (module cua Quyen) de tim duong di.
        // onlyBuilt = true: goi tin CHI duoc di tren day da thuc su duoc
        // Kruskal chon xay (MST + Backup), khong duoc di tren toan bo
        // do thi ung vien (neu khong se ra duong di khong co that ngoai
        // doi, vi du chui qua 1 thiet bi Host chi co 1 cong).
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

        // Buoc 2: di doc duong do, cong don do tre tung chang (module cua Y)
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

} // namespace RouteReport
