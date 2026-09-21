# BÁO CÁO VÀ GIẢI THÍCH CHI TIẾT TẤT CẢ CÁC FILE TUẦN 1 (Ý & QUYẾN)

Tài liệu này tổng hợp toàn bộ các file, cấu trúc code, ý nghĩa từng dòng và mục đích của các Task đã thực hiện trong Tuần 1 thuộc đồ án PBL2.

---

## 1. Task 1.4 – Khai báo các Header dùng chung (Common Headers)

**Mục đích:** Định nghĩa các cấu trúc dữ liệu cơ bản và kiểu dữ liệu chuẩn (Contract) để cả Ý và Quyến cùng sử dụng mà không bị trùng lặp hay lệch kiểu dữ liệu.

---

### 📄 File 1: `include/Types.h`
**Mục đích:** Chứa các kiểu liệt kê (`enum class`) để quản lý loại thiết bị, phương tiện truyền dẫn, giao thức mạng...

```cpp
#ifndef TYPES_H
#define TYPES_H

#include <string>

// Định nghĩa các loại thiết bị mạng trong hệ thống
enum class NodeType {
    ROUTER,   // Thiết bị định tuyến
    HOST,     // Máy trạm / Máy tính đầu cuối
    SWITCH    // Bộ chuyển mạch
};

// Định nghĩa loại phương tiện truyền dẫn (cáp/sóng)
enum class MediaType {
    FIBER,    // Cáp quang
    COPPER,   // Cáp đồng
    WIRELESS  // Sóng không dây
};

// Định nghĩa giao thức truyền tải
enum class Protocol {
    TCP,      // Giao thức TCP (đảm bảo tin cậy)
    UDP       // Giao thức UDP (tốc độ cao)
};

// Định nghĩa loại gói tin
enum class PacketType {
    REAL_TIME, // Gói tin thời gian thực (như thoại, video)
    BULK_DATA, // Gói tin dữ liệu lớn (như file download)
    CONTROL    // Gói tin điều khiển hệ thống
};

// Định nghĩa loại sự kiện xảy ra trên đường truyền (cho các tuần sau)
enum class EventType {
    LINK_DOWN,    // Sự kiện đứt cáp / hỏng đường truyền
    LINK_UP,      // Sự kiện khôi phục đường truyền
    LOAD_UPDATE,  // Sự kiện cập nhật tải trên đường truyền
    DELAY_UPDATE  // Sự kiện cập nhật độ trễ
};

#endif // TYPES_H
```

---

### 📄 File 2: `include/Config.h`
**Mục đích:** Đặt các hằng số hệ thống chung, giúp dễ chỉnh sửa thông số sau này mà không cần sửa rải rác trong code.

```cpp
#ifndef CONFIG_H
#define CONFIG_H

namespace Config {
    // Đơn vị quy ước trong toàn bộ hệ thống:
    // Thời gian: giây (s) | Dung lượng: byte | Băng thông: Mbps | Khoảng cách: mét (m)

    const double MAX_LOAD = 1.0;               // Tải tối đa của đường truyền (100%)
    const double DEFAULT_TERRAIN_FACTOR = 1.0; // Hệ số địa hình mặc định (1.0 = đồng bằng)
}

#endif // CONFIG_H
```

---

### 📄 File 3: `include/Node.h`
**Mục đích:** Định nghĩa cấu trúc lưu trữ thông tin của một Nút mạng (Node).

```cpp
#ifndef NODE_H
#define NODE_H

#include <string>
#include "Types.h"

struct Node {
    int id;                 // Định danh duy nhất của Node (VD: 0, 1, 2...)
    std::string name;       // Tên hiển thị (VD: "R1", "H1")
    NodeType type;          // Loại Node (ROUTER, HOST, SWITCH)
    int totalPorts;         // Tổng số cổng (port) tối đa thiết bị hỗ trợ
    int usedPorts;          // Số cổng hiện tại đã bị chiếm dụng
    double processingTime;  // Thời gian xử lý gói tin của nút (tính bằng giây)

    // Constructor mặc định khởi tạo giá trị an toàn
    Node() : id(-1), type(NodeType::ROUTER), totalPorts(0), usedPorts(0), processingTime(0.0) {}
};

#endif // NODE_H
```

---

### 📄 File 4: `include/Edge.h`
**Mục đích:** Định nghĩa cấu trúc cho một Cạnh / Đường truyền (Edge) kết nối giữa 2 Node.

```cpp
#ifndef EDGE_H
#define EDGE_H

#include "Types.h"

struct Edge {
    int id;                   // ID đường truyền
    int u;                    // ID node gốc
    int v;                    // ID node đích
    MediaType mediaType;      // Loại cáp (FIBER, COPPER, WIRELESS)
    double length;            // Chiều dài cáp (m)
    double bandwidthMbps;     // Băng thông (Mbps)
    double maxSegmentLength;  // Chiều dài tối đa cho phép của 1 đoạn cáp (m)
    double unitPrice;         // Đơn giá vật tư trên mỗi mét
    double terrainFactor;     // Hệ số địa hình (núi, sông...)
    double equipmentCost;     // Chi phí thiết bị đầu cuối
    double maintenanceCost;   // Chi phí bảo trì
    int mtu;                  // Kích thước gói tin tối đa (Maximum Transmission Unit)
    
    // Các trạng thái động (thay đổi khi mô phỏng)
    double currentLoad;       // Mức tải hiện tại (từ 0.0 đến 1.0)
    double queueDelay;        // Độ trễ hàng chờ (giây)
    bool isUp;                // Trạng thái đường truyền: true = hoạt động, false = hỏng
    bool isBackup;            // Đã được chọn làm đường dự phòng chưa
    bool isBuilt;             // Đã được chọn xây dựng trong cây khung (MST) chưa

    // Constructor giá trị mặc định
    Edge() : id(-1), u(-1), v(-1), mediaType(MediaType::COPPER), length(0.0), 
             bandwidthMbps(0.0), maxSegmentLength(0.0), unitPrice(0.0), 
             terrainFactor(1.0), equipmentCost(0.0), maintenanceCost(0.0), 
             mtu(1500), currentLoad(0.0), queueDelay(0.0), 
             isUp(true), isBackup(false), isBuilt(false) {}
};

#endif // EDGE_H
```

---

### 📄 File 5: `include/Packet.h`
**Mục đích:** Định nghĩa gói tin dữ liệu luân chuyển trong mạng.

```cpp
#ifndef PACKET_H
#define PACKET_H

#include "Types.h"

struct Packet {
    int id;               // ID gói tin
    int source;           // ID node gửi
    int destination;      // ID node nhận
    int sizeBytes;        // Kích thước gói tin (bytes)
    PacketType type;      // Loại gói tin (REAL_TIME, BULK_DATA, CONTROL)
    Protocol protocol;    // Giao thức (TCP, UDP)
    bool canFragment;     // Có cho phép phân mảnh gói tin khi MTU nhỏ không
    bool newConnection;   // Có phải gói mở kết nối mới không

    Packet() : id(-1), source(-1), destination(-1), sizeBytes(0), 
               type(PacketType::BULK_DATA), protocol(Protocol::UDP), 
               canFragment(true), newConnection(false) {}
};

#endif // PACKET_H
```

---

## 2. Task A1.1 – Tạo khung rỗng (Stub) cho Graph

**Mục đích:** Khai báo lớp `Graph` để Ý có thể gọi hàm `addNode`, `addEdge` ngay, còn phần thuật toán chi tiết bên trong sẽ do Quyến cài đặt sau.

---

### 📄 File 6: `include/Graph.h`
**Mục đích:** Khai báo giao diện của Đồ thị (dùng Danh sách kề và Hash map).

```cpp
#ifndef GRAPH_H
#define GRAPH_H

#include <vector>
#include <unordered_map>
#include <stdexcept>
#include "Node.h"
#include "Edge.h"

class Graph {
private:
    std::unordered_map<int, Node> nodes; // Lưu Node theo cặp <nodeId, Node>
    std::unordered_map<int, Edge> edges; // Lưu Edge theo cặp <edgeId, Edge>
    
    // Danh sách kề: Với mỗi nodeId, lưu danh sách các edgeId kết nối với nó
    std::unordered_map<int, std::vector<int>> adjacency;

public:
    Graph();
    ~Graph();

    void addNode(const Node& node);                           // Thêm nút
    void addEdge(const Edge& edge);                           // Thêm cạnh
    Node getNode(int nodeId) const;                           // Truy vấn nút theo ID
    Edge getEdge(int edgeId) const;                           // Truy vấn cạnh theo ID
    std::vector<int> getNeighbors(int nodeId) const;           // Lấy các cạnh kề
    int otherEndpoint(int nodeId, int edgeId) const;          // Lấy nút ở đầu kia của cạnh
    void buildAdjacency();                                    // Xây dựng danh sách kề
    int V() const;                                            // Trả về số lượng nút
    int E() const;                                            // Trả về số lượng cạnh
};

#endif // GRAPH_H
```

---

### 📄 File 7: `src/Graph.cpp`
**Mục đích:** Triển khai các hàm rỗng cơ bản nhất để dự án biên dịch thành công mà không bị lỗi thiếu hàm (`undefined reference`).

```cpp
#include "Graph.h"

Graph::Graph() {}
Graph::~Graph() {}

void Graph::addNode(const Node& node) {
    nodes[node.id] = node; // Thêm node vào map
}

void Graph::addEdge(const Edge& edge) {
    edges[edge.id] = edge; // Thêm edge vào map
}

Node Graph::getNode(int nodeId) const {
    auto it = nodes.find(nodeId);
    if (it != nodes.end()) return it->second;
    throw std::runtime_error("Node not found!"); // Ném lỗi nếu tìm không thấy ID
}

Edge Graph::getEdge(int edgeId) const {
    auto it = edges.find(edgeId);
    if (it != edges.end()) return it->second;
    throw std::runtime_error("Edge not found!");
}

std::vector<int> Graph::getNeighbors(int nodeId) const {
    auto it = adjacency.find(nodeId);
    if (it != adjacency.end()) return it->second;
    return {};
}

int Graph::otherEndpoint(int nodeId, int edgeId) const { return -1; }
void Graph::buildAdjacency() {}
int Graph::V() const { return nodes.size(); }
int Graph::E() const { return edges.size(); }
```

---

## 3. Tasks B1.1, B1.2, B1.3 – Module IOManager & Validation (Nhiệm vụ của Ý)

**Mục đích:** Đọc dữ liệu từ file văn bản `.txt`, loại bỏ dòng nhiễu/comment, ép kiểu dữ liệu string sang Enum, kiểm tra tính hợp lệ dữ liệu và thêm vào Đồ thị.

---

### 📄 File 8: `include/Validation.h` & `src/Validation.cpp`
**Mục đích:** Kiểm định dữ liệu. Nếu phát hiện dữ liệu phi logic (như số âm, node không tồn tại), sẽ từ chối (`REJECT`) và giải thích rõ nguyên nhân.

```cpp
#include "Validation.h"

namespace Validation {

// Kiểm tra tính hợp lệ của Node
bool isValidNode(const Node& node, std::string& reason) {
    if (node.totalPorts < 0) {
        reason = "totalPorts không được âm"; // Gán lý do từ chối
        return false;                      // Trả về không hợp lệ
    }
    return true;
}

// Kiểm tra tính hợp lệ của Edge
bool isValidEdge(const Edge& edge, const Graph& graph, std::string& reason) {
    // 1. Kiểm tra Node u có tồn tại trong Đồ thị chưa
    try {
        graph.getNode(edge.u);
    } catch (...) {
        reason = "Node u (" + std::to_string(edge.u) + ") không tồn tại";
        return false;
    }
    
    // 2. Kiểm tra Node v có tồn tại trong Đồ thị chưa
    try {
        graph.getNode(edge.v);
    } catch (...) {
        reason = "Node v (" + std::to_string(edge.v) + ") không tồn tại";
        return false;
    }

    // 3. Kiểm tra băng thông phải > 0
    if (edge.bandwidthMbps <= 0) {
        reason = "bandwidthMbps phải lớn hơn 0";
        return false;
    }

    // 4. Kiểm tra MTU phải > 0
    if (edge.mtu <= 0) {
        reason = "mtu phải lớn hơn 0";
        return false;
    }

    // 5. Kiểm tra tải hiện tại phải nằm trong khoảng [0.0, 1.0]
    if (edge.currentLoad < 0 || edge.currentLoad > 1) {
        reason = "currentLoad phải nằm trong khoảng [0, 1]";
        return false;
    }
    return true;
}

// Kiểm tra gói tin Packet
bool isValidPacket(const Packet& packet, std::string& reason) {
    if (packet.sizeBytes < 0) {
        reason = "sizeBytes không được âm";
        return false;
    }
    return true;
}

} // namespace Validation
```

---

### 📄 File 9: `include/IOManager.h` & `src/IOManager.cpp`
**Mục đích:** Module chính đảm nhận đọc file TXT, xử lý chuỗi và chuyển thành dữ liệu trong chương trình.

```cpp
#include "IOManager.h"
#include "Validation.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <cctype>

// Hàm kiểm tra xem dòng có phải là dòng trống hoặc comment không
bool IOManager::isSkippableLine(const std::string& line) {
    if (line.empty()) return true; // Dòng trống -> bỏ qua
    for (char c : line) {
        if (!std::isspace(c)) {   // Tìm ký tự đầu tiên không phải khoảng trắng
            if (c == '#') return true; // Nếu là '#' -> dòng comment -> bỏ qua
            return false;              // Là dữ liệu hợp lệ -> không bỏ qua
        }
    }
    return true;
}

// Hàm ép kiểu String -> Enum NodeType (Task B1.2)
NodeType IOManager::stringToNodeType(const std::string& str) {
    if (str == "ROUTER") return NodeType::ROUTER;
    if (str == "HOST") return NodeType::HOST;
    if (str == "SWITCH") return NodeType::SWITCH;
    throw std::invalid_argument("Unknown NodeType: " + str); // Bắt lỗi gõ sai chữ
}

// Hàm đọc danh sách Node từ file TXT (Task B1.1)
bool IOManager::loadNodes(const std::string& filepath, Graph& graph) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Lỗi: Không thể mở file " << filepath << "\n";
        return false;
    }

    std::string line;
    int lineCount = 0;
    while (std::getline(file, line)) { // Đọc từng dòng
        lineCount++;
        if (isSkippableLine(line)) continue; // Bỏ qua comment/dòng trống

        std::stringstream ss(line);
        Node node;
        std::string typeStr;

        // Tách các trường dữ liệu ngăn cách bởi dấu space
        if (!(ss >> node.id >> node.name >> typeStr >> node.totalPorts >> node.processingTime)) {
            std::cerr << "Lỗi sai format dòng " << lineCount << " trong " << filepath << "\n";
            continue;
        }

        // Chuyển String sang Enum
        try {
            node.type = stringToNodeType(typeStr);
        } catch (const std::exception& e) {
            std::cerr << "Lỗi dữ liệu Enum dòng " << lineCount << ": " << e.what() << "\n";
            continue;
        }

        // Kiểm định dữ liệu qua Validation trước khi thêm vào Graph
        std::string reason;
        if (!Validation::isValidNode(node, reason)) {
            std::cerr << "REJECT Node ID " << node.id << ": " << reason << "\n";
            continue;
        }

        graph.addNode(node); // Thêm thành công vào Đồ thị
    }
    return true;
}
```

---

## 4. Dữ liệu thử nghiệm & Chương trình Main

### 📄 Các file trong thư mục `data/`
- **`data/nodes.txt`**: Danh sách nút mẫu (có 1 dòng cố tình gõ sai `totalPorts = -1` để test).
- **`data/edges.txt`**: Danh sách cạnh mẫu (có các dòng cố tình lỗi: trỏ node 9 không có thực, băng thông âm, tải > 1.0).
- **`data/packets.txt`**: Danh sách gói tin mẫu (có gói tin kích thước âm).

### 📄 File 10: `main.cpp`
**Mục đích:** File chạy chính để gọi các module đọc file và in kết quả ra màn hình.

```cpp
#include <iostream>
#include <vector>
#include "Graph.h"
#include "IOManager.h"

int main() {
    std::cout << "=== KHOI TAO DO THI PBL2 ===\n";
    Graph graph;
    
    // 1. Nạp Nodes
    std::cout << "\n--- Load Nodes ---\n";
    if (IOManager::loadNodes("data/nodes.txt", graph)) {
        std::cout << "=> So luong node hien tai: " << graph.V() << "\n";
    }
    
    // 2. Nạp Edges
    std::cout << "\n--- Load Edges ---\n";
    if (IOManager::loadEdges("data/edges.txt", graph)) {
        std::cout << "=> So luong edge hien tai: " << graph.E() << "\n";
    }
    
    // 3. Nạp Packets
    std::cout << "\n--- Load Packets ---\n";
    std::vector<Packet> packets;
    if (IOManager::loadPackets("data/packets.txt", packets)) {
        std::cout << "=> So luong packet load thanh cong: " << packets.size() << "\n";
    }

    std::cout << "\n=== KET THUC KHOI TAO ===\n";
    return 0;
}
```
1️⃣ Đầu vào, đầu ra và vị trí trong 11 bước?
Vị trí: Nằm ở Bước 1 (Khởi tạo và nạp dữ liệu mạng) trong quy trình 11 bước của bài toán.
Đầu vào: Các file văn bản nodes.txt, edges.txt, packets.txt.
Đầu ra: Đối tượng Graph đã được nạp danh sách Node và Edge hợp lệ, cùng danh sách std::vector<Packet> sạch.
2️⃣ Công thức nào được dùng và vì sao có mặt trong bài?
Công thức/Quy tắc: Kiểm tra miền giá trị hợp lệ: $$0.0 \le \text{currentLoad} \le 1.0$$ $$\text{bandwidthMbps} > 0, \quad \text{mtu} > 0, \quad \text{totalPorts} \ge 0, \quad \text{sizeBytes} \ge 0$$ $$u, v \in V \quad (\text{Node } u \text{ và } v \text{ phải tồn tại trong tập Node})$$
Vì sao có mặt: Theo nguyên lý "Garbage In, Garbage Out" (Dữ liệu vào rác thì kết quả ra rác). Nếu không lọc sạch dữ liệu ngay từ Bước 1, các thuật toán Kruskal (Tuần 2), Dijkstra (Tuần 3), và Phân mảnh gói tin (Tuần 4) sẽ bị crash hoặc chạy sai kết quả.
3️⃣ Nếu đổi một tham số thì kết quả thay đổi thế nào và vì sao?
Kịch bản: Trong file edges.txt, nếu sửa v của Edge ID 3 từ node 2 thành node 9 (chưa từng khai báo trong nodes.txt), hoặc sửa bandwidthMbps từ 1000 thành -10.
Kết quả: Module Validation sẽ lập tức chặn lại, ngắt không cho thêm Edge đó vào Graph, và in ra cảnh báo REJECT Edge ID 3: Node v (9) không tồn tại hoặc REJECT Edge ID 4: bandwidthMbps phải lớn hơn 0.
Vì sao: Nhờ cơ chế kiểm tra tham chiếu chéo (graph.getNode()) và kiểm tra điều kiện biên trước khi gọi graph.addEdge().
