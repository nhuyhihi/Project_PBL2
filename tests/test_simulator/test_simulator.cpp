#include <iostream>
#include <cassert>
#include <cmath>
#include "../../include/Simulator.h"
#include "../../include/Graph.h"
#include "../../include/Packet.h"
#include "../../include/PathResult.h"

// Hàm test tự chế nhỏ gọn cho Task B4.1 -> B4.4
void test_Week4_Ý() {
    std::cout << "--- BAT DAU TEST TUAN 4 (Y) ---\n";

    // 1. Tạo đồ thị giả lập
    Graph g;
    Node n1; n1.id = 1;
    Node n2; n2.id = 2;
    g.addNode(n1);
    g.addNode(n2);

    Edge e1;
    e1.id = 1;
    e1.u = 1;
    e1.v = 2;
    e1.currentLoad = 0.2;
    e1.queueDelay = 0.01;
    g.addEdge(e1);

    // 2. Test Snapshot (Task B4.1 & B4.2)
    NetworkSnapshot snap1 = Simulator::snapshotNetworkState(g);
    assert(snap1.load[1] == 0.2 && "Loi: Snapshot khong luu dung Load ban dau");
    assert(snap1.queueDelay[1] == 0.01 && "Loi: Snapshot khong luu dung Delay ban dau");
    std::cout << "[PASS] NetworkSnapshot hoat dong dung.\n";

    // 3. Test applyEvent LOAD_UPDATE (Task B4.4)
    ScenarioEvent evLoad = {EventType::LOAD_UPDATE, 1, 0.85, 1.0};
    Simulator::applyEvent(g, evLoad);
    
    assert(g.getEdge(1).currentLoad == 0.85 && "Loi: applyEvent khong cap nhat dung Load");
    
    // Kiểm tra tính độc lập của Snapshot (đồ thị đổi nhưng snapshot cũ phải đứng yên)
    assert(snap1.load[1] == 0.2 && "Loi: Snapshot bi thay doi khi Graph thay doi (Loi kien truc)");
    std::cout << "[PASS] applyEvent (LOAD_UPDATE) + Tinh doc lap cua Snapshot.\n";

    // 4. Test chặn lề giá trị của applyEvent (Bảo vệ dữ liệu)
    ScenarioEvent evLoadNham = {EventType::LOAD_UPDATE, 1, 5.0, 2.0}; // Cố tình truyền Load = 5.0
    Simulator::applyEvent(g, evLoadNham);
    assert(g.getEdge(1).currentLoad == 1.0 && "Loi: applyEvent khong chan gia tri Load > 1.0");
    std::cout << "[PASS] applyEvent chan gia tri Load hop le (max 1.0).\n";

    // 5. Test applyEvent DELAY_UPDATE
    ScenarioEvent evDelay = {EventType::DELAY_UPDATE, 1, 0.05, 3.0};
    Simulator::applyEvent(g, evDelay);
    assert(g.getEdge(1).queueDelay == 0.05 && "Loi: applyEvent khong cap nhat dung Delay");
    std::cout << "[PASS] applyEvent (DELAY_UPDATE) hoat dong dung.\n";

    std::cout << "--- TAT CA TEST TUAN 4 (Y) PHAN 1 DEU PASS ---\n\n";
}

// Hàm test riêng cho sendPacket và updateNetworkState (Task B4.5 & B4.6)
void test_sendPacket_and_updateLoad() {
    std::cout << "--- BAT DAU TEST SEND PACKET (TASK B4.5 & B4.6) ---\n";
    
    // 1. Setup một mạng mini
    Graph g;
    Node n1; n1.id = 1; g.addNode(n1);
    Node n2; n2.id = 2; g.addNode(n2);
    
    Edge e1; 
    e1.id = 1; 
    e1.u = 1; 
    e1.v = 2; 
    e1.bandwidthMbps = 100.0; 
    e1.mtu = 1500; 
    e1.currentLoad = 0.0; // Sửa thành 0.0 để test Load tăng lên rõ ràng, tránh bị tụt do hệ số Decay 0.99
    g.addEdge(e1);

    // 2. Setup một gói tin hợp lệ
    Packet pkt;
    pkt.sizeBytes = 1000; // Nhỏ hơn MTU
    pkt.protocol = Protocol::UDP;
    pkt.canFragment = false;

    // 3. Setup đường đi (Giả lập kết quả trả về từ thuật toán Dijkstra của Quyến)
    PathResult path;
    path.reachable = true;
    path.nodes = {1, 2};
    path.edges = {1};

    double initialLoad = g.getEdge(1).currentLoad;

    // --- TEST 1: Gửi thành công và Cập nhật Load ---
    SimulationResult res = Simulator::sendPacket(pkt, path, g);
    
    assert(res.success == true && "Loi: sendPacket that bai voi duong di hop le va goi tin nho.");
    // Sau khi gửi, dTrans sinh ra loadDelta, làm tăng currentLoad
    assert(g.getEdge(1).currentLoad > initialLoad && "Loi: Load (tai) khong tang sau khi gui goi tin! (updateNetworkState chua chay)");
    std::cout << "[PASS] Giao thuc sendPacket thanh cong. Load mang da tang len sau khi truyen.\n";

    // --- TEST 2: Lỗi vượt rào MTU ---
    Packet pktLarge;
    pktLarge.sizeBytes = 2000; // Lớn hơn MTU (1500)
    pktLarge.canFragment = false; // Cấm phân mảnh
    pktLarge.protocol = Protocol::UDP;

    SimulationResult resFail = Simulator::sendPacket(pktLarge, path, g);
    assert(resFail.success == false && "Loi: Khong chan (Drop) goi tin co kich thuoc lon hon MTU ma khong the phan manh.");
    std::cout << "[PASS] sendPacket da bat duoc loi MTU va chan goi tin thanh cong.\n";

    // --- TEST 3: Dijkstra báo lỗi (reachable = false) ---
    PathResult badPath;
    badPath.reachable = false;
    SimulationResult resBadRoute = Simulator::sendPacket(pkt, badPath, g);
    assert(resBadRoute.success == false && resBadRoute.failReason == "NO_ROUTE_AVAILABLE" && "Loi: Van gui duoc khi khong co duong.");
    std::cout << "[PASS] sendPacket bao loi NO_ROUTE_AVAILABLE khi Dijkstra khong tim duoc duong.\n";

    std::cout << "--- TAT CA TEST TUAN 4 (Y) PHAN 2 DEU PASS ---\n";
}

int main() {
    test_Week4_Ý();
    test_sendPacket_and_updateLoad();
    std::cout << "\n>>> CHUC MUNG! BAN DA PASS HET CAC YEU CAU CUA TUAN 4! <<<\n";
    return 0;
}
