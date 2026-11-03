#include "Dijkstra.h"
#include <queue>
#include <unordered_map>
#include <limits>
#include "RoutingCost.h"

double lengthWeight(const Edge& e) { return e.length; }

namespace {

struct State {
    double dist;
    int    node;
};

bool operator>(const State& a, const State& b) {
    return a.dist > b.dist;
}

} // namespace

PathResult dijkstra(const Graph& g, int source, int destination, const WeightFn& weight, bool onlyBuilt) {
    PathResult result;

    if (!g.hasNode(source) || !g.hasNode(destination)) return result;

    std::unordered_map<int, double> dist;
    std::unordered_map<int, int>    parent;
    std::unordered_map<int, int>    parentEdge;
    std::unordered_map<int, bool>   visited;

    for (int id : g.nodeIds()) dist[id] = std::numeric_limits<double>::infinity();
    dist[source] = 0.0;

    std::priority_queue<State, std::vector<State>, std::greater<State>> pq;
    pq.push({0.0, source});

    while (!pq.empty()) {
        State cur = pq.top();
        pq.pop();

        if (visited[cur.node]) continue;
        if (cur.dist > dist[cur.node]) continue;

        visited[cur.node] = true;
        if (cur.node == destination) break;

        for (int edgeId : g.getNeighbors(cur.node)) {
            const Edge& e = g.getEdge(edgeId);
            if (!e.isUp) continue;
            if (onlyBuilt && !e.isBuilt) continue;
            int v = g.otherEndpoint(cur.node, edgeId);
            if (v == -1 || visited[v]) continue;

            double w = weight(e);
            double newDist = dist[cur.node] + w;

            if (newDist < dist[v]) {
                dist[v] = newDist;
                parent[v] = cur.node;
                parentEdge[v] = edgeId;
                pq.push({newDist, v});
            }
        }
    }

    if (!visited[destination]) {
        return result;
    }

    std::vector<int> nodesRev;
    std::vector<int> edgesRev;
    int cur = destination;
    nodesRev.push_back(cur);
    while (cur != source) {
        edgesRev.push_back(parentEdge[cur]);
        cur = parent[cur];
        nodesRev.push_back(cur);
    }

    result.reachable = true;
    result.nodes.assign(nodesRev.rbegin(), nodesRev.rend());
    result.edges.assign(edgesRev.rbegin(), edgesRev.rend());
    result.totalCost = dist[destination];
    result.hopCount  = static_cast<int>(result.edges.size());

    return result;
}

PathResult dijkstra(const Graph& g, int source, int destination, const Packet& packet, bool onlyBuilt) {
    return dijkstra(g, source, destination, [&packet](const Edge& e) {
        return RoutingCost::calculateRoutingCost(packet, e);
    }, onlyBuilt);
}
