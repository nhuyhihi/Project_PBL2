// ============================================================
// Test A1.3: các file dữ liệu SAI phải bị Validation của IOManager loại đúng.
// Chạy từ thư mục gốc repo (đường dẫn file là tương đối):
//   g++ -std=c++17 -Wall -Wextra -Iinclude src/Graph.cpp src/IOManager.cpp src/Validation.cpp tests/unit/test_invalid_data.cpp -o test_invalid_data
//   ./test_invalid_data
// Các dòng "REJECT ..." in ra stderr là BÌNH THƯỜNG - đó chính là validation đang hoạt động.
// ============================================================
#include <cstdio>
#include <string>
#include <vector>
#include "Graph.h"
#include "IOManager.h"

static int g_pass = 0, g_fail = 0;
#define CHECK(cond)                                                          \
    do {                                                                     \
        if (cond) { ++g_pass; }                                              \
        else { ++g_fail; std::printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); } \
    } while (0)

// Đồ thị nền: 4 node hợp lệ id 0..3 để các edge test tham chiếu tới
static Graph baseGraph() {
    Graph g;
    for (int i = 0; i < 4; ++i) {
        Node n;
        n.id = i; n.type = NodeType::ROUTER; n.totalPorts = 4; n.processingTime = 0.001;
        g.addNode(n);
    }
    return g;
}

// Nạp 1 file edge vào đồ thị nền, trả về số edge được nhận
static int edgesAccepted(const std::string& file) {
    Graph g = baseGraph();
    IOManager::loadEdges("tests/data/invalid/" + file, g);
    return g.E();
}

int main() {
    std::puts("[Edge sai bị loại]");
    CHECK(edgesAccepted("edge_unknown_node.txt") == 0);     // v = 9 không tồn tại
    CHECK(edgesAccepted("edge_zero_bandwidth.txt") == 0);   // bandwidth = 0
    CHECK(edgesAccepted("edge_zero_mtu.txt") == 0);         // mtu = 0
    CHECK(edgesAccepted("edge_load_over_1.txt") == 0);      // load = 1.5 > 1

    std::puts("[Ca biên hợp lệ vẫn được nhận]");
    CHECK(edgesAccepted("edge_load_boundary_valid.txt") == 1);  // load = 1.0 là hợp lệ

    std::puts("[Node sai bị loại]");
    {
        Graph g;
        IOManager::loadNodes("tests/data/invalid/node_negative_ports.txt", g);
        CHECK(g.V() == 0);                                  // totalPorts = -1
    }

    std::puts("[Packet sai bị loại]");
    {
        std::vector<Packet> packets;
        IOManager::loadPackets("tests/data/invalid/packet_negative_size.txt", packets);
        CHECK(packets.empty());                             // sizeBytes = -100
    }

    std::printf("\nKet qua: %d dat, %d loi\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
