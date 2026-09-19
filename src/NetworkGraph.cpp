#include "NetworkGraph.h"

// --- struct Edge ---
bool Edge::operator<(const Edge &other) const {
    return w < other.w;
}

// --- class DSU ---
DSU::DSU(int n) {
    parent.resize(n + 1);
    rankValue.assign(n + 1, 0);
    for (int i = 0; i <= n; i++) {
        parent[i] = i;
    }
}

int DSU::find(int x) {
    if (parent[x] != x) {
        parent[x] = find(parent[x]); // Path compression
    }
    return parent[x];
}

bool DSU::unite(int a, int b) {
    a = find(a);
    b = find(b);

    if (a == b) return false;

    if (rankValue[a] < rankValue[b]) {
        swap(a, b);
    }
    parent[b] = a;
    if (rankValue[a] == rankValue[b]) {
        rankValue[a]++;
    }
    return true;
}

// --- class NetworkGraph ---
NetworkGraph::NetworkGraph(int n, bool directed) : numNodes(n), numEdges(0), isDirected(directed) {
    adj.resize(n + 1);
}

void NetworkGraph::addEdge(int u, int v, long long w) {
    // Cập nhật danh sách cạnh cho Kruskal
    edgeList.push_back({u, v, w});
    numEdges++;

    // Cập nhật danh sách kề cho Dijkstra
    adj[u].push_back({v, w});
    if (!isDirected) {
        adj[v].push_back({u, w});
    }
}

#include <algorithm>
#include <fstream>

void NetworkGraph::printPath(int source, int destination, const vector<int>& trace) {
    if (trace[destination] == -1 && source != destination) {
        cout << "Khong co duong di tu " << source << " den " << destination << endl;
        return;
    }
    
    vector<int> path;
    for (int v = destination; v != -1; v = trace[v]) {
        path.push_back(v);
    }
    reverse(path.begin(), path.end());
    
    cout << "Lo trinh: ";
    for (size_t i = 0; i < path.size(); i++) {
        cout << path[i];
        if (i < path.size() - 1) cout << " -> ";
    }
    cout << endl;
}

void NetworkGraph::dijkstra(int source, int destination) {
    vector<long long> dist(numNodes + 1, LLONG_MAX);
    vector<int> trace(numNodes + 1, -1);
    
    dist[source] = 0;
    priority_queue<pair<long long, int>, vector<pair<long long, int>>, greater<pair<long long, int>>> pq;
    pq.push({0, source});
    
    while (!pq.empty()) {
        long long d = pq.top().first;
        int u = pq.top().second;
        pq.pop();
        
        if (d > dist[u]) continue;
        if (u == destination) break; // Dung som neu da tim thay dich
        
        for (auto edge : adj[u]) {
            int v = edge.first;
            long long w = edge.second;
            
            if (dist[u] + w < dist[v]) {
                dist[v] = dist[u] + w;
                trace[v] = u;
                pq.push({dist[v], v});
            }
        }
    }
    
    if (dist[destination] == LLONG_MAX) {
        cout << "INF" << endl;
    } else {
        cout << "Tong do tre (chi phi) ngan nhat: " << dist[destination] << endl;
        printPath(source, destination, trace);
    }
}

vector<Edge> NetworkGraph::kruskal() {
    sort(edgeList.begin(), edgeList.end());
    
    DSU dsu(numNodes);
    long long totalWeight = 0;
    int selectedEdges = 0;
    vector<Edge> mstEdges;
    
    for (const Edge &edge : edgeList) {
        if (dsu.unite(edge.u, edge.v)) {
            mstEdges.push_back(edge);
            totalWeight += edge.w;
            selectedEdges++;
            
            if (selectedEdges == numNodes - 1) break;
        }
    }
    
    if (selectedEdges != numNodes - 1 && numNodes > 1) {
        cout << "Do thi khong lien thong, khong the tao cay khung!" << endl;
    } else {
        cout << "Tong chi phi cay khung nho nhat: " << totalWeight << endl;
    }
    
    return mstEdges;
}

void NetworkGraph::loadFromFile(const string& filename) {
    ifstream fin(filename);
    if (!fin) {
        cout << "Khong the mo file " << filename << endl;
        return;
    }
    
    int n, m;
    if (fin >> n >> m) {
        numNodes = n;
        numEdges = 0;
        adj.assign(n + 1, vector<pair<int, long long>>());
        edgeList.clear();
        
        for (int i = 0; i < m; i++) {
            int u, v;
            long long w;
            fin >> u >> v >> w;
            addEdge(u, v, w);
        }
    }
    fin.close();
}
