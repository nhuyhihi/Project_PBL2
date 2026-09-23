#ifndef ROUTE_REPORT_H
#define ROUTE_REPORT_H

#include <vector>
#include "Graph.h"
#include "Packet.h"
#include "Dijkstra.h"

// ============================================================
// RouteReport.h - Noi Dijkstra (Quyen) voi DelayModel (Y).
//
// Voi moi Packet (co source, destination), dung Dijkstra tim duong di,
// roi cong don do tre tung chang bang DelayModel::calculateTotalDelay.
// Day chinh la buoc con thieu trong so do Tuan 3:
//   MST + Backup -> Dijkstra -> PathResult -> Delay / MTU / TCP / UDP
// ============================================================
namespace RouteReport {

// In bao cao dinh tuyen + do tre cho tung packet trong danh sach.
// weight: dung de Dijkstra chon duong (mac dinh theo do dai vat ly).
void printPacketRouteReport(const Graph& graph, const std::vector<Packet>& packets,
                             const WeightFn& weight = lengthWeight);

} // namespace RouteReport

#endif // ROUTE_REPORT_H
