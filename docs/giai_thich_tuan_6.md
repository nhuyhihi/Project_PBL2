# BÁO CÁO VÀ GIẢI THÍCH CHI TIẾT TẤT CẢ CÁC FILE TUẦN 6
**Chủ đề:** Tích Hợp Hệ Thống + Menu Điều Khiển + Reporter + 6 Bộ Demo + Logging + Kiến Trúc Chuẩn (Final Integration & Documentation)

---

Tài liệu này tổng hợp toàn bộ các đoạn code quan trọng nhất của **Tuần 6**, kèm comment giải thích chi tiết ngay tại từng dòng code và phân tích kiến trúc tích hợp toàn diện của đồ án PBL2.

---

## 1. Task B6.1 – Tinh gọn `main.cpp` theo mô hình Orchestrator

* **File:** [main.cpp](file:///d:/PBL2/Project_PBL2/main.cpp)
* **Ý nghĩa:** Tuân thủ triệt để nguyên lý Single Responsibility. `main.cpp` đóng vai trò là nhạc trưởng điều phối: nạp dữ liệu, khởi tạo MST & Backup, cung cấp Menu tương tác và gọi các module độc lập. Mọi logic thuật toán nặng đều được đưa về các file chuyên trách.

```cpp
#include <iostream>
#include <vector>
#include <string>
#include "Graph.h"
#include "IOManager.h"
#include "Kruskal.h"
#include "CostModel.h"
#include "Reporter.h"
#include "Simulator.h"
#include "Scenario.h"

int main() {
    // BƯỚC 1: NẠP DỮ LIỆU CỐT LÕI VÀ KIỂM TRA HỢP LỆ (DEFENSIVE INPUT)
    Graph baseGraph;
    // Đọc danh sách Node và Cạnh từ file cấu hình, báo lỗi ngay nếu file hỏng
    if (!IOManager::loadNodes("data/nodes.txt", baseGraph) ||
        !IOManager::loadEdges("data/edges.txt", baseGraph)) {
        std::cerr << "Loi: Khong the khoi tao do thi ban dau!\n";
        return 1;
    }

    // Đọc danh sách các gói tin cần mô phỏng truyền dẫn
    std::vector<Packet> packets;
    IOManager::loadPackets("data/packets.txt", packets);

    // BƯỚC 2: NẠP BẢNG GIÁ VẬT TƯ VÀ QUY HOẠCH HẠ TẦNG (MST + BACKUP)
    CostModel pricingModel;
    pricingModel.loadPricing("data/pricing.txt"); // Nạp bảng đơn giá thực tế

    // Tạo lambda hàm chi phí cáp vật lý để truyền vào thuật toán Kruskal
    auto costFn = [&pricingModel](const Edge& e) { return pricingModel.calculateCablingCost(e).total; };
    
    // Xây dựng cây khung nhỏ nhất (MST) tối ưu hóa chi phí lắp đặt cáp
    MSTResult mstResult = buildMST(baseGraph, costFn);

    // Lựa chọn 2 tuyến cáp dự phòng chiến lược (Backup Links) phòng ngừa sự cố đứt mạng
    selectBackupLinks(baseGraph, mstResult, 2, costFn);

    std::cout << "=> He thong mang da san sang: " << baseGraph.V() << " nodes, " 
              << baseGraph.E() << " edges, " << packets.size() << " packets.\n";

    // BƯỚC 3: VÒNG LẶP MENU TƯƠNG TÁC ĐIỀU PHỐI (TASK B6.2)
    std::string inputLine;
    while (true) {
        displayMenu(); // In bảng danh sách 9 chức năng lựa chọn
        if (!std::getline(std::cin, inputLine)) {
            std::cout << "\n[EOF] Ket thuc chuong trinh.\n";
            break;
        }

        // Xử lý chuỗi nhập an toàn: Cắt bỏ ký tự xuống dòng '\r', khoảng trắng thừa trên Windows
        while (!inputLine.empty() && (inputLine.back() == '\r' || inputLine.back() == ' ' || inputLine.back() == '\t')) {
            inputLine.pop_back();
        }
        while (!inputLine.empty() && (inputLine.front() == ' ' || inputLine.front() == '\t')) {
            inputLine.erase(inputLine.begin());
        }

        if (inputLine.empty()) continue; // Bỏ qua nếu người dùng ấn Enter rỗng

        // Chuyển chuỗi thành số nguyên có try-catch bảo vệ
        int choice = -1;
        try { choice = std::stoi(inputLine); } catch (...) { choice = -1; }

        if (choice == 0) {
            std::cout << "\nCam on ban da su dung chuong trinh! Tam biet!\n";
            break;
        }

        // Điều phối thực thi tác vụ theo lựa chọn của người dùng
        switch (choice) {
            case 1: runSelectedScenario(baseGraph, packets, "data/scenarios/basic.txt", "log_basic.txt"); break;
            case 2: runSelectedScenario(baseGraph, packets, "data/scenarios/qos.txt", "log_qos.txt"); break;
            case 3: runSelectedScenario(baseGraph, packets, "data/scenarios/congestion.txt", "log_congestion.txt"); break;
            case 4: runSelectedScenario(baseGraph, packets, "data/scenarios/mtu.txt", "log_mtu.txt"); break;
            case 5: runSelectedScenario(baseGraph, packets, "data/scenarios/failure.txt", "log_failure.txt"); break;
            case 6: runSelectedScenario(baseGraph, packets, "data/scenarios/redundancy.txt", "log_redundancy.txt"); break;
            case 7: Reporter::printMSTReport(baseGraph, pricingModel); break;     // In báo cáo cáp
            case 8: Reporter::printFullRoutingTable(baseGraph); break;            // In bảng định tuyến toàn mạng
            default: std::cout << "Lua chon khong hop le! Vui long nhap so tu 0 den 8.\n"; break;
        }

        std::cout << "\nNhan Enter de tiep tuc...";
        std::string pauseLine;
        std::getline(std::cin, pauseLine);
    }

    return 0;
}
```

---

## 2. Task B6.2 – Cô lập trạng thái mạng trước mỗi lần chạy (`Network State Isolation`)

* **File:** [main.cpp](file:///d:/PBL2/Project_PBL2/main.cpp) (hàm `runSelectedScenario`)
* **Ý nghĩa:** Tránh ô nhiễm trạng thái giữa các lần chạy. Nếu chạy kịch bản Đứt cáp (`failure.txt`) làm `isUp = false`, khi người dùng chọn tiếp kịch bản `qos.txt` thì mạng phải quay về nguyên vẹn như lúc ban đầu chứ không bị giữ lại vết đứt cáp cũ.

```cpp
void runSelectedScenario(Graph& baseGraph, 
                         const std::vector<Packet>& packets, 
                         const std::string& scenarioFile, 
                         const std::string& logFileName = "") {
    // ĐIỂM SÁNG KIẾN TRÚC: Luôn tạo bản sao độc lập từ baseGraph gốc
    // Nhờ đó, mọi biến đổi do sự kiện (đứt cáp isUp=false, nghẽn tải load=95%) 
    // chỉ xảy ra trên testGraph cục bộ và sẽ tự động được giải phóng khi kịch bản kết thúc
    Graph testGraph = baseGraph;

    // Nạp dữ liệu các mốc thời gian và sự kiện từ file kịch bản
    Scenario sc;
    if (!IOManager::loadScenario(scenarioFile, sc)) {
        std::cerr << "Loi: Khong the doc file kich ban " << scenarioFile << "\n";
        return;
    }

    // Thiết lập đường dẫn lưu file log văn bản
    std::string logPath = "";
    if (!logFileName.empty()) {
        logPath = "data/logs/" + logFileName;
    }

    // Kích hoạt bộ máy mô phỏng thực thi kịch bản độc lập
    Simulator::runScenarioSimulation(testGraph, packets, sc, logPath);
}
```

---

## 3. Task B6.3 – Module Báo Cáo Hạ Tầng & Bảng Định Tuyến (`Reporter`)

* **File:** [src/Reporter.cpp](file:///d:/PBL2/Project_PBL2/src/Reporter.cpp)
* **Ý nghĩa:** Cung cấp 2 báo cáo kỹ thuật cốt lõi:
  1. `printMSTReport`: So sánh tổng chi phí của 3 phương án hạ tầng (*Xây toàn bộ* vs *Chỉ xây MST* vs *MST + Backup*).
  2. `printFullRoutingTable`: Bảng định tuyến chi tiết cho tất cả các node trong đồ thị.

```cpp
// 1. In báo cáo tổng hợp quy hoạch cáp hạ tầng
void printMSTReport(const Graph& graph, const CostModel& costModel) {
    std::cout << "\n========================================================================\n";
    std::cout << "               BAO CAO QUY HOACH CAP MANG (MST & BACKUP)                \n";
    std::cout << "========================================================================\n";

    double totalAllCost = 0.0;    // Tổng chi phí nếu xây dựng mù quáng tất cả các cạnh
    double totalMSTCost = 0.0;    // Chi phí tối thiểu khi chỉ xây các cạnh thuộc MST
    double totalBackupCost = 0.0; // Chi phí đầu tư thêm cho các tuyến cáp dự phòng

    // Duyệt qua toàn bộ cạnh trong mạng để phân loại và cộng dồn chi phí
    for (int edgeId : graph.edgeIds()) {
        const Edge& e = graph.getEdge(edgeId);
        double cost = costModel.calculateCablingCost(e).total;
        totalAllCost += cost;

        if (e.isBuilt) {
            if (e.isBackup) {
                totalBackupCost += cost; // Tuyến cáp được Kruskal chọn làm Backup
            } else {
                totalMSTCost += cost;    // Tuyến cáp thuộc khung xương sống MST
            }
        }
    }

    // In bảng so sánh định lượng giúp chủ đầu tư thấy rõ hiệu quả tiết kiệm chi phí
    std::cout << "  - Chi phi xay dung TOAN BO mang ban dau : " << totalAllCost << " VND\n";
    std::cout << "  - Chi phi tiet kiem chi xay cay KHUNG (MST) : " << totalMSTCost << " VND\n";
    std::cout << "  - Chi phi dau tu them cap DU PHONG (Backup): " << totalBackupCost << " VND\n";
    std::cout << "  - Tong chi phi thuc te da trien khai       : " << (totalMSTCost + totalBackupCost) << " VND\n";
    std::cout << "  => TIET KIEM DUOC SO VOI XAY TOAN BO        : " 
              << (totalAllCost - (totalMSTCost + totalBackupCost)) << " VND ("
              << (1.0 - (totalMSTCost + totalBackupCost) / totalAllCost) * 100 << "%)\n";
    std::cout << "========================================================================\n";
}

// 2. In bảng định tuyến hoàn chỉnh cho mọi Node (Full Routing Table)
void printFullRoutingTable(const Graph& graph) {
    std::cout << "\n========================================================================\n";
    std::cout << "                     BANG DINH TUYEN TOAN BO MANG                       \n";
    std::cout << "========================================================================\n";

    // Duyệt qua từng node nguồn trong hệ thống
    for (int src : graph.nodeIds()) {
        std::cout << "\n[ROUTING TABLE TAI NODE " << src << "]:\n";
        std::cout << "Dich (Dest) | Next Hop | Chi phi (Cost) | So chang (Hops) | Lo trinh day du\n";
        std::cout << "------------------------------------------------------------------------\n";

        for (int dest : graph.nodeIds()) {
            if (src == dest) continue; // Bỏ qua chính nó

            // Chạy Dijkstra tìm đường ngắn nhất chỉ trên các dây đã được cấp phép xây dựng (onlyBuilt = true)
            PathResult res = dijkstra(graph, src, dest, DijkstraWeights::lengthWeight, /*onlyBuilt=*/true);

            if (res.reachable) {
                // Next hop chính là node liền kề đầu tiên trên đường đi
                int nextHop = res.nodes[1]; 
                
                // Ghép chuỗi lộ trình trực quan dạng: 0 -> 2 -> 3
                std::string pathStr = "";
                for (size_t i = 0; i < res.nodes.size(); ++i) {
                    pathStr += std::to_string(res.nodes[i]) + (i + 1 < res.nodes.size() ? " -> " : "");
                }

                std::cout << std::left 
                          << std::setw(12) << dest 
                          << std::setw(11) << nextHop 
                          << std::setw(17) << res.totalCost 
                          << std::setw(18) << res.hopCount 
                          << pathStr << "\n";
            }
        }
    }
    std::cout << "========================================================================\n";
}
```

---

## 4. Task B6.5 – Hệ Thống Ghi Nhật Ký Mô Phỏng Ra File (`Logger`)

* **File:** [src/Logger.cpp](file:///d:/PBL2/Project_PBL2/src/Logger.cpp)
* **Ý nghĩa:** Tự động định dạng và xuất bảng theo dõi 10 cột ra file văn bản `.txt` tại `data/logs/` (như `log_basic.txt`, `log_failure.txt`...). Giúp đối chiếu, kiểm tra sau mô phỏng mà không bị mất dấu vết.

```cpp
bool exportSimulationLog(const std::string& filepath,
                         const std::string& scenarioName,
                         const std::vector<SimulationLogEntry>& logs) {
    // Mở luồng ghi file văn bản
    std::ofstream outFile(filepath);
    if (!outFile.is_open()) {
        std::cerr << "Loi: Khong the mo hoac tao file log: " << filepath << "\n";
        return false;
    }

    // In phần đầu tiêu đề của file báo cáo
    outFile << "========================================================================================================\n";
    outFile << "                               NHAT KY MO PHONG MANG (SIMULATION LOG)                                   \n";
    outFile << "Kich ban (Scenario) : " << scenarioName << "\n";
    outFile << "Tong so ban ghi log : " << logs.size() << "\n";
    outFile << "========================================================================================================\n\n";

    // Canh lề chuẩn hóa 10 cột theo định dạng bảng báo cáo kỹ thuật
    outFile << std::left
            << std::setw(8)  << "Time(s)"
            << std::setw(8)  << "Packet"
            << std::setw(6)  << "Src"
            << std::setw(6)  << "Dst"
            << std::setw(6)  << "Hop"
            << std::setw(8)  << "Frag"
            << std::setw(12) << "Delay(s)"
            << std::setw(10) << "Status"
            << std::setw(20) << "Path"
            << std::setw(28) << "Failure / Note"
            << "\n";
    outFile << "--------------------------------------------------------------------------------------------------------\n";

    // Duyệt qua từng bản ghi nhật ký mô phỏng
    for (const auto& entry : logs) {
        std::string delayStr = "-";
        if (entry.status == "SUCCESS") {
            // Định dạng số thực 6 chữ số thập phân cho độ trễ
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%.6f", entry.delay);
            delayStr = buf;
        }

        // Tạo chuỗi ghi chú tổng hợp sự cố và chuyển tuyến
        std::string note = "";
        if (!entry.failureEvent.empty()) note = entry.failureEvent;
        if (entry.status != "SUCCESS" && !entry.dropReason.empty()) {
            if (!note.empty()) note += " | ";
            note += "DROP: " + entry.dropReason;
        } else if (entry.isRerouted) {
            if (!note.empty()) note += " | ";
            note += "[REROUTED]"; // Đánh dấu gói tin đã đổi hướng tránh sự cố
        }

        // Ghi dòng dữ liệu tương ứng vào file
        outFile << std::left
                << std::setw(8)  << std::fixed << std::setprecision(1) << entry.timestamp
                << std::setw(8)  << ("#" + std::to_string(entry.packetId))
                << std::setw(6)  << entry.source
                << std::setw(6)  << entry.destination
                << std::setw(6)  << (entry.status == "SUCCESS" ? std::to_string(entry.hopCount) : "-")
                << std::setw(8)  << entry.fragmentCount
                << std::setw(12) << delayStr
                << std::setw(10) << entry.status
                << std::setw(20) << entry.path
                << note
                << "\n";
    }

    outFile << "--------------------------------------------------------------------------------------------------------\n";
    outFile << "Ket thuc nhat ky mo phong.\n";
    outFile.close();
    return true; // Ghi file thành công
}
```

---

## 5. Hướng dẫn biên dịch và kiểm tra chương trình

```bash
# Biên dịch toàn bộ mã nguồn C++17 bằng trình biên dịch MinGW g++:
g++ -std=c++17 -Iinclude main.cpp src/*.cpp -o bin/main.exe

# Khởi chạy ứng dụng điều phối Menu:
.\bin\main.exe
```

* Nhập từ `1` đến `6` để chạy 6 kịch bản mô phỏng tương tác.
* Nhập `7` để xem phân tích tối ưu chi phí hạ tầng MST & Backup.
* Nhập `8` để xem bảng định tuyến toàn mạng.
* Mọi kết quả chạy đều được tự động lưu vết tại thư mục `data/logs/`.
