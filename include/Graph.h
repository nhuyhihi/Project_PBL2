#ifndef GRAPH_H
#define GRAPH_H

#include <vector>
#include <unordered_map>
#include <stdexcept>
#include "Node.h"
#include "Edge.h"

// ============================================================
// Graph - đồ thị vô hướng lưu mạng.
//
// Lưu 3 thứ:
//   nodes     : id -> Node
//   edges     : id -> Edge
//   adjacency : nodeId -> danh sách EDGE ID chạm vào node đó
//
// Vì sao adjacency lưu edgeId thay vì nodeId?
//   Hai node có thể có NHIỀU dây ứng viên (vd cáp đồng + cáp quang).
//   Có edgeId thì biết chính xác dây nào, lấy được cost/bandwidth của nó.
//   Muốn biết node bên kia thì gọi otherEndpoint(nodeId, edgeId).
//
// Vì sao getNode/getEdge trả THAM CHIẾU (không copy)?
//   Kruskal phải sửa trực tiếp usedPorts, isBuilt, isBackup trong đồ thị.
//   Nếu trả bản copy thì sửa xong đồ thị không đổi gì.
// ============================================================
class Graph {
private:
    std::unordered_map<int, Node> nodes;
    std::unordered_map<int, Edge> edges;

    // adjacency[nodeId] = danh sách các edgeId kề
    std::unordered_map<int, std::vector<int>> adjacency;

public:
    Graph();
    ~Graph();

    // Thêm Node. Trả false (không thêm gì) nếu id đã tồn tại.
    bool addNode(const Node& node);

    // Thêm Edge. Trả false (không thêm gì) nếu: id đã tồn tại,
    // node u hoặc v chưa tồn tại, hoặc u == v (dây nối node với chính nó).
    // Adjacency được cập nhật ngay: edge xuất hiện ở CẢ HAI đầu.
    bool addEdge(const Edge& edge);

    bool hasNode(int nodeId) const;
    bool hasEdge(int edgeId) const;

    // Lấy Node / Edge theo id. Ném std::runtime_error nếu không tồn tại
    // (Validation dùng cách bắt exception này để kiểm tra node có tồn tại).
    // Bản non-const trả tham chiếu để sửa được usedPorts, isBuilt, ...
    Node&       getNode(int nodeId);
    const Node& getNode(int nodeId) const;
    Edge&       getEdge(int edgeId);
    const Edge& getEdge(int edgeId) const;

    // Danh sách edgeId kề với một node (rỗng nếu node không tồn tại)
    const std::vector<int>& getNeighbors(int nodeId) const;

    // Trả nodeId ở đầu kia của cạnh khi biết một nodeId và edgeId.
    // Trả -1 nếu edge không tồn tại hoặc node không thuộc edge đó.
    int otherEndpoint(int nodeId, int edgeId) const;

    // Dựng lại adjacency từ đầu theo danh sách edges (dùng khi sửa trực tiếp edges).
    // Thứ tự edgeId trong mỗi danh sách tăng dần -> kết quả luôn giống nhau giữa các lần chạy.
    void buildAdjacency();

    // Số lượng node / edge
    int V() const;
    int E() const;

    // Danh sách id đã sắp tăng dần (unordered_map duyệt không theo thứ tự cố định,
    // nên thuật toán cần thứ tự ổn định thì lấy từ đây)
    std::vector<int> nodeIds() const;
    std::vector<int> edgeIds() const;

    // Id node lớn nhất (-1 nếu chưa có node). DSU dùng để biết cần bao nhiêu phần tử.
    int maxNodeId() const;

    // Đưa về trạng thái "chưa xây gì": usedPorts = 0, isBuilt = isBackup = false
    void resetBuildState();
};

#endif // GRAPH_H
