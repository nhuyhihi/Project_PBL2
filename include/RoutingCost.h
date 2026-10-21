#ifndef ROUTING_COST_H
#define ROUTING_COST_H

#include "Edge.h"
#include "Packet.h"
#include "Dijkstra.h"   
namespace RoutingCost {

double congestionPenalty(double load);
double calculateRoutingCost(const Packet& packet, const Edge& edge);

}

#endif 
