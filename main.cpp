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

void displayMenu() {
    std::cout << "\n==============================================================\n";
    std::cout << "        DO AN PBL2: THIET KE & MO PHONG MANG VIEN THONG       \n";
    std::cout << "                 MENU SCENARIO (TASK B6.2)                    \n";
    std::cout << "==============================================================\n";
    std::cout << "  [1] Scenario 1: BASIC       (Dieu kien mang ly tuong)\n";
    std::cout << "  [2] Scenario 2: QoS         (Uu tien dich vu REAL_TIME vs BULK)\n";
    std::cout << "  [3] Scenario 3: CONGESTION  (Tac nghen mang & tu dong ne duong)\n";
    std::cout << "  [4] Scenario 4: MTU         (Kiem tra phan manh Fragmentation)\n";
    std::cout << "  [5] Scenario 5: FAILURE     (Su co dut cap & tu dong reroute)\n";
    std::cout << "  [6] Scenario 6: REDUNDANCY  (Phan tich vai tro cap Backup)\n";
    std::cout << "  ------------------------------------------------------------\n";
    std::cout << "  [7] Xem Bao cao Quy hoach Cap (MST & Backup Links)\n";
    std::cout << "  [8] Xem Bang dinh tuyen toan mang (Full Routing Table)\n";
    std::cout << "  [0] Thoat chuong trinh\n";
    std::cout << "==============================================================\n";
    std::cout << "Vui long chon thao tac [0-8]: ";
}

void runSelectedScenario(Graph& baseGraph, const std::vector<Packet>& packets, const std::string& scenarioFile, const std::string& logFileName = "") {
    // Luôn copy đồ thị để mỗi lần chạy một kịch bản, mạng được bắt đầu ở trạng thái sạch ban đầu
    Graph testGraph = baseGraph;

    Scenario sc;
    if (!IOManager::loadScenario(scenarioFile, sc)) {
        std::cerr << "Loi: Khong the doc file kich ban " << scenarioFile << "\n";
        return;
    }

    std::string logPath = "";
    if (!logFileName.empty()) {
        logPath = "data/logs/" + logFileName;
    }

    Simulator::runScenarioSimulation(testGraph, packets, sc, logPath);
}

int main() {
    // 1. NẠP DỮ LIỆU CỐT LÕI VÀ KIỂM TRA HỢP LỆ
    Graph baseGraph;
    if (!IOManager::loadNodes("data/nodes.txt", baseGraph) ||
        !IOManager::loadEdges("data/edges.txt", baseGraph)) {
        std::cerr << "Loi: Khong the khoi tao do thi ban dau!\n";
        return 1;
    }

    std::vector<Packet> packets;
    IOManager::loadPackets("data/packets.txt", packets, &baseGraph);

    // 2. NẠP BẢNG GIÁ VÀ XÂY DỰNG MST + BACKUP
    CostModel pricingModel;
    pricingModel.loadPricing("data/pricing.txt");
    auto costFn = [&pricingModel](const Edge& e) { return pricingModel.calculateCablingCost(e).total; };
    
    MSTResult mstResult = buildMST(baseGraph, costFn);
    selectBackupLinks(baseGraph, mstResult, 2, costFn);

    std::cout << "=> He thong mang da san sang: " << baseGraph.V() << " nodes, " 
              << baseGraph.E() << " edges, " << packets.size() << " packets.\n";

    // 3. VÒNG LẶP MENU TƯƠNG TÁC 
    std::string inputLine;
    while (true) {
        displayMenu();
        if (!std::getline(std::cin, inputLine)) {
            std::cout << "\n[EOF] Ket thuc chuong trinh.\n";
            break;
        }

        // Loại bỏ khoảng trắng và \r thừa nếu có (đặc biệt khi pipe trên Windows)
        while (!inputLine.empty() && (inputLine.back() == '\r' || inputLine.back() == ' ' || inputLine.back() == '\t')) {
            inputLine.pop_back();
        }
        while (!inputLine.empty() && (inputLine.front() == ' ' || inputLine.front() == '\t')) {
            inputLine.erase(inputLine.begin());
        }

        if (inputLine.empty()) continue;

        int choice = -1;
        try {
            choice = std::stoi(inputLine);
        } catch (...) {
            choice = -1;
        }

        if (choice == 0) {
            std::cout << "\nCam on ban da su dung chuong trinh! Tam biet!\n";
            break;
        }

        switch (choice) {
            case 1:
                runSelectedScenario(baseGraph, packets, "data/scenarios/basic.txt", "log_basic.txt");
                break;
            case 2:
                runSelectedScenario(baseGraph, packets, "data/scenarios/qos.txt", "log_qos.txt");
                break;
            case 3:
                runSelectedScenario(baseGraph, packets, "data/scenarios/congestion.txt", "log_congestion.txt");
                break;
            case 4:
                runSelectedScenario(baseGraph, packets, "data/scenarios/mtu.txt", "log_mtu.txt");
                break;
            case 5:
                runSelectedScenario(baseGraph, packets, "data/scenarios/failure.txt", "log_failure.txt");
                break;
            case 6:
                runSelectedScenario(baseGraph, packets, "data/scenarios/redundancy.txt", "log_redundancy.txt");
                break;
            case 7:
                Reporter::printMSTReport(baseGraph, pricingModel);
                break;
            case 8:
                Reporter::printFullRoutingTable(baseGraph);
                break;
            default:
                std::cout << "Lua chon khong hop le! Vui long nhap so tu 0 den 8.\n";
                break;
        }

        std::cout << "\nNhan Enter de tiep tuc...";
        std::string pauseLine;
        std::getline(std::cin, pauseLine);
    }

    return 0;
}
