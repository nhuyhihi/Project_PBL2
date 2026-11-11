#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <fstream>

#ifdef _WIN32
#include <windows.h>
#endif

#include <SFML/Graphics.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/System/String.hpp>
#include <SFML/Window/Event.hpp>

#include "imgui.h"
#include "imgui-SFML.h"

#include "Graph.h"
#include "IOManager.h"
#include "Kruskal.h"
#include "CostModel.h"
#include "DelayModel.h"
#include "RoutingCost.h"
#include "Dijkstra.h"
#include "PathResult.h"
#include "Connectivity.h"
#include "FailureHandler.h"
#include "Simulator.h"
#include "Scenario.h"
#include "Logger.h"
#include "Validation.h"
#include "EnumUtils.h"


constexpr unsigned int WINDOW_WIDTH   = 1280;
constexpr unsigned int WINDOW_HEIGHT  = 720;
constexpr float TOP_BAR_HEIGHT        = 42.0f;
constexpr float STATUS_BAR_HEIGHT     = 28.0f;

constexpr float SAFE_CENTER_X         = 500.0f;
constexpr float SAFE_CENTER_Y         = 410.0f;
constexpr float ROUTER_RADIUS         = 210.0f;
constexpr float HOST_DISTANCE         = 120.0f;
constexpr float NODE_RADIUS           = 22.0f;
constexpr float CUT_HITBOX_TOLERANCE  = 8.0f;

constexpr float DRAWER_MIN_WIDTH      = 300.0f;
constexpr float DRAWER_MAX_WIDTH      = 430.0f;
constexpr float DRAWER_DEFAULT_WIDTH  = 395.0f;
constexpr float DRAWER_HEIGHT         = 650.0f;

struct KruskalStep {
    int edgeId = -1;
    int u = 0, v = 0;
    double weight = 0.0;
    bool isSelected = false;
    std::string description;
};

struct PacketAnim {
    int sourceNode = 0;
    int destNode = 0;
    int currentHopIndex = 0;
    float progress = 0.0f;
    float speed = 1.0f;
    std::vector<int> path;
    bool isDelivered = false;
    bool isBlocked = false;
};

struct DeliveryRipple {
    int nodeId = -1;
    float timer = 0.0f;
};

struct AppState {
    bool isMSTActive = false;
    bool hasRunMST = false;
    bool isScenarioActive = false;
    bool isCutModeActive = false;
    bool isRestoreModeActive = false;

    bool isSidebarOpen = false;
    int  sidebarMode = 0; 
    bool switchTabRequested = false;
    int  requestedTab = 0;
    int  hoveredNodeId = -1;
    int  selectedSrcNode = 0;
    int  selectedDstNode = -1;

    int  viewMode = 0; 
    int  selectedPacketPreset = 0; 
    bool onlyBuiltLinks = true;   

    MSTResult mstResult;
    PathResult currentPath;
    BackupResult backupResult;

    int lastRouteSrc = -1;
    int lastRouteDst = -1;
    std::vector<int> lastRouteNodes;
    bool isRerouted = false;
    bool dropByMtu = false;   // DROP vì gói DF lớn hơn MTU của cáp còn lại (không phải mất liên thông)
    int  dropMtu = 0;         // MTU nhỏ nhất trên đường vật lý còn lại (để hiển thị)
    std::string previousRouteStr = "";

    // Máy trạng thái Kruskal
    std::vector<KruskalStep> kruskalHistory;
    int currentKruskalStep = 0;

    // Quản lý kịch bản
    int activeScenarioIdx = 0;
    std::vector<std::string> scenarioFiles = {
        "basic.txt", "qos.txt", "congestion.txt", "redundancy.txt"
    };
    std::vector<std::string> scenarioNames = {
        "1. Cơ Bản", "2. Phân Luồng QoS", "3. Tắc Nghẽn", "4. Đóng Góp Dự Phòng"
    };
    Scenario loadedScenario;
    int currentScenarioStep = 0;
    int maxScenarioSteps = 0;

    // Gói tin hoạt họa & hiệu ứng sóng đích
    std::vector<PacketAnim> activePackets;
    std::vector<DeliveryRipple> ripples;

    // Thông tin định tuyến lớp Logic hiện tại
    SimulationResult logicSimResult;
    bool hasRoutingInfo = false;

    std::string statusMessage = "Hệ thống sẵn sàng. Chọn Nguồn/Đích trên thanh điều khiển hoặc click trực tiếp.";

    void reset(int defaultNodeId = 0) {
        isMSTActive = false;
        hasRunMST = false;
        isScenarioActive = false;
        isCutModeActive = false;
        isRestoreModeActive = false;
        isSidebarOpen = false;
        sidebarMode = 0;
        hoveredNodeId = -1;
        selectedSrcNode = defaultNodeId;
        selectedDstNode = -1;
        viewMode = 0;
        selectedPacketPreset = 0;
        onlyBuiltLinks = true;
        currentKruskalStep = 0;
        currentScenarioStep = 0;
        mstResult = MSTResult();
        currentPath = PathResult();
        backupResult = BackupResult();
        lastRouteSrc = -1;
        lastRouteDst = -1;
        lastRouteNodes.clear();
        isRerouted = false;
        dropByMtu = false;
        dropMtu = 0;
        previousRouteStr.clear();
        activePackets.clear();
        ripples.clear();
        hasRoutingInfo = false;
        statusMessage = "Đã khôi phục mạng về trạng thái sạch ban đầu.";
    }
};


Packet buildCurrentPacket(const AppState& appState, const std::vector<Packet>& packets, int srcNode, int dstNode) {
    Packet pkt;
    pkt.id = 999;
    pkt.source = srcNode;
    pkt.destination = dstNode;

    if (!packets.empty()) {
        int idx = std::clamp(appState.selectedPacketPreset, 0, static_cast<int>(packets.size()) - 1);
        pkt = packets[idx];
        pkt.source = srcNode;
        pkt.destination = dstNode;
    } else {
        pkt.sizeBytes = 1500;
        pkt.protocol = Protocol::UDP;
        pkt.canFragment = true;
        pkt.newConnection = false;
        pkt.type = PacketType::REAL_TIME;
    }

    return pkt;
}

// Trả về thành phần liên thông lớn nhất gồm các cáp đang hoạt động (trạm ngoài tập này bị cô lập)
std::unordered_set<int> mainNetworkNodes(const Graph& g, bool onlyBuilt) {
    std::unordered_set<int> best;
    for (const auto& comp : Connectivity::connectedComponents(g, onlyBuilt)) {
        if (comp.size() > best.size()) best = std::unordered_set<int>(comp.begin(), comp.end());
    }
    return best;
}

// Tính tổng khoảng cách mét thực tế trên đường đi
double calculatePathPhysicalLength(const Graph& g, const std::vector<int>& pathNodes) {
    if (pathNodes.size() < 2) return 0.0;
    double total = 0.0;
    for (size_t i = 0; i < pathNodes.size() - 1; ++i) {
        int u = pathNodes[i];
        int v = pathNodes[i + 1];
        for (int eId : g.getNeighbors(u)) {
            if (g.otherEndpoint(u, eId) == v) {
                total += g.getEdge(eId).length;
                break;
            }
        }
    }
    return total;
}

std::string formatNodePath(const Graph& g, const std::vector<int>& nodes) {
    std::string out = "";
    for (size_t i = 0; i < nodes.size(); ++i) {
        int nid = nodes[i];
        std::string nm = g.hasNode(nid) ? g.getNode(nid).name : "";
        if (nm.empty()) nm = std::to_string(nid);
        out += nm + (i + 1 < nodes.size() ? " -> " : "");
    }
    return out;
}

// Hình học đường cong Bézier tách cáp chéo và kiểm tra hitbox
float distancePointToSegment(const sf::Vector2f& p, const sf::Vector2f& a, const sf::Vector2f& b) {
    const float l2 = (b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y);
    if (l2 == 0.0f) {
        float dx = p.x - a.x, dy = p.y - a.y;
        return std::sqrt(dx * dx + dy * dy);
    }
    const float t = std::clamp(((p.x - a.x) * (b.x - a.x) + (p.y - a.y) * (b.y - a.y)) / l2, 0.0f, 1.0f);
    const sf::Vector2f proj(a.x + t * (b.x - a.x), a.y + t * (b.y - a.y));
    float dx = p.x - proj.x, dy = p.y - proj.y;
    return std::sqrt(dx * dx + dy * dy);
}

bool isPairMatch(int u, int v, int a, int b) {
    if (a == -1 || b == -1) return false;
    return (u == a && v == b) || (u == b && v == a);
}

sf::Vector2f getDiagonalCurveControl(const sf::Vector2f& posA, const sf::Vector2f& posB) {
    sf::Vector2f mid = (posA + posB) * 0.5f;
    float dx = posB.x - posA.x;
    float dy = posB.y - posA.y;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < 1.0f) return mid;
    sf::Vector2f normal(-dy / len, dx / len);
    return mid + normal * 55.0f;
}

sf::Vector2f evalQuadraticBezier(const sf::Vector2f& p0, const sf::Vector2f& p1, const sf::Vector2f& p2, float t) {
    float u = 1.0f - t;
    return u * u * p0 + 2.0f * u * t * p1 + t * t * p2;
}

float getDistanceToEdge(const sf::Vector2f& clickPos, bool isCurved,
                        const sf::Vector2f& p1, const sf::Vector2f& p2, const sf::Vector2f& ctrl) {
    if (isCurved) {
        constexpr int SEGS = 16;
        float minDist = 99999.0f;
        sf::Vector2f prev = p1;
        for (int i = 1; i <= SEGS; ++i) {
            float t = static_cast<float>(i) / SEGS;
            sf::Vector2f curr = evalQuadraticBezier(p1, ctrl, p2, t);
            minDist = std::min(minDist, distancePointToSegment(clickPos, prev, curr));
            prev = curr;
        }
        return minDist;
    }
    return distancePointToSegment(clickPos, p1, p2);
}

// Hàm tiện ích vẽ đồ họa SFML
void drawThickLine(sf::RenderWindow& win, const sf::Vector2f& p1, const sf::Vector2f& p2, sf::Color color, float thickness) {
    sf::Vector2f dir = p2 - p1;
    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    if (len < 0.1f) return;
    sf::Vector2f normal(-dir.y / len * (thickness / 2.0f), dir.x / len * (thickness / 2.0f));

    sf::Vertex quad[6] = {
        sf::Vertex{p1 + normal, color, {0.0f, 0.0f}},
        sf::Vertex{p2 + normal, color, {0.0f, 0.0f}},
        sf::Vertex{p2 - normal, color, {0.0f, 0.0f}},
        sf::Vertex{p1 + normal, color, {0.0f, 0.0f}},
        sf::Vertex{p2 - normal, color, {0.0f, 0.0f}},
        sf::Vertex{p1 - normal, color, {0.0f, 0.0f}}
    };
    win.draw(quad, 6, sf::PrimitiveType::Triangles);
}

void drawCurvedCable(sf::RenderWindow& win, const sf::Vector2f& p1, const sf::Vector2f& ctrl, const sf::Vector2f& p2, sf::Color color, float thickness) {
    constexpr int SEGS = 16;
    sf::Vector2f prev = p1;
    for (int i = 1; i <= SEGS; ++i) {
        float t = static_cast<float>(i) / SEGS;
        sf::Vector2f curr = evalQuadraticBezier(p1, ctrl, p2, t);
        drawThickLine(win, prev, curr, color, thickness);
        prev = curr;
    }
}

void drawLinkBadge(sf::RenderWindow& win, const sf::Font& font, const sf::Vector2f& pos, 
                   const std::string& text, sf::Color bgColor, sf::Color borderColor, sf::Color textColor) {
    if (text.empty()) return;
    sf::Text txt(font);
    txt.setString(sf::String::fromUtf8(text.begin(), text.end()));
    txt.setCharacterSize(10);
    txt.setFillColor(textColor);
    txt.setStyle(sf::Text::Bold);
    sf::FloatRect bounds = txt.getLocalBounds();

    float padX = 5.0f;
    float padY = 2.0f;
    sf::Vector2f badgeSize(bounds.size.x + padX * 2.0f, bounds.size.y + padY * 2.0f + 2.0f);

    sf::RectangleShape badge(badgeSize);
    badge.setOrigin({badgeSize.x / 2.0f, badgeSize.y / 2.0f});
    badge.setPosition(pos);
    badge.setFillColor(bgColor);
    badge.setOutlineColor(borderColor);
    badge.setOutlineThickness(1.0f);
    win.draw(badge);

    txt.setOrigin({bounds.position.x + bounds.size.x / 2.0f, bounds.position.y + bounds.size.y / 2.0f});
    txt.setPosition(pos);
    win.draw(txt);
}

// Nút bật/tắt trạng thái trên thanh công cụ
bool DrawToggleButton(const char* label, bool isActive, const ImVec4& baseColor, const ImVec2& size = ImVec2(0, 0)) {
    if (isActive) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(baseColor.x * 0.70f, baseColor.y * 0.70f, baseColor.z * 0.70f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(baseColor.x * 0.80f, baseColor.y * 0.80f, baseColor.z * 0.80f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(baseColor.x * 0.60f, baseColor.y * 0.60f, baseColor.z * 0.60f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.20f, 0.25f, 0.30f, 0.90f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.5f);
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, baseColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(baseColor.x * 1.10f, baseColor.y * 1.10f, baseColor.z * 1.10f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(baseColor.x * 0.90f, baseColor.y * 0.90f, baseColor.z * 0.90f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.80f, 0.82f, 0.85f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
    }

    std::string text = label;
    bool clicked = ImGui::Button(text.c_str(), size);

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(5);
    return clicked;
}

// Giao diện sáng ImGui
void ApplyCleanLightTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::StyleColorsLight(&style);

    style.WindowRounding    = 5.0f;
    style.FrameRounding     = 4.0f;
    style.PopupRounding     = 5.0f;
    style.GrabRounding      = 4.0f;
    style.TabRounding       = 4.0f;
    style.ScrollbarRounding = 4.0f;

    style.WindowBorderSize  = 1.0f;
    style.FrameBorderSize   = 1.0f;
    style.PopupBorderSize   = 1.0f;

    style.ItemSpacing   = ImVec2(10.0f, 6.0f);
    style.FramePadding  = ImVec2(8.0f, 4.0f);
    style.WindowPadding = ImVec2(12.0f, 8.0f);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg]             = ImVec4(0.98f, 0.98f, 0.99f, 0.98f);
    colors[ImGuiCol_PopupBg]              = ImVec4(1.00f, 1.00f, 1.00f, 0.99f);
    colors[ImGuiCol_Border]               = ImVec4(0.82f, 0.85f, 0.88f, 1.00f);
    colors[ImGuiCol_FrameBg]              = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]       = ImVec4(0.93f, 0.95f, 0.98f, 1.00f);
    colors[ImGuiCol_FrameBgActive]        = ImVec4(0.88f, 0.92f, 0.97f, 1.00f);

    colors[ImGuiCol_Text]                 = ImVec4(0.12f, 0.15f, 0.18f, 1.00f);
    colors[ImGuiCol_TextDisabled]         = ImVec4(0.55f, 0.58f, 0.62f, 1.00f);

    colors[ImGuiCol_CheckMark]            = ImVec4(0.12f, 0.68f, 0.28f, 1.00f);

    colors[ImGuiCol_Button]               = ImVec4(0.92f, 0.94f, 0.96f, 1.00f);
    colors[ImGuiCol_ButtonHovered]        = ImVec4(0.85f, 0.89f, 0.95f, 1.00f);
    colors[ImGuiCol_ButtonActive]         = ImVec4(0.78f, 0.83f, 0.90f, 1.00f);

    colors[ImGuiCol_Tab]                  = ImVec4(0.92f, 0.93f, 0.95f, 1.00f);
    colors[ImGuiCol_TabHovered]           = ImVec4(0.85f, 0.89f, 0.96f, 1.00f);
    colors[ImGuiCol_TabActive]            = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TabUnfocused]         = ImVec4(0.94f, 0.95f, 0.96f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive]   = ImVec4(0.98f, 0.98f, 0.99f, 1.00f);

    colors[ImGuiCol_Header]               = ImVec4(0.90f, 0.93f, 0.97f, 1.00f);
    colors[ImGuiCol_HeaderHovered]        = ImVec4(0.84f, 0.89f, 0.95f, 1.00f);
    colors[ImGuiCol_HeaderActive]         = ImVec4(0.78f, 0.84f, 0.92f, 1.00f);
    colors[ImGuiCol_Separator]            = ImVec4(0.84f, 0.87f, 0.90f, 1.00f);
}

// Khởi tạo lịch sử duyệt cạnh Kruskal và danh sách cáp dự phòng
static std::string formatCurrency(double cost) {
    long long c = static_cast<long long>(std::round(cost));
    std::string s = std::to_string(c);
    int n = static_cast<int>(s.length()) - 3;
    while (n > 0) {
        s.insert(n, ".");
        n -= 3;
    }
    return s + " ĐV";
}

void recordKruskalSteps(Graph& g, const CostModel& pricingModel, AppState& appState, bool updateGraphBuildFlags = true) {
    appState.kruskalHistory.clear();
    auto costFn = [&pricingModel](const Edge& e) { return pricingModel.calculateCablingCost(e).total; };

    Graph targetG = g;
    std::vector<Rejection> rejections;
    appState.mstResult = buildMST(targetG, costFn, &rejections);
    appState.backupResult = selectBackupLinks(targetG, appState.mstResult, DEFAULT_BACKUP_LINKS, costFn);

    if (updateGraphBuildFlags) {
        g = targetG;
    }

    std::unordered_map<int, RejectReason> rejectMap;
    for (const auto& r : rejections) {
        rejectMap[r.edgeId] = r.reason;
    }

    std::vector<int> order = targetG.edgeIds();
    std::sort(order.begin(), order.end(), [&](int x, int y) {
        return compareEdges(targetG.getEdge(x), targetG.getEdge(y), costFn);
    });

    for (int edgeId : order) {
        const Edge& e = targetG.getEdge(edgeId);
        KruskalStep step;
        step.edgeId = edgeId;
        step.u = e.u;
        step.v = e.v;
        step.weight = costFn(e);

        std::string uName = targetG.getNode(e.u).name.empty() ? std::to_string(e.u) : targetG.getNode(e.u).name;
        std::string vName = targetG.getNode(e.v).name.empty() ? std::to_string(e.v) : targetG.getNode(e.v).name;

        if (rejectMap.count(edgeId) == 0) {
            step.isSelected = true;
            step.description = "Chấp nhận: Nối " + uName + " - " + vName + 
                               ", Chi phí: " + formatCurrency(step.weight);
        } else {
            step.isSelected = false;
            step.description = "Loại bỏ: Nối " + uName + " - " + vName + 
                               " (" + toString(rejectMap[edgeId]) + ")";
        }
        appState.kruskalHistory.push_back(step);
    }
}

// Cập nhật lộ trình định tuyến Dijkstra cho gói tin hiện tại
void updateRoutingPath(const Graph& g, const std::vector<Packet>& packets, AppState& appState) {
    appState.dropByMtu = false;
    appState.dropMtu = 0;
    if (appState.selectedSrcNode == -1 || appState.selectedDstNode == -1 || appState.selectedSrcNode == appState.selectedDstNode) {
        appState.currentPath = PathResult();
        appState.hasRoutingInfo = false;
        appState.isRerouted = false;
        return;
    }

    Packet qosPkt = buildCurrentPacket(appState, packets, appState.selectedSrcNode, appState.selectedDstNode);

    // Chạy Dijkstra với onlyBuiltLinks (mặc định true: chỉ đi trên cáp đã lắp đặt MST + Backup)
    appState.currentPath = dijkstra(g, appState.selectedSrcNode, appState.selectedDstNode, qosPkt, appState.onlyBuiltLinks);

    // Nếu vẫn liên thông nhưng gói DF vượt MTU thì xác định MTU nhỏ nhất để báo lỗi
    if (!appState.currentPath.reachable && !qosPkt.canFragment &&
        Connectivity::isConnected(g, appState.selectedSrcNode, appState.selectedDstNode, appState.onlyBuiltLinks)) {
        appState.dropByMtu = true;
        PathResult physical = dijkstra(g, appState.selectedSrcNode, appState.selectedDstNode, lengthWeight, appState.onlyBuiltLinks);
        if (physical.reachable && !physical.edges.empty()) {
            int minMtu = g.getEdge(physical.edges.front()).mtu;
            for (int id : physical.edges) minMtu = std::min(minMtu, g.getEdge(id).mtu);
            appState.dropMtu = minMtu;
        }
    }

    // Sử dụng Simulator::evaluatePacket (Read-Only, không cần copy Graph hay làm bẩn currentLoad)
    appState.logicSimResult = Simulator::evaluatePacket(qosPkt, appState.currentPath, g);
    appState.hasRoutingInfo = true;

    // Kiểm tra xem lộ trình có vừa tự động đổi hướng (Reroute) trên cùng cặp (Src, Dst) không
    if (appState.lastRouteSrc == appState.selectedSrcNode &&
        appState.lastRouteDst == appState.selectedDstNode &&
        !appState.lastRouteNodes.empty() &&
        appState.currentPath.reachable &&
        appState.currentPath.nodes != appState.lastRouteNodes) {
        appState.isRerouted = true;
        appState.previousRouteStr = formatNodePath(g, appState.lastRouteNodes);
    } else if (appState.lastRouteSrc != appState.selectedSrcNode || appState.lastRouteDst != appState.selectedDstNode) {
        appState.isRerouted = false;
        appState.previousRouteStr.clear();
    }

    if (appState.currentPath.reachable) {
        appState.lastRouteSrc = appState.selectedSrcNode;
        appState.lastRouteDst = appState.selectedDstNode;
        appState.lastRouteNodes = appState.currentPath.nodes;
    }

    std::string srcName = g.getNode(appState.selectedSrcNode).name;
    std::string dstName = g.getNode(appState.selectedDstNode).name;
    if (srcName.empty()) srcName = "Node " + std::to_string(appState.selectedSrcNode);
    if (dstName.empty()) dstName = "Node " + std::to_string(appState.selectedDstNode);

    if (appState.currentPath.reachable) {
        double distM = calculatePathPhysicalLength(g, appState.currentPath.nodes);
        std::stringstream ss;
        ss << "Định tuyến " << srcName << " -> " << dstName 
           << " [" << appState.currentPath.hopCount << " hops, Cự ly: " 
           << static_cast<int>(distM) << "m, Độ trễ: " 
           << std::fixed << std::setprecision(3) << appState.logicSimResult.totalDelay * 1000.0 << " ms, "
           << appState.logicSimResult.fragmentCount << " mảnh]";
        if (appState.isRerouted) {
            ss << " [ĐÃ ĐỔI ĐƯỜNG từ " << appState.previousRouteStr << "]";
        }
        appState.statusMessage = ss.str();
    } else {
        if (appState.dropByMtu) {
            std::string mtuText = appState.dropMtu > 0 ? " " + std::to_string(appState.dropMtu) + "B" : "";
            appState.statusMessage = "DROP [DF_BLOCKED_BY_MTU]: Gói tin " + std::to_string(qosPkt.sizeBytes) +
                                     "B vượt quá MTU" + mtuText + " nhưng cấm phân mảnh!";
        } else {
            appState.statusMessage = "DROP [UNREACHABLE]: Không còn đường đi hợp lệ từ " + srcName + " đến " + dstName + "!";
        }
    }
}

void applyScenarioToStep(Graph& workingGraph, const Graph& baseGraph, const CostModel& /*pricingModel*/,
                         const std::vector<Packet>& packets, const Scenario& scenario, int targetStep, AppState& appState) {
    // Khôi phục từ baseGraph (đã có sẵn cờ isBuilt & isBackup của quy hoạch hạ tầng ban đầu)
    workingGraph = baseGraph;
    if (targetStep >= 0 && targetStep < static_cast<int>(scenario.events.size())) {
        for (int i = 0; i <= targetStep; ++i) {
            const auto& ev = scenario.events[i];
            if (workingGraph.hasEdge(ev.targetId)) {
                Simulator::applyEvent(workingGraph, ev);
            }
        }
    }
    // Giữ nguyên quy hoạch hạ tầng ban đầu trên baseGraph khi chạy sự kiện kịch bản
    if (appState.selectedDstNode == -1) {
        if (!packets.empty()) {
            appState.selectedSrcNode = packets.front().source;
            appState.selectedDstNode = packets.front().destination;
        } else {
            appState.selectedDstNode = 1;
        }
    }

    // Tự động cập nhật ngay lập tức lộ trình và độ trễ ở Lớp Logic
    updateRoutingPath(workingGraph, packets, appState);

    // Tự động kích hoạt hoạt họa chấm chạy gói tin theo lộ trình mới
    if (appState.currentPath.reachable && appState.currentPath.nodes.size() >= 2) {
        appState.activePackets.clear();
        PacketAnim p;
        p.sourceNode = appState.selectedSrcNode;
        p.destNode = appState.selectedDstNode;
        p.path = appState.currentPath.nodes;
        p.currentHopIndex = 0;
        p.progress = 0.0f;
        p.speed = 1.35f;
        appState.activePackets.push_back(p);
    }
}

// Chương trình chính
int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    // 1. Nạp dữ liệu và quy hoạch hạ tầng cơ sở (MST + Dự phòng)
    Graph baseGraph;
    if (!IOManager::loadNodes("data/nodes.txt", baseGraph) ||
        !IOManager::loadEdges("data/edges.txt", baseGraph)) {
        std::cerr << "Lỗi: Không thể nạp đồ thị ban đầu!\n";
        return 1;
    }

    std::vector<Packet> packets;
    IOManager::loadPackets("data/packets.txt", packets, &baseGraph);

    CostModel pricingModel;
    pricingModel.loadPricing("data/pricing.txt");
    auto costFn = [&pricingModel](const Edge& e) { return pricingModel.calculateCablingCost(e).total; };

    // Xây dựng cây khung MST và cáp dự phòng trên baseGraph
    MSTResult initialMST = buildMST(baseGraph, costFn);
    selectBackupLinks(baseGraph, initialMST, DEFAULT_BACKUP_LINKS, costFn);

    Graph workingGraph = baseGraph;

    int defaultNodeId = workingGraph.nodeIds().empty() ? 0 : workingGraph.nodeIds().front();
    AppState appState;
    appState.reset(defaultNodeId);
    appState.statusMessage = "Sơ đồ khảo sát ban đầu. Bấm 'Chạy MST' để quy hoạch cáp hoặc chọn 'Lớp Logic'.";

    // Tự động kiểm tra nếu có file lưu trạng thái mạng trước đó (data/saved_network.txt)
    std::ifstream savedIn("data/saved_network.txt");
    if (savedIn.is_open()) {
        std::string line;
        int restoredCount = 0;
        while (std::getline(savedIn, line)) {
            if (line.empty() || line[0] == '#') continue;
            std::istringstream iss(line);
            int eId, u, v, isUpVal;
            double len;
            if (iss >> eId >> u >> v >> len >> isUpVal) {
                if (workingGraph.hasEdge(eId)) {
                    workingGraph.getEdge(eId).isUp = (isUpVal != 0);
                    restoredCount++;
                }
            }
        }
        savedIn.close();
        if (restoredCount > 0) {
            appState.hasRunMST = true;
            appState.statusMessage = "[Tự Động Nạp]: Đã khôi phục trạng thái mạng cũ từ data/saved_network.txt (" + 
                                     std::to_string(restoredCount) + " tuyến cáp).";
        }
    }

    recordKruskalSteps(workingGraph, pricingModel, appState, /*updateGraphBuildFlags=*/true);

    IOManager::loadScenario("data/scenarios/" + appState.scenarioFiles[0], appState.loadedScenario);
    appState.maxScenarioSteps = static_cast<int>(appState.loadedScenario.events.size());

    std::vector<int> allNodeIds = workingGraph.nodeIds();
    std::vector<std::string> nodeLabels;
    for (int id : allNodeIds) {
        std::string n = workingGraph.getNode(id).name;
        nodeLabels.push_back(n.empty() ? ("Node " + std::to_string(id)) : n);
    }

    std::vector<std::string> packetPresetLabels;
    for (const Packet& p : packets) {
        std::string tStr = EnumUtils::toString(p.type);
        std::string prStr = EnumUtils::toString(p.protocol);
        std::string dfStr = p.canFragment ? "Cho băm" : "DF=Chặn";
        packetPresetLabels.push_back(
            "Pkt #" + std::to_string(p.id) + " [packets.txt]: " +
            std::to_string(p.sizeBytes) + "B | " + tStr + " | " + prStr + " | " + dfStr
        );
    }
    if (packetPresetLabels.empty()) {
        packetPresetLabels.push_back("Mặc định 1500B | REAL_TIME | UDP");
    }

    sf::ContextSettings settings;
    settings.antiAliasingLevel = 8;

    std::string winTitle = "Mô phỏng hạ tầng mạng và định tuyến gói tin";
    sf::String sfTitle = sf::String::fromUtf8(winTitle.begin(), winTitle.end());

    sf::RenderWindow window(
        sf::VideoMode({WINDOW_WIDTH, WINDOW_HEIGHT}),
        sfTitle,
        sf::Style::Titlebar | sf::Style::Close,
        sf::State::Windowed,
        settings
    );
    window.setFramerateLimit(60);

    if (!ImGui::SFML::Init(window)) {
        std::cerr << "Lỗi: Không thể khởi tạo ImGui::SFML!\n";
        return 1;
    }

    ApplyCleanLightTheme();

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    
    ImFontConfig fontConfig;
    fontConfig.OversampleH = 2;
    fontConfig.OversampleV = 2;
    static const ImWchar* vRanges = io.Fonts->GetGlyphRangesVietnamese();
    ImFont* vnFont = io.Fonts->AddFontFromFileTTF(
        "C:/Windows/Fonts/segoeui.ttf",
        16.0f,
        &fontConfig,
        vRanges
    );
    if (vnFont != nullptr) {
        io.FontDefault = vnFont;
        (void)ImGui::SFML::UpdateFontTexture();
    }

    sf::Font sfmlFont;
    bool hasSfmlFont = sfmlFont.openFromFile("C:/Windows/Fonts/segoeui.ttf") || sfmlFont.openFromFile("C:/Windows/Fonts/arial.ttf");

    sf::Clock deltaClock;

    sf::VertexArray gridDots(sf::PrimitiveType::Points);
    constexpr float GRID_STEP = 28.0f;
    for (float gx = 20.0f; gx < WINDOW_WIDTH - 20.0f; gx += GRID_STEP) {
        for (float gy = TOP_BAR_HEIGHT + 14.0f; gy < WINDOW_HEIGHT - STATUS_BAR_HEIGHT - 10.0f; gy += GRID_STEP) {
            gridDots.append(sf::Vertex{sf::Vector2f(gx, gy), sf::Color(210, 222, 235, 170), {0.0f, 0.0f}});
        }
    }

    std::unordered_map<int, sf::Vector2f> nodePositions;
    int curvedDiagU = -1, curvedDiagV = -1;
    int straightDiagU = -1, straightDiagV = -1;
    sf::Vector2f diagCtrl(SAFE_CENTER_X, SAFE_CENTER_Y);

    auto recomputeLayout = [&]() {
        nodePositions.clear();
        curvedDiagU = curvedDiagV = -1;
        straightDiagU = straightDiagV = -1;

        std::vector<int> routerIds;
        std::vector<int> hostIds;

        for (int id : workingGraph.nodeIds()) {
            if (workingGraph.getNode(id).type == NodeType::HOST) {
                hostIds.push_back(id);
            } else {
                routerIds.push_back(id);
            }
        }

        int numRouters = static_cast<int>(routerIds.size());
        for (int i = 0; i < numRouters; ++i) {
            float angle = i * (2.0f * 3.14159265f / numRouters) - 3.14159265f / 4.0f;
            nodePositions[routerIds[i]] = sf::Vector2f(
                SAFE_CENTER_X + ROUTER_RADIUS * std::cos(angle),
                SAFE_CENTER_Y + ROUTER_RADIUS * std::sin(angle)
            );
        }

        // Nếu có từ 4 Router trở lên, tự động tách đường chéo thứ 2 (routerIds[1] - routerIds[3]) thành cung Bezier
        if (numRouters >= 4) {
            straightDiagU = routerIds[0];
            straightDiagV = routerIds[2];
            curvedDiagU   = routerIds[1];
            curvedDiagV   = routerIds[3];
            diagCtrl = getDiagonalCurveControl(nodePositions.at(curvedDiagU), nodePositions.at(curvedDiagV));
        }

        for (int hId : hostIds) {
            int connectedRouter = -1;
            for (int eId : workingGraph.getNeighbors(hId)) {
                int other = workingGraph.otherEndpoint(hId, eId);
                if (other != -1 && workingGraph.getNode(other).type == NodeType::ROUTER) {
                    connectedRouter = other;
                    break;
                }
            }

            if (connectedRouter != -1 && nodePositions.count(connectedRouter)) {
                sf::Vector2f rPos = nodePositions.at(connectedRouter);
                float dirX = rPos.x - SAFE_CENTER_X;
                float dirY = rPos.y - SAFE_CENTER_Y;
                float len = std::sqrt(dirX * dirX + dirY * dirY);
                if (len < 1.0f) { dirX = 1.0f; dirY = 0.0f; len = 1.0f; }
                dirX /= len; dirY /= len;

                nodePositions[hId] = sf::Vector2f(rPos.x + dirX * HOST_DISTANCE, rPos.y + dirY * HOST_DISTANCE);
            } else {
                nodePositions[hId] = sf::Vector2f(SAFE_CENTER_X - 250.0f, SAFE_CENTER_Y);
            }
        }
    };
    recomputeLayout();

    // 3. Vòng lặp giao diện chính (60 FPS)
    while (window.isOpen()) {
        sf::Vector2i mousePixel = sf::Mouse::getPosition(window);
        sf::Vector2f mousePos(static_cast<float>(mousePixel.x), static_cast<float>(mousePixel.y));

        while (const std::optional event = window.pollEvent()) {
            ImGui::SFML::ProcessEvent(window, *event);

            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            // Click chuột trên Canvas
            if (const auto* mousePress = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (!ImGui::GetIO().WantCaptureMouse) {
                    sf::Vector2f clickPos(static_cast<float>(mousePress->position.x), 
                                          static_cast<float>(mousePress->position.y));

                    // 1. Chế độ Cắt Cáp hoặc Khôi Phục Cáp (CHỈ THAO TÁC TRÊN CÁP ĐÃ LẮP ĐẶT HỢP LỆ)
                    if ((appState.isCutModeActive || appState.isRestoreModeActive) && mousePress->button == sf::Mouse::Button::Left) {
                        int hitEdgeId = -1;
                        bool hitUnbuilt = false;
                        float minDist = CUT_HITBOX_TOLERANCE + 1.0f;
                        for (int eId : workingGraph.edgeIds()) {
                            const Edge& e = workingGraph.getEdge(eId);
                            if (nodePositions.count(e.u) && nodePositions.count(e.v)) {
                                bool curved = isPairMatch(e.u, e.v, curvedDiagU, curvedDiagV);
                                float d = getDistanceToEdge(clickPos, curved, nodePositions.at(e.u), nodePositions.at(e.v), diagCtrl);
                                if (d <= CUT_HITBOX_TOLERANCE && d < minDist) {
                                    minDist = d;
                                    if (!e.isBuilt || !Validation::isStaticFeasible(e)) {
                                        hitUnbuilt = true;
                                        hitEdgeId = -1;
                                    } else {
                                        hitUnbuilt = false;
                                        hitEdgeId = eId;
                                    }
                                }
                            }
                        }

                        if (hitEdgeId != -1) {
                            Edge& edge = workingGraph.getEdge(hitEdgeId);
                            bool targetState = appState.isRestoreModeActive ? true : false;
                            FailureHandler::setLinkState(workingGraph, hitEdgeId, targetState);
                            
                            std::string uName = workingGraph.getNode(edge.u).name.empty() ? std::to_string(edge.u) : workingGraph.getNode(edge.u).name;
                            std::string vName = workingGraph.getNode(edge.v).name.empty() ? std::to_string(edge.v) : workingGraph.getNode(edge.v).name;

                            updateRoutingPath(workingGraph, packets, appState);

                            bool isNetConnected = Connectivity::isNetworkConnected(workingGraph, appState.onlyBuiltLinks);
                            if (!targetState) {
                                if (isNetConnected) {
                                    std::string msg = "[Đứt Cáp " + uName + "-" + vName + "]: Mạng vẫn liên thông!";
                                    if (appState.currentPath.reachable) {
                                        msg += " Đã tự động đổi sang đường vòng: " + formatNodePath(workingGraph, appState.currentPath.nodes);
                                    }
                                    appState.statusMessage = msg;
                                } else {
                                    auto comps = Connectivity::connectedComponents(workingGraph, appState.onlyBuiltLinks);
                                    appState.statusMessage = "[CẢNH BÁO MẤT LIÊN THÔNG]: Cáp " + uName + "-" + vName + " đứt làm mạng bị chia cắt thành " + 
                                                             std::to_string(comps.size()) + " vùng độc lập!";
                                }
                            } else {
                                appState.statusMessage = "[Khôi Phục]: Đã nối lại cáp " + uName + "-" + vName + ". Tuyến cáp đã hoạt động bình thường!";
                            }
                        } else if (hitUnbuilt) {
                            appState.statusMessage = "[Từ chối] Tuyến cáp chưa được thi công lắp đặt, không thể cắt/nối!";
                        } else {
                            if (appState.isRestoreModeActive) {
                                appState.statusMessage = "Không bấm trúng cáp đã lắp nào. Đã thoát chế độ Khôi Phục Cáp.";
                                appState.isRestoreModeActive = false;
                            } else {
                                appState.statusMessage = "Không bấm trúng cáp đã lắp nào. Đã thoát chế độ Cắt Cáp.";
                                appState.isCutModeActive = false;
                            }
                        }
                    }
                    // 2. Chế độ Chọn Node nguồn / đích trên Canvas
                    else {
                        // CHUỘT PHẢI: Chọn trạm NGUỒN (Source)
                        if (mousePress->button == sf::Mouse::Button::Right) {
                            for (const auto& [nId, pos] : nodePositions) {
                                bool hit = false;
                                if (workingGraph.getNode(nId).type == NodeType::HOST) {
                                    if (std::abs(clickPos.x - pos.x) <= 22.0f && std::abs(clickPos.y - pos.y) <= 20.0f) hit = true;
                                } else {
                                    float dx = clickPos.x - pos.x, dy = clickPos.y - pos.y;
                                    if (std::sqrt(dx * dx + dy * dy) <= NODE_RADIUS + 4.0f) hit = true;
                                }
                                if (hit) {
                                    appState.selectedSrcNode = nId;
                                    if (appState.selectedDstNode == nId) appState.selectedDstNode = -1;
                                    updateRoutingPath(workingGraph, packets, appState);
                                    break;
                                }
                            }
                        }
                        // CHUỘT TRÁI: Chọn trạm ĐÍCH (Destination)
                        else if (mousePress->button == sf::Mouse::Button::Left) {
                            for (const auto& [nId, pos] : nodePositions) {
                                bool hit = false;
                                if (workingGraph.getNode(nId).type == NodeType::HOST) {
                                    if (std::abs(clickPos.x - pos.x) <= 22.0f && std::abs(clickPos.y - pos.y) <= 20.0f) hit = true;
                                } else {
                                    float dx = clickPos.x - pos.x, dy = clickPos.y - pos.y;
                                    if (std::sqrt(dx * dx + dy * dy) <= NODE_RADIUS + 4.0f) hit = true;
                                }

                                if (hit) {
                                    if (appState.selectedSrcNode != nId) {
                                        appState.selectedDstNode = nId;
                                        updateRoutingPath(workingGraph, packets, appState);

                                        if (appState.currentPath.reachable && appState.currentPath.nodes.size() >= 2) {
                                            PacketAnim p;
                                            p.sourceNode = appState.selectedSrcNode;
                                            p.destNode = appState.selectedDstNode;
                                            p.path = appState.currentPath.nodes;
                                            p.currentHopIndex = 0;
                                            p.progress = 0.0f;
                                            p.speed = 1.0f;
                                            appState.activePackets.push_back(p);
                                        }
                                    } else {
                                        std::string sName = workingGraph.getNode(nId).name;
                                        if (sName.empty()) sName = "Node " + std::to_string(nId);
                                        appState.statusMessage = "Trạm " + sName + " đang là nguồn. Click chuột trái vào trạm khác để định tuyến.";
                                    }
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }

        // BƯỚC 3.2: Cập nhật thời gian & Hoạt họa gói tin
        sf::Time dtTime = deltaClock.restart();
        float dt = std::min(dtTime.asSeconds(), 0.033f);
        ImGui::SFML::Update(window, dtTime);

        for (auto& pkt : appState.activePackets) {
            if (pkt.isDelivered || pkt.isBlocked) continue;

            // Kiểm tra tức thì nếu sợi cáp gói tin ĐANG BAY TRÊN ĐÓ vừa bị cắt đứt giữa chừng
            if (pkt.currentHopIndex < static_cast<int>(pkt.path.size()) - 1) {
                int curU = pkt.path[pkt.currentHopIndex];
                int curV = pkt.path[pkt.currentHopIndex + 1];
                bool currentHopDown = false;
                for (int eId : workingGraph.getNeighbors(curU)) {
                    const Edge& e = workingGraph.getEdge(eId);
                    if (workingGraph.otherEndpoint(curU, eId) == curV && !e.isUp) {
                        currentHopDown = true;
                        break;
                    }
                }
                if (currentHopDown) {
                    Packet reroutePkt = buildCurrentPacket(appState, packets, curU, pkt.destNode);
                    PathResult newPath = dijkstra(workingGraph, curU, pkt.destNode, reroutePkt, appState.onlyBuiltLinks);
                    if (newPath.reachable) {
                        pkt.path = newPath.nodes;
                        pkt.currentHopIndex = 0;
                        pkt.progress = 0.0f;
                        std::string uName = workingGraph.getNode(curU).name.empty() ? std::to_string(curU) : workingGraph.getNode(curU).name;
                        appState.statusMessage = "Cáp đang truyền bị cắt! Gói tin tự động thu hồi về " + uName + " và chuyển hướng!";
                    } else {
                        pkt.isBlocked = true;
                        appState.statusMessage = "DROP: Cáp đang truyền bị cắt đứt và không còn đường thay thế!";
                        continue;
                    }
                }
            }

            pkt.progress += pkt.speed * dt;
            if (pkt.progress >= 1.0f) {
                pkt.progress = 0.0f;
                pkt.currentHopIndex++;
                if (pkt.currentHopIndex >= static_cast<int>(pkt.path.size()) - 1) {
                    pkt.isDelivered = true;
                    appState.ripples.push_back(DeliveryRipple{pkt.destNode, 0.0f});
                    appState.statusMessage = "Gói tin đã chuyển đến đích thành công!";
                } else {
                    int u = pkt.path[pkt.currentHopIndex];
                    int v = pkt.path[pkt.currentHopIndex + 1];
                    bool linkBroken = false;
                    for (int eId : workingGraph.getNeighbors(u)) {
                        const Edge& e = workingGraph.getEdge(eId);
                        if (workingGraph.otherEndpoint(u, eId) == v && !e.isUp) {
                            linkBroken = true;
                            break;
                        }
                    }
                    if (linkBroken) {
                        Packet reroutePkt = buildCurrentPacket(appState, packets, u, pkt.destNode);
                        PathResult newPath = dijkstra(workingGraph, u, pkt.destNode, reroutePkt, appState.onlyBuiltLinks);
                        if (newPath.reachable) {
                            pkt.path = newPath.nodes;
                            pkt.currentHopIndex = 0;
                            pkt.progress = 0.0f;
                            std::string uName = workingGraph.getNode(u).name.empty() ? std::to_string(u) : workingGraph.getNode(u).name;
                            appState.statusMessage = "Cáp đứt phía trước! Tự động chuyển hướng từ trạm " + uName;
                        } else {
                            pkt.isBlocked = true;
                            appState.statusMessage = "DROP: Tuyến phía trước bị đứt và không có đường thay thế!";
                        }
                    }
                }
            }
        }

        for (auto& rip : appState.ripples) {
            rip.timer += dt * 1.5f;
        }
        appState.ripples.erase(
            std::remove_if(appState.ripples.begin(), appState.ripples.end(),
                           [](const DeliveryRipple& r) { return r.timer >= 1.0f; }),
            appState.ripples.end()
        );

        // BƯỚC 3.3: Xác định Node & Edge đang được hover
        appState.hoveredNodeId = -1;
        int hoveredEdgeId = -1;
        if (!ImGui::GetIO().WantCaptureMouse) {
            for (const auto& [nId, pos] : nodePositions) {
                float dx = mousePos.x - pos.x, dy = mousePos.y - pos.y;
                if (std::sqrt(dx * dx + dy * dy) <= NODE_RADIUS + 2.0f) {
                    appState.hoveredNodeId = nId;
                    break;
                }
            }

            if (appState.isCutModeActive || appState.isRestoreModeActive) {
                float minCutDist = CUT_HITBOX_TOLERANCE;
                for (int eId : workingGraph.edgeIds()) {
                    const Edge& e = workingGraph.getEdge(eId);
                    if (!e.isBuilt || !Validation::isStaticFeasible(e)) continue;

                    if (nodePositions.count(e.u) && nodePositions.count(e.v)) {
                        bool curved = isPairMatch(e.u, e.v, curvedDiagU, curvedDiagV);
                        float d = getDistanceToEdge(mousePos, curved, nodePositions.at(e.u), nodePositions.at(e.v), diagCtrl);
                        if (d < minCutDist) {
                            minCutDist = d;
                            hoveredEdgeId = eId;
                        }
                    }
                }
            }
        }

        // Module 1: Thanh công cụ trên cùng
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(ImVec2(static_cast<float>(WINDOW_WIDTH), TOP_BAR_HEIGHT));
        ImGuiWindowFlags toolbarFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                                        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14.0f, 6.0f));
        if (ImGui::Begin("##TopToolbar", nullptr, toolbarFlags)) {
            ImGui::AlignTextToFramePadding();
            ImGui::TextColored(ImVec4(0.2f, 0.4f, 0.7f, 1.0f), "CHẾ ĐỘ:");
            ImGui::SameLine(0, 8.0f);

            if (ImGui::RadioButton("Lớp Vật Lý", &appState.viewMode, 0)) {
                // Chuyển sang Lớp Vật Lý
            }
            ImGui::SameLine(0, 10.0f);
            if (ImGui::RadioButton("Lớp Logic", &appState.viewMode, 1)) {
                // Chuyển sang Lớp Logic -> Tắt chế độ xem cây khung MST để đường định tuyến không bị đè
                appState.isMSTActive = false;
            }

            ImGui::SameLine(0, 16.0f);
            ImGui::TextColored(ImVec4(0.75f, 0.78f, 0.82f, 1.0f), "|");
            ImGui::SameLine(0, 16.0f);

            // 1. Nút Chạy MST
            if (DrawToggleButton("Chạy MST", appState.isMSTActive, ImVec4(0.12f, 0.46f, 0.85f, 1.0f))) {
                appState.isMSTActive = !appState.isMSTActive;
                if (appState.isMSTActive) {
                    appState.hasRunMST = true;
                    appState.viewMode = 0; // TỰ ĐỘNG CHUYỂN SANG LỚP VẬT LÝ ĐỂ HIỂN THỊ RÕ CÂY KHUNG
                    appState.isScenarioActive = false;
                    appState.isSidebarOpen = true;
                    appState.sidebarMode = 0;
                    appState.switchTabRequested = true;
                    appState.requestedTab = 0;
                    // Hiển thị quy hoạch gốc tính trên baseGraph, không ghi đè cờ isBuilt của workingGraph
                    recordKruskalSteps(baseGraph, pricingModel, appState, /*updateGraphBuildFlags=*/false);
                    appState.currentKruskalStep = static_cast<int>(appState.kruskalHistory.size());
                    bool hasDownLink = false;
                    for (int eId : workingGraph.edgeIds()) {
                        if (!workingGraph.getEdge(eId).isUp) {
                            hasDownLink = true;
                            break;
                        }
                    }
                    if (hasDownLink) {
                        appState.statusMessage = "Đang xem quy hoạch hạ tầng gốc. Cáp đứt khi vận hành không làm thay đổi cây khung ban đầu.";
                    } else {
                        appState.statusMessage = "Chế độ MST: Đã tính toán cây khung và cáp dự phòng.";
                    }
                } else {
                    appState.statusMessage = "Đã tắt chế độ xem cây khung MST.";
                }
            }
            ImGui::SameLine(0, 8.0f);

            // 2. Nút Kịch Bản
            if (DrawToggleButton("Kịch Bản", appState.isScenarioActive, ImVec4(0.38f, 0.42f, 0.76f, 1.0f))) {
                if (appState.isScenarioActive && appState.isSidebarOpen && appState.sidebarMode == 1) {
                    appState.isScenarioActive = false;
                    appState.isSidebarOpen = false;
                } else {
                    appState.isScenarioActive = true;
                    appState.isMSTActive = false;
                    appState.viewMode = 1; // Tự động chuyển sang Lớp Logic khi bật Kịch Bản
                    appState.isSidebarOpen = true;
                    appState.sidebarMode = 1;
                    appState.switchTabRequested = true;
                    appState.requestedTab = 1;
                    applyScenarioToStep(workingGraph, baseGraph, pricingModel, packets, appState.loadedScenario, appState.currentScenarioStep, appState);
                    appState.statusMessage = "Chế độ Kịch Bản: Đang kích hoạt " + appState.scenarioNames[appState.activeScenarioIdx];
                }
            }

            if (appState.isScenarioActive) {
                ImGui::SameLine(0, 8.0f);
                if (ImGui::Button("<")) {
                    if (appState.currentScenarioStep > 0) {
                        appState.currentScenarioStep--;
                        applyScenarioToStep(workingGraph, baseGraph, pricingModel, packets, appState.loadedScenario, appState.currentScenarioStep, appState);
                    }
                }
                ImGui::SameLine(0, 4.0f);
                ImGui::TextColored(ImVec4(0.3f, 0.3f, 0.35f, 1.0f), "Bước %d/%d",
                                   appState.maxScenarioSteps == 0 ? 0 : appState.currentScenarioStep + 1,
                                   appState.maxScenarioSteps);
                ImGui::SameLine(0, 4.0f);
                if (ImGui::Button(">")) {
                    if (appState.currentScenarioStep < appState.maxScenarioSteps - 1) {
                        appState.currentScenarioStep++;
                        applyScenarioToStep(workingGraph, baseGraph, pricingModel, packets, appState.loadedScenario, appState.currentScenarioStep, appState);
                    }
                }
            }
            ImGui::SameLine(0, 8.0f);

            // 3. Nút Cắt Cáp
            const char* cutLabel = appState.isCutModeActive ? "Đang Cắt Cáp..." : "Cắt Cáp";
            if (DrawToggleButton(cutLabel, appState.isCutModeActive, ImVec4(0.86f, 0.22f, 0.22f, 1.0f))) {
                appState.isCutModeActive = !appState.isCutModeActive;
                if (appState.isCutModeActive) {
                    appState.isRestoreModeActive = false;
                    appState.statusMessage = "Chế độ Cắt Cáp: Click trực tiếp vào cáp trên sơ đồ để ngắt kết nối.";
                } else {
                    appState.statusMessage = "Đã thoát chế độ Cắt Cáp.";
                }
            }
            ImGui::SameLine(0, 6.0f);

            // 4. Nút Khôi Phục Cáp
            const char* restoreLabel = appState.isRestoreModeActive ? "Đang Nối Cáp..." : "Khôi Phục";
            if (DrawToggleButton(restoreLabel, appState.isRestoreModeActive, ImVec4(0.16f, 0.65f, 0.30f, 1.0f))) {
                appState.isRestoreModeActive = !appState.isRestoreModeActive;
                if (appState.isRestoreModeActive) {
                    appState.isCutModeActive = false;
                    appState.statusMessage = "Chế độ Khôi Phục: Click vào cáp đang đứt trên sơ đồ để nối lại.";
                } else {
                    appState.statusMessage = "Đã thoát chế độ Khôi Phục.";
                }
            }
            // Nhóm bên phải: Lưu Mạng, Đặt Lại, Bảng số liệu & Thoát
            ImGui::SameLine(WINDOW_WIDTH - 335.0f);
            if (ImGui::Button(" Lưu ")) {
                std::filesystem::create_directories("data");
                std::ofstream out("data/saved_network.txt");
                if (out.is_open()) {
                    out << "# CAU HINH TRANG THAI MANG DA LUU (PBL2)\n";
                    out << "# edgeId u v length isUp\n";
                    for (int eId : workingGraph.edgeIds()) {
                        const Edge& e = workingGraph.getEdge(eId);
                        out << e.id << " " << e.u << " " << e.v << " " << e.length << " " << (e.isUp ? 1 : 0) << "\n";
                    }
                    out.close();
                    appState.statusMessage = "[Đã Lưu]: Đã lưu trạng thái mạng. Lần sau mở app sẽ tự động nạp tiếp sơ đồ này!";
                }
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Lưu trạng thái mạng hiện tại. Khi khởi động lại app sẽ tự động tiếp tục sơ đồ này.");
            }

            ImGui::SameLine(0, 6.0f);
            if (ImGui::Button(" Đặt Lại ")) {
                workingGraph = baseGraph;
                appState.reset(defaultNodeId);
                recordKruskalSteps(workingGraph, pricingModel, appState, /*updateGraphBuildFlags=*/true);
                if (std::filesystem::exists("data/saved_network.txt")) {
                    std::filesystem::remove("data/saved_network.txt");
                }
                appState.statusMessage = "Đã khôi phục mạng về trạng thái ban đầu và làm mới sơ đồ.";
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Khôi phục mạng về trạng thái sạch ban đầu.");
            }
            ImGui::SameLine(0, 5.0f);
            std::string drawerBtnLabel = appState.isSidebarOpen ? " Ẩn Bảng [X] " : " Bảng Số Liệu ";
            if (ImGui::Button(drawerBtnLabel.c_str())) {
                appState.isSidebarOpen = !appState.isSidebarOpen;
            }
            ImGui::SameLine(0, 5.0f);
            if (ImGui::Button(" Thoát ")) {
                window.close();
            }
        }
        ImGui::End();
        ImGui::PopStyleVar();

        // Thẻ nổi chọn nguồn/đích và thông số định tuyến ở Lớp Logic
        if (appState.viewMode == 1) {
            float pillWidth = appState.isSidebarOpen ? (WINDOW_WIDTH - DRAWER_DEFAULT_WIDTH - 24.0f) : 855.0f;
            ImGui::SetNextWindowPos(ImVec2(16.0f, TOP_BAR_HEIGHT + 6.0f), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(pillWidth, 66.0f), ImGuiCond_Always);
            ImGuiWindowFlags pillFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                                         ImGuiWindowFlags_NoSavedSettings;
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(1.0f, 1.0f, 1.0f, 0.97f));
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.12f, 0.46f, 0.85f, 0.70f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.5f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 6.0f));

            if (ImGui::Begin("##RoutingFloatingPill", nullptr, pillFlags)) {
                // HÀNG 1: CHỌN NGUỒN, ĐÍCH, GÓI TIN PRESET, DỊCH VỤ QoS & NÚT GỬI GÓI TIN
                ImGui::AlignTextToFramePadding();
                ImGui::TextColored(ImVec4(0.12f, 0.46f, 0.85f, 1.0f), "Nguồn:");
                ImGui::SameLine(0, 3.0f);
                ImGui::SetNextItemWidth(60.0f);
                int curSrcIdx = 0;
                for (int i = 0; i < static_cast<int>(allNodeIds.size()); ++i) {
                    if (allNodeIds[i] == appState.selectedSrcNode) curSrcIdx = i;
                }
                if (ImGui::Combo("##SrcCombo", &curSrcIdx, [](void* data, int idx, const char** out_text) {
                    auto& vec = *static_cast<std::vector<std::string>*>(data);
                    if (idx < 0 || idx >= static_cast<int>(vec.size())) return false;
                    *out_text = vec[idx].c_str();
                    return true;
                }, &nodeLabels, static_cast<int>(nodeLabels.size()))) {
                    appState.selectedSrcNode = allNodeIds[curSrcIdx];
                    if (appState.selectedDstNode == appState.selectedSrcNode) appState.selectedDstNode = -1;
                    updateRoutingPath(workingGraph, packets, appState);
                }

                ImGui::SameLine(0, 6.0f);
                ImGui::TextColored(ImVec4(0.85f, 0.35f, 0.1f, 1.0f), "-> Đích:");
                ImGui::SameLine(0, 3.0f);
                ImGui::SetNextItemWidth(65.0f);
                int curDstIdx = -1;
                for (int i = 0; i < static_cast<int>(allNodeIds.size()); ++i) {
                    if (allNodeIds[i] == appState.selectedDstNode) curDstIdx = i;
                }
                const char* dstPreview = (curDstIdx >= 0) ? nodeLabels[curDstIdx].c_str() : "--";
                if (ImGui::BeginCombo("##DstCombo", dstPreview)) {
                    for (int i = 0; i < static_cast<int>(allNodeIds.size()); ++i) {
                        if (allNodeIds[i] == appState.selectedSrcNode) continue;
                        bool isSel = (curDstIdx == i);
                        if (ImGui::Selectable(nodeLabels[i].c_str(), isSel)) {
                            appState.selectedDstNode = allNodeIds[i];
                            updateRoutingPath(workingGraph, packets, appState);
                        }
                    }
                    ImGui::EndCombo();
                }

                ImGui::SameLine(0, 8.0f);
                ImGui::Text("Gói:");
                ImGui::SameLine(0, 3.0f);
                ImGui::SetNextItemWidth(210.0f);
                if (ImGui::BeginCombo("##PktPresetCombo", packetPresetLabels[appState.selectedPacketPreset].c_str())) {
                    for (int i = 0; i < static_cast<int>(packetPresetLabels.size()); ++i) {
                        bool isSel = (appState.selectedPacketPreset == i);
                        if (ImGui::Selectable(packetPresetLabels[i].c_str(), isSel)) {
                            appState.selectedPacketPreset = i;
                            updateRoutingPath(workingGraph, packets, appState);
                        }
                    }
                    ImGui::EndCombo();
                }

                Packet curPkt = buildCurrentPacket(appState, packets, appState.selectedSrcNode, appState.selectedDstNode);
                ImGui::SameLine(0, 10.0f);
                if (curPkt.type == PacketType::REAL_TIME) {
                    ImGui::TextColored(ImVec4(0.15f, 0.65f, 0.25f, 1.0f), "[QoS: REAL_TIME]");
                } else if (curPkt.type == PacketType::BULK_DATA) {
                    ImGui::TextColored(ImVec4(0.15f, 0.45f, 0.85f, 1.0f), "[QoS: BULK_DATA]");
                } else {
                    ImGui::TextColored(ImVec4(0.45f, 0.50f, 0.55f, 1.0f), "[QoS: CONTROL]");
                }


                if (appState.currentPath.reachable && appState.currentPath.nodes.size() >= 2) {
                    ImGui::SameLine(0, 10.0f);
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.62f, 0.28f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                    if (ImGui::Button(" Gửi Gói Tin ")) {
                        PacketAnim p;
                        p.sourceNode = appState.selectedSrcNode;
                        p.destNode = appState.selectedDstNode;
                        p.path = appState.currentPath.nodes;
                        p.currentHopIndex = 0;
                        p.progress = 0.0f;
                        p.speed = 1.0f;
                        appState.activePackets.push_back(p);
                        std::string sN = workingGraph.getNode(appState.selectedSrcNode).name;
                        std::string dN = workingGraph.getNode(appState.selectedDstNode).name;
                        appState.statusMessage = "Đang truyền gói tin từ " + sN + " -> " + dN + "...";
                    }
                    ImGui::PopStyleColor(2);
                }

                // HÀNG 2: KẾT QUẢ ĐỊNH TUYẾN, ĐỘ TRỄ, PHÂN MẢNH MTU, TCP HANDSHAKE & REROUTE
                std::string sN = workingGraph.getNode(appState.selectedSrcNode).name;
                std::string dN = (appState.selectedDstNode != -1) ? workingGraph.getNode(appState.selectedDstNode).name : "";
                Packet activePkt = buildCurrentPacket(appState, packets, appState.selectedSrcNode, appState.selectedDstNode);

                if (appState.selectedDstNode == -1) {
                    ImGui::TextColored(ImVec4(0.45f, 0.50f, 0.55f, 1.0f), "Click chuột trái: Chọn Đích  |  Chuột phải: Đổi Nguồn");
                } else if (appState.currentPath.reachable) {
                    std::string pathSummary = formatNodePath(workingGraph, appState.currentPath.nodes);
                    double distM = calculatePathPhysicalLength(workingGraph, appState.currentPath.nodes);

                    ImGui::TextColored(ImVec4(0.12f, 0.62f, 0.28f, 1.0f), "%s", pathSummary.c_str());
                    ImGui::SameLine(0, 6.0f);
                    ImGui::TextColored(ImVec4(0.30f, 0.35f, 0.42f, 1.0f),
                        "(%dm | Trễ: %.3fms | %d hops | %d mảnh%s)",
                        static_cast<int>(distM),
                        appState.logicSimResult.totalDelay * 1000.0,
                        appState.currentPath.hopCount,
                        appState.logicSimResult.fragmentCount,
                        (activePkt.protocol == Protocol::TCP && activePkt.newConnection) ? " | TCP+RTT" : " | UDP");

                    if (appState.isRerouted) {
                        ImGui::SameLine(0, 8.0f);
                        ImGui::TextColored(ImVec4(0.88f, 0.35f, 0.08f, 1.0f),
                            "[REROUTE từ %s]", appState.previousRouteStr.c_str());
                    }
                } else {
                    if (appState.dropByMtu) {
                        ImGui::TextColored(ImVec4(0.85f, 0.18f, 0.18f, 1.0f),
                            "[DROP - DF_BLOCKED_BY_MTU] Gói %dB vượt MTU %dB nhưng cấm phân mảnh!",
                            activePkt.sizeBytes, appState.dropMtu);
                    } else {
                        ImGui::TextColored(ImVec4(0.85f, 0.18f, 0.18f, 1.0f),
                            "[DROP - UNREACHABLE] Mất liên lạc từ %s đến %s trên hạ tầng cáp đã lắp đặt!",
                            sN.c_str(), dN.c_str());
                    }
                }
            }
            ImGui::End();
            ImGui::PopStyleVar(3);
            ImGui::PopStyleColor(2);
        }

        // Module 3: Ngăn kéo bảng dữ liệu bên phải
        if (appState.isSidebarOpen) {
            ImGui::SetNextWindowPos(ImVec2(WINDOW_WIDTH - DRAWER_DEFAULT_WIDTH, TOP_BAR_HEIGHT), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSizeConstraints(ImVec2(DRAWER_MIN_WIDTH, DRAWER_HEIGHT), ImVec2(DRAWER_MAX_WIDTH, DRAWER_HEIGHT));

            ImGuiWindowFlags drawerFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(1.0f, 1.0f, 1.0f, 0.98f));
            if (ImGui::Begin("Bảng Dữ Liệu##Sidebar", &appState.isSidebarOpen, drawerFlags)) {
                if (ImGui::BeginTabBar("SidebarTabs")) {
                    // TAB 1: QUY HOẠCH CÁP (MST) & SO SÁNH 3 MỨC NGÂN SÁCH
                    ImGuiTabItemFlags tab1Flags = 0;
                    if (appState.switchTabRequested && appState.requestedTab == 0) {
                        tab1Flags |= ImGuiTabItemFlags_SetSelected;
                    }
                    if (ImGui::BeginTabItem(" Quy Hoạch Cáp ", nullptr, tab1Flags)) {
                        appState.sidebarMode = 0;
                        appState.isMSTActive = true;
                        appState.hasRunMST = true;
                        appState.isScenarioActive = false;

                        // 1. BẢNG SO SÁNH 3 MỨC NGÂN SÁCH
                        double costAll = 0.0;
                        int feasibleEdgeCount = 0;
                        for (int eId : baseGraph.edgeIds()) {
                            const Edge& bEdge = baseGraph.getEdge(eId);
                            if (Validation::isStaticFeasible(bEdge)) {
                                double c = costFn(bEdge);
                                if (!std::isinf(c)) {
                                    costAll += c;
                                    feasibleEdgeCount++;
                                }
                            }
                        }

                        double costMST = appState.mstResult.totalCost;
                        double costBackup = appState.backupResult.totalCost;
                        double costMSTBackup = costMST + costBackup;
                        double savedPct = (costAll > 0.0 && !std::isinf(costAll) && !std::isinf(costMST)) ? ((costAll - costMST) / costAll * 100.0) : 0.0;

                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.12f, 0.46f, 0.85f, 1.0f), "SO SÁNH 3 MỨC NGÂN SÁCH:");
                        if (ImGui::BeginTable("BudgetCompare", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                            ImGui::TableSetupColumn("Phương Án", ImGuiTableColumnFlags_WidthFixed, 118.0f);
                            ImGui::TableSetupColumn("Chi Phí", ImGuiTableColumnFlags_WidthFixed, 74.0f);
                            ImGui::TableSetupColumn("Đặc Điểm", ImGuiTableColumnFlags_WidthStretch);
                            ImGui::TableHeadersRow();

                            // 1. Toàn Bộ
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("1. Toàn bộ");
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%s", formatCurrency(costAll).c_str());
                            ImGui::TableSetColumnIndex(2);
                            ImGui::TextColored(ImVec4(0.85f, 0.25f, 0.25f, 1.0f), "Tốn kém (%d cáp)", feasibleEdgeCount);

                            // 2. Cây Khung (MST)
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::TextColored(ImVec4(0.12f, 0.62f, 0.28f, 1.0f), "2. Cây khung");
                            ImGui::TableSetColumnIndex(1);
                            if (std::isinf(costMST) || !appState.mstResult.connected) {
                                ImGui::TextColored(ImVec4(0.85f, 0.2f, 0.2f, 1.0f), "Không khả thi");
                            } else {
                                ImGui::TextColored(ImVec4(0.12f, 0.62f, 0.28f, 1.0f), "%s", formatCurrency(costMST).c_str());
                            }
                            ImGui::TableSetColumnIndex(2);
                            if (appState.mstResult.connected) {
                                ImGui::TextColored(ImVec4(0.12f, 0.62f, 0.28f, 1.0f), "Rẻ nhất (-%.0f%%)", savedPct);
                            } else {
                                ImGui::TextColored(ImVec4(0.85f, 0.2f, 0.2f, 1.0f), "Bị chia cắt");
                            }

                            // 3. MST + Dự Phòng (Cùng mất đi nếu MST không liên thông)
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::TextColored(ImVec4(0.15f, 0.45f, 0.95f, 1.0f), "3. MST + Dự phòng");
                            ImGui::TableSetColumnIndex(1);
                            if (std::isinf(costMST) || !appState.mstResult.connected) {
                                ImGui::TextColored(ImVec4(0.85f, 0.2f, 0.2f, 1.0f), "Không khả thi");
                            } else {
                                ImGui::TextColored(ImVec4(0.15f, 0.45f, 0.95f, 1.0f), "%s", formatCurrency(costMSTBackup).c_str());
                            }
                            ImGui::TableSetColumnIndex(2);
                            if (appState.mstResult.connected) {
                                ImGui::TextColored(ImVec4(0.15f, 0.45f, 0.95f, 1.0f), "Chịu lỗi (+%d cáp)", static_cast<int>(appState.backupResult.edgeIds.size()));
                            } else {
                                ImGui::TextColored(ImVec4(0.85f, 0.2f, 0.2f, 1.0f), "Mạng không liên thông");
                            }

                            ImGui::EndTable();
                        }

                        // Chú thích màu sắc ngắn gọn
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.12f, 0.62f, 0.28f, 1.0f), "[Xanh lá] MST");
                        ImGui::SameLine(0, 10.0f);
                        ImGui::TextColored(ImVec4(0.15f, 0.45f, 0.95f, 1.0f), "[Xanh dương] Dự phòng");
                        ImGui::SameLine(0, 10.0f);
                        ImGui::TextColored(ImVec4(0.55f, 0.60f, 0.65f, 1.0f), "[Xám] Không dùng");

                        // Cảnh báo nếu đồ thị bị chia cắt
                        if (!appState.mstResult.connected) {
                            ImGui::Spacing();
                            ImGui::TextColored(ImVec4(0.85f, 0.2f, 0.2f, 1.0f), "[! CẢNH BÁO] Đồ thị bị chia cắt! Không thể tạo MST.");
                        }

                        // Ghi chú khi đang có cáp đứt ở tầng vận hành
                        bool hasDownLinkInTab = false;
                        for (int eId : workingGraph.edgeIds()) {
                            if (!workingGraph.getEdge(eId).isUp) {
                                hasDownLinkInTab = true;
                                break;
                            }
                        }
                        if (hasDownLinkInTab) {
                            ImGui::Spacing();
                            ImGui::TextColored(ImVec4(0.85f, 0.42f, 0.05f, 1.0f), "* Đang có cáp đứt khi vận hành; bảng trên là quy hoạch gốc.");
                        }

                        // 2. THUẬT TOÁN KRUSKAL TỪNG BƯỚC
                        ImGui::Spacing();
                        ImGui::Separator();
                        ImGui::TextColored(ImVec4(0.12f, 0.46f, 0.85f, 1.0f), "Thuật toán Kruskal Từng Bước:");
                        
                        ImGui::Text("Tiến trình: Bước %d / %d", appState.currentKruskalStep, static_cast<int>(appState.kruskalHistory.size()));
                        if (ImGui::Button(" [<< Bước trước] ") && appState.currentKruskalStep > 0) {
                            appState.currentKruskalStep--;
                        }
                        ImGui::SameLine();
                        if (ImGui::Button(" [Bước kế tiếp >>] ") && appState.currentKruskalStep < static_cast<int>(appState.kruskalHistory.size())) {
                            appState.currentKruskalStep++;
                        }
                        ImGui::SameLine();
                        if (ImGui::Button(" [Xem Tất Cả] ")) {
                            appState.currentKruskalStep = static_cast<int>(appState.kruskalHistory.size());
                        }

                        ImGui::Separator();
                        ImGui::TextColored(ImVec4(0.2f, 0.4f, 0.7f, 1.0f), "Lịch sử duyệt cạnh:");
                        if (ImGui::BeginChild("KruskalLogList", ImVec2(0, 250), true)) {
                            for (int i = 0; i < appState.currentKruskalStep; ++i) {
                                const auto& st = appState.kruskalHistory[i];
                                if (st.isSelected) {
                                    ImGui::TextColored(ImVec4(0.15f, 0.65f, 0.25f, 1.0f), "[%d] %s", i + 1, st.description.c_str());
                                } else {
                                    ImGui::TextColored(ImVec4(0.85f, 0.25f, 0.25f, 1.0f), "[%d] %s", i + 1, st.description.c_str());
                                }
                            }
                            if (appState.currentKruskalStep < static_cast<int>(appState.kruskalHistory.size())) {
                                const auto& nextSt = appState.kruskalHistory[appState.currentKruskalStep];
                                std::string uName = workingGraph.getNode(nextSt.u).name.empty() ? std::to_string(nextSt.u) : workingGraph.getNode(nextSt.u).name;
                                std::string vName = workingGraph.getNode(nextSt.v).name.empty() ? std::to_string(nextSt.v) : workingGraph.getNode(nextSt.v).name;
                                ImGui::TextColored(ImVec4(0.9f, 0.65f, 0.1f, 1.0f), ">> Đang xét: %s - %s (Chi phí: %s)", uName.c_str(), vName.c_str(), formatCurrency(nextSt.weight).c_str());
                            }
                        }
                        ImGui::EndChild();

                        // NÚT XUẤT BÁO CÁO QUY HOẠCH CÁP (.TXT) PHỤC VỤ BÁO CÁO PBL2
                        ImGui::Spacing();
                        if (ImGui::Button(" [Xuất Báo Cáo Quy Hoạch Cáp] ", ImVec2(-1.0f, 28.0f))) {
                            std::filesystem::create_directories("data");
                            std::ofstream out("data/bao_cao_quy_hoach_mst.txt");
                            if (out.is_open()) {
                                out << "================================================================================\n";
                                out << "          BAO CAO KET QUA QUY HOACH HA TANG CAP VIEN THONG (PBL2)\n";
                                out << "================================================================================\n\n";
                                
                                out << "1. SO SANH 3 MUC NGAN SACH DAU TU:\n";
                                out << "   - Phuong an 1 (Toan bo cap kha thi): " << formatCurrency(costAll) << " (" << feasibleEdgeCount << " cap - Ton kem, du thua)\n";
                                out << "   - Phuong an 2 (Cay khung toi uu MST): " << formatCurrency(costMST) << " (" << appState.mstResult.edgeIds.size() << " cap - Tiet kiem " << static_cast<int>(savedPct) << "%)\n";
                                out << "   - Phuong an 3 (MST + Cap du phong):   " << formatCurrency(costMSTBackup) << " (" << (appState.mstResult.edgeIds.size() + appState.backupResult.edgeIds.size()) << " cap - Bao ve " << appState.backupResult.edgeIds.size() << " canh)\n\n";

                                out << "2. DANH SACH TUYEN CAP CAY KHUNG (MST THI CONG CHINH):\n";
                                int idx = 1;
                                for (int eId : appState.mstResult.edgeIds) {
                                    const Edge& e = baseGraph.getEdge(eId);
                                    std::string uN = baseGraph.getNode(e.u).name;
                                    std::string vN = baseGraph.getNode(e.v).name;
                                    std::string mType = (e.mediaType == MediaType::FIBER) ? "Fiber" : "Copper";
                                    out << "   [" << idx++ << "] Tuyen " << uN << " - " << vN << " | Cu ly: " << static_cast<int>(e.length) 
                                        << "m | Loai: " << mType << " | Chi phi: " << formatCurrency(costFn(e)) << "\n";
                                }

                                out << "\n3. DANH SACH TUYEN CAP DU PHONG (BACKUP):\n";
                                idx = 1;
                                for (int bId : appState.backupResult.edgeIds) {
                                    const Edge& e = baseGraph.getEdge(bId);
                                    std::string uN = baseGraph.getNode(e.u).name;
                                    std::string vN = baseGraph.getNode(e.v).name;
                                    std::string mType = (e.mediaType == MediaType::FIBER) ? "Fiber" : "Copper";
                                    out << "   [" << idx++ << "] Tuyen " << uN << " - " << vN << " | Cu ly: " << static_cast<int>(e.length) 
                                        << "m | Loai: " << mType << " | Chi phi: " << formatCurrency(costFn(e)) << "\n";
                                }

                                out << "\n4. TONG KET HA TANG XAY DUNG:\n";
                                out << "   - Tong so tram mang: " << baseGraph.nodeIds().size() << " (Router & Host)\n";
                                out << "   - Tong tuyen cap thi cong: " << (appState.mstResult.edgeIds.size() + appState.backupResult.edgeIds.size()) << " tuyen\n";
                                out << "   - Tong chi phi thi cong: " << static_cast<int>(costMSTBackup) << " DV\n";
                                out << "================================================================================\n";
                                out.close();
                                appState.statusMessage = "[Thành công]: Đã xuất báo cáo quy hoạch cáp ra file data/bao_cao_quy_hoach_mst.txt";
                            }
                        }
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("Xuất phương án thi công và danh sách cáp MST ra file text để làm báo cáo.");
                        }

                        ImGui::EndTabItem();
                    }

                    // TAB 2: MÔ PHỎNG KỊCH BẢN & KẾT QUẢ TRUYỀN PACKETS.TXT & LOGGER
                    ImGuiTabItemFlags tab2Flags = 0;
                    if (appState.switchTabRequested && appState.requestedTab == 1) {
                        tab2Flags |= ImGuiTabItemFlags_SetSelected;
                    }
                    if (ImGui::BeginTabItem(" Mô Phỏng Kịch Bản ", nullptr, tab2Flags)) {
                        appState.sidebarMode = 1;
                        appState.isScenarioActive = true;
                        appState.isMSTActive = false;

                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.75f, 0.50f, 0.10f, 1.0f), "Chọn Kịch Bản:");
                        ImGui::SetNextItemWidth(-1.0f);
                        if (ImGui::BeginCombo("##ScenarioCombo", appState.scenarioNames[appState.activeScenarioIdx].c_str())) {
                            for (int i = 0; i < static_cast<int>(appState.scenarioNames.size()); ++i) {
                                bool isSel = (appState.activeScenarioIdx == i);
                                if (ImGui::Selectable(appState.scenarioNames[i].c_str(), isSel)) {
                                    appState.activeScenarioIdx = i;
                                    IOManager::loadScenario("data/scenarios/" + appState.scenarioFiles[i], appState.loadedScenario);
                                    appState.maxScenarioSteps = static_cast<int>(appState.loadedScenario.events.size());
                                    appState.currentScenarioStep = 0;
                                    // Kịch bản mới: quên lộ trình của kịch bản cũ để không hiện nhầm nhãn REROUTE
                                    appState.isRerouted = false;
                                    appState.previousRouteStr.clear();
                                    appState.lastRouteNodes.clear();
                                    appState.lastRouteSrc = -1;
                                    appState.lastRouteDst = -1;
                                    if (appState.selectedDstNode == -1 && !packets.empty()) {
                                        appState.selectedSrcNode = packets.front().source;
                                        appState.selectedDstNode = packets.front().destination;
                                    }
                                    applyScenarioToStep(workingGraph, baseGraph, pricingModel, packets, appState.loadedScenario, 0, appState);
                                    appState.statusMessage = "Đã nạp kịch bản: " + appState.scenarioNames[i];
                                }
                            }
                            ImGui::EndCombo();
                        }

                        ImGui::Separator();
                        ImGui::Text("Tiến trình: Bước %d / %d", appState.maxScenarioSteps == 0 ? 0 : appState.currentScenarioStep + 1, appState.maxScenarioSteps);
                        if (ImGui::Button(" [<< Lùi bước] ") && appState.currentScenarioStep > 0) {
                            appState.currentScenarioStep--;
                            applyScenarioToStep(workingGraph, baseGraph, pricingModel, packets, appState.loadedScenario, appState.currentScenarioStep, appState);
                        }
                        ImGui::SameLine();
                        if (ImGui::Button(" [Tiến bước >>] ") && appState.currentScenarioStep < appState.maxScenarioSteps - 1) {
                            appState.currentScenarioStep++;
                            applyScenarioToStep(workingGraph, baseGraph, pricingModel, packets, appState.loadedScenario, appState.currentScenarioStep, appState);
                        }
                        ImGui::SameLine();
                        if (ImGui::Button(" [Chạy từ đầu] ")) {
                            appState.currentScenarioStep = 0;
                            applyScenarioToStep(workingGraph, baseGraph, pricingModel, packets, appState.loadedScenario, appState.currentScenarioStep, appState);
                        }

                        ImGui::Separator();
                        ImGui::TextColored(ImVec4(0.2f, 0.4f, 0.7f, 1.0f), "Danh sách sự kiện:");
                        if (ImGui::BeginChild("ScenarioEvents", ImVec2(0, 340), true)) {
                            if (appState.loadedScenario.events.empty()) {
                                ImGui::TextColored(ImVec4(0.45f, 0.50f, 0.55f, 1.0f),
                                    "Kịch bản tĩnh.");
                            }
                            for (int i = 0; i < static_cast<int>(appState.loadedScenario.events.size()); ++i) {
                                const auto& ev = appState.loadedScenario.events[i];
                                std::string evName = EnumUtils::toString(ev.eventType);
                                
                                std::string edgeName = "";
                                if (workingGraph.hasEdge(ev.targetId)) {
                                    const auto& ed = workingGraph.getEdge(ev.targetId);
                                    std::string uN = workingGraph.getNode(ed.u).name;
                                    std::string vN = workingGraph.getNode(ed.v).name;
                                    edgeName = " (" + uN + "-" + vN + ")";
                                }

                                char labelBuf[128];
                                if (ev.eventType == EventType::LOAD_UPDATE) {
                                    std::snprintf(labelBuf, sizeof(labelBuf), "[%.1fs] %s Edge #%d%s (Tải: %.0f%%)##ev%d", ev.time, evName.c_str(), ev.targetId, edgeName.c_str(), ev.value * 100.0, i);
                                } else if (ev.eventType == EventType::DELAY_UPDATE) {
                                    std::snprintf(labelBuf, sizeof(labelBuf), "[%.1fs] %s Edge #%d%s (Q: %.2fms)##ev%d", ev.time, evName.c_str(), ev.targetId, edgeName.c_str(), ev.value * 1000.0, i);
                                } else {
                                    std::snprintf(labelBuf, sizeof(labelBuf), "[%.1fs] %s Edge #%d%s##ev%d", ev.time, evName.c_str(), ev.targetId, edgeName.c_str(), i);
                                }

                                bool isCurrent = (i == appState.currentScenarioStep);
                                if (isCurrent) {
                                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.55f, 0.10f, 1.0f));
                                }
                                if (ImGui::Selectable(labelBuf, isCurrent)) {
                                    appState.currentScenarioStep = i;
                                    applyScenarioToStep(workingGraph, baseGraph, pricingModel, packets, appState.loadedScenario, appState.currentScenarioStep, appState);
                                }
                                if (isCurrent) {
                                    ImGui::PopStyleColor();
                                }
                            }
                        }
                        ImGui::EndChild();

                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.45f, 0.50f, 0.55f, 1.0f), "* Click trực tiếp vào sự kiện để xem gói tin đổi đường.");
                        ImGui::EndTabItem();
                    }

                    ImGui::EndTabBar();
                }
                appState.switchTabRequested = false;
            }
            ImGui::End();
            ImGui::PopStyleColor();
        }

        // Module 2: Tooltip bảng định tuyến khi di chuột qua trạm
        if (appState.hoveredNodeId != -1 && !ImGui::GetIO().WantCaptureMouse) {
            std::string hName = workingGraph.getNode(appState.hoveredNodeId).name;
            if (hName.empty()) hName = "Node " + std::to_string(appState.hoveredNodeId);

            Packet samplePkt = buildCurrentPacket(appState, packets, appState.hoveredNodeId, appState.hoveredNodeId);
            std::string qosModeStr = EnumUtils::toString(samplePkt.type);

            ImGui::BeginTooltip();
            ImGui::TextColored(ImVec4(0.12f, 0.46f, 0.85f, 1.0f), "BẢNG ĐỊNH TUYẾN - %s (QoS: %s)", hName.c_str(), qosModeStr.c_str());
            ImGui::Separator();
            if (ImGui::BeginTable("RoutingHover", 4, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Đích");
                ImGui::TableSetupColumn("Next-Hop");
                ImGui::TableSetupColumn("Lộ Trình");
                ImGui::TableSetupColumn("Trễ (ms)");
                ImGui::TableHeadersRow();

                for (int destId : workingGraph.nodeIds()) {
                    if (destId == appState.hoveredNodeId) continue;
                    Packet hoverPkt = buildCurrentPacket(appState, packets, appState.hoveredNodeId, destId);
                    PathResult res = dijkstra(workingGraph, appState.hoveredNodeId, destId, hoverPkt, appState.onlyBuiltLinks);

                    std::string dName = workingGraph.getNode(destId).name;
                    if (dName.empty()) dName = "Node " + std::to_string(destId);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%s", dName.c_str());

                    ImGui::TableSetColumnIndex(1);
                    if (res.reachable && res.nodes.size() >= 2) {
                        std::string nextName = workingGraph.getNode(res.nodes[1]).name;
                        if (nextName.empty()) nextName = "Node " + std::to_string(res.nodes[1]);
                        ImGui::Text("-> %s", nextName.c_str());
                    } else {
                        ImGui::TextColored(ImVec4(0.85f, 0.2f, 0.2f, 1.0f), "DROP");
                    }

                    ImGui::TableSetColumnIndex(2);
                    if (res.reachable) {
                        ImGui::Text("%s", formatNodePath(workingGraph, res.nodes).c_str());
                    } else {
                        ImGui::TextColored(ImVec4(0.85f, 0.2f, 0.2f, 1.0f), "-");
                    }

                    ImGui::TableSetColumnIndex(3);
                    if (res.reachable) {
                        SimulationResult hSim = Simulator::evaluatePacket(hoverPkt, res, workingGraph);
                        ImGui::Text("%.3f ms", hSim.totalDelay * 1000.0);
                    } else {
                        ImGui::TextColored(ImVec4(0.85f, 0.2f, 0.2f, 1.0f), "INF");
                    }
                }
                ImGui::EndTable();
            }

            ImGui::EndTooltip();
        }

        // Thanh trạng thái dưới đáy
        ImGui::SetNextWindowPos(ImVec2(0.0f, WINDOW_HEIGHT - STATUS_BAR_HEIGHT));
        ImGui::SetNextWindowSize(ImVec2(static_cast<float>(WINDOW_WIDTH), STATUS_BAR_HEIGHT));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14.0f, 5.0f));
        if (ImGui::Begin("##StatusBar", nullptr, toolbarFlags)) {
            std::string isolatedWarn = "";
            const std::unordered_set<int> mainNetStatus = mainNetworkNodes(workingGraph, appState.onlyBuiltLinks);
            for (int nId : workingGraph.nodeIds()) {
                if (mainNetStatus.count(nId) == 0) {
                    std::string nm = workingGraph.getNode(nId).name;
                    if (nm.empty()) nm = "Node " + std::to_string(nId);
                    if (!isolatedWarn.empty()) isolatedWarn += ", ";
                    isolatedWarn += nm;
                }
            }

            if (!isolatedWarn.empty()) {
                ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.0f), "[! CẢNH BÁO] Trạm %s bị cô lập!", isolatedWarn.c_str());
                ImGui::SameLine();
            } else {
                if (appState.statusMessage.find("DROP") != std::string::npos) {
                    ImGui::TextColored(ImVec4(0.85f, 0.18f, 0.18f, 1.0f), "[TRẠNG THÁI - DROP]:");
                } else {
                    ImGui::TextColored(ImVec4(0.15f, 0.55f, 0.25f, 1.0f), "[TRẠNG THÁI]:");
                }
                ImGui::SameLine();
            }
            ImGui::TextUnformatted(appState.statusMessage.c_str());
        }
        ImGui::End();
        ImGui::PopStyleVar();

        // Vẽ đồ thị mạng trên Canvas SFML
        window.clear(sf::Color(248, 250, 252));

        window.draw(gridDots);

        // Vẽ các tuyến cáp
        for (int eId : workingGraph.edgeIds()) {
            const Edge& edge = workingGraph.getEdge(eId);
            if (!nodePositions.count(edge.u) || !nodePositions.count(edge.v)) continue;

            sf::Vector2f p1 = nodePositions.at(edge.u);
            sf::Vector2f p2 = nodePositions.at(edge.v);

            bool inRoutingPath = false;
            if (appState.currentPath.reachable && appState.currentPath.edges.size() >= 1) {
                for (int pathEdgeId : appState.currentPath.edges) {
                    if (pathEdgeId == eId) {
                        inRoutingPath = true;
                        break;
                    }
                }
            }

            bool isBackup = false;
            for (int bId : appState.backupResult.edgeIds) {
                if (bId == eId) {
                    isBackup = true;
                    break;
                }
            }

            // Hào quang cam đánh dấu lộ trình định tuyến (chỉ hiển thị ở Lớp Logic)
            if (appState.viewMode == 1 && inRoutingPath && edge.isUp && appState.currentPath.reachable) {
                sf::Color haloColor(245, 140, 20, 150);
                float haloThickness = 8.5f;
                if (isPairMatch(edge.u, edge.v, curvedDiagU, curvedDiagV)) {
                    drawCurvedCable(window, p1, diagCtrl, p2, haloColor, haloThickness);
                } else {
                    drawThickLine(window, p1, p2, haloColor, haloThickness);
                }
            }

            // Màu lõi cáp theo trạng thái: đỏ (đứt), xanh lá (MST), xanh dương (dự phòng), xám (chưa lắp)
            sf::Color edgeColor(180, 190, 202, 140);
            float thickness = 2.0f;

            bool isEdgeHovered = (eId == hoveredEdgeId && (appState.isCutModeActive || appState.isRestoreModeActive));

            if (!edge.isUp) {
                edgeColor = sf::Color(239, 68, 68, 255); // Cáp bị đứt: Đỏ rực
                thickness = 2.8f;
            } else if (appState.viewMode == 0 && !appState.hasRunMST && !appState.isMSTActive) {
                // Trạng thái khảo sát ban đầu trước khi bấm Chạy MST: hiển thị đồng nhất các tuyến tiềm năng
                edgeColor = sf::Color(100, 116, 139, 190);
                thickness = 2.4f;
            } else if (appState.isMSTActive && appState.sidebarMode == 0) {
                // ĐANG XEM KRUSKAL TỪNG BƯỚC TRONG TAB QUY HOẠCH MST
                bool inMST = false;
                bool isCurrent = false;
                for (int i = 0; i < appState.currentKruskalStep; ++i) {
                    if (appState.kruskalHistory[i].edgeId == eId) {
                        if (appState.kruskalHistory[i].isSelected) inMST = true;
                    }
                }
                if (appState.currentKruskalStep < static_cast<int>(appState.kruskalHistory.size()) &&
                    appState.kruskalHistory[appState.currentKruskalStep].edgeId == eId) {
                    isCurrent = true;
                }

                if (isCurrent) {
                    edgeColor = sf::Color(245, 175, 25, 255);
                    thickness = 3.8f;
                } else if (inMST) {
                    edgeColor = sf::Color(35, 170, 75, 255);  // Cây khung MST: Xanh lá cây đậm
                    thickness = 3.2f;
                } else if (isBackup && appState.mstResult.connected && appState.currentKruskalStep >= static_cast<int>(appState.kruskalHistory.size())) {
                    edgeColor = sf::Color(37, 99, 235, 255);  // Cáp dự phòng: Xanh dương nổi bật (#2563EB)
                    thickness = 2.8f;
                } else {
                    edgeColor = sf::Color(180, 190, 202, 130);
                    thickness = 1.6f;
                }
            } else {
                // TẤT CẢ CÁC CHẾ ĐỘ CÒN LẠI: Lớp Logic, Mô phỏng Kịch Bản, hoặc Sau khi quy hoạch MST
                if (!Validation::isStaticFeasible(edge)) {
                    edgeColor = sf::Color(203, 213, 225, 80);
                    thickness = 1.2f;
                } else if (!edge.isBuilt) {
                    edgeColor = sf::Color(203, 213, 225, 110);
                    thickness = 1.4f;
                } else if (edge.isBackup) {
                    edgeColor = sf::Color(37, 99, 235, 240);  // Cáp dự phòng: Xanh dương (#2563EB)
                    thickness = 2.6f;
                } else {
                    edgeColor = sf::Color(35, 170, 75, 240);  // Cáp cây khung MST: Xanh lá cây (#23AA4B)
                    thickness = 3.0f;
                }
            }

            if (isEdgeHovered) {
                if (appState.isRestoreModeActive) {
                    edgeColor = sf::Color(34, 197, 94, 255); // Xanh lá khi hover chế độ khôi phục
                } else {
                    edgeColor = sf::Color(239, 68, 68, 255); // Đỏ khi hover chế độ cắt
                }
                thickness = 4.5f;
            }

            sf::Color hoverHalo = appState.isRestoreModeActive ? sf::Color(187, 247, 208, 200) : sf::Color(254, 202, 202, 200);

            if (isPairMatch(edge.u, edge.v, curvedDiagU, curvedDiagV)) {
                if (isEdgeHovered) {
                    drawCurvedCable(window, p1, diagCtrl, p2, hoverHalo, thickness + 4.0f);
                }
                drawCurvedCable(window, p1, diagCtrl, p2, edgeColor, thickness);
            } else {
                if (isEdgeHovered) {
                    drawThickLine(window, p1, p2, hoverHalo, thickness + 4.0f);
                }
                drawThickLine(window, p1, p2, edgeColor, thickness);
            }
        }

        // 3. VẼ NHÃN THÔNG SỐ BADGE TRÊN CÁP
        if (hasSfmlFont) {
            for (int eId : workingGraph.edgeIds()) {
                const Edge& edge = workingGraph.getEdge(eId);
                if (!nodePositions.count(edge.u) || !nodePositions.count(edge.v)) continue;

                sf::Vector2f p1 = nodePositions.at(edge.u);
                sf::Vector2f p2 = nodePositions.at(edge.v);
                sf::Vector2f badgePos;

                if (isPairMatch(edge.u, edge.v, curvedDiagU, curvedDiagV)) {
                    badgePos = evalQuadraticBezier(p1, diagCtrl, p2, 0.5f);
                } else {
                    badgePos = (p1 + p2) * 0.5f;
                }

                if (isPairMatch(edge.u, edge.v, straightDiagU, straightDiagV)) {
                    badgePos.x -= 22.0f;
                    badgePos.y += 18.0f;
                }

                bool inRoutingPath = false;
                if (appState.currentPath.reachable) {
                    for (int pathEdgeId : appState.currentPath.edges) {
                        if (pathEdgeId == eId) {
                            inRoutingPath = true;
                            break;
                        }
                    }
                }

                bool isBackup = false;
                for (int bId : appState.backupResult.edgeIds) {
                    if (bId == eId) {
                        isBackup = true;
                        break;
                    }
                }

                std::string badgeText = "";
                sf::Color badgeBg(255, 255, 255, 235);
                sf::Color badgeBorder(203, 213, 225);
                sf::Color badgeTextColor(71, 85, 105);

                if (!edge.isUp) {
                    badgeText = "DOWN";
                    badgeBg = sf::Color(239, 68, 68);
                    badgeBorder = sf::Color(185, 28, 28);
                    badgeTextColor = sf::Color::White;
                } else if (appState.viewMode == 0 && !appState.hasRunMST && !appState.isMSTActive) {
                    badgeText = std::to_string(static_cast<int>(edge.length)) + "m";
                    badgeBg = sf::Color(255, 255, 255, 235);
                    badgeBorder = sf::Color(148, 163, 184);
                    badgeTextColor = sf::Color(51, 65, 85);
                } else if (appState.isMSTActive && appState.sidebarMode == 0 && appState.viewMode == 0) {
                    // CHỈ KHI đang mở Tab Quy Hoạch Cáp (MST) ở Lớp Vật Lý mới hiển thị chi phí kVNĐ
                    badgeText = "";
                    double cost = pricingModel.calculateCablingCost(edge).total;
                    bool inMST = false;
                    for (int i = 0; i < appState.currentKruskalStep; ++i) {
                        if (appState.kruskalHistory[i].edgeId == eId && appState.kruskalHistory[i].isSelected) {
                            inMST = true;
                            break;
                        }
                    }
                    bool isCurrent = (appState.currentKruskalStep < static_cast<int>(appState.kruskalHistory.size()) &&
                                      appState.kruskalHistory[appState.currentKruskalStep].edgeId == eId);

                    if (isCurrent) {
                        badgeText = formatCurrency(cost);
                        badgeBg = sf::Color(254, 249, 195);
                        badgeBorder = sf::Color(234, 179, 8);
                        badgeTextColor = sf::Color(161, 98, 7);
                    } else if (inMST) {
                        badgeText = formatCurrency(cost);
                        badgeBg = sf::Color(236, 253, 245);
                        badgeBorder = sf::Color(34, 197, 94);
                        badgeTextColor = sf::Color(21, 128, 61);
                    } else if (isBackup && appState.mstResult.connected && appState.currentKruskalStep >= static_cast<int>(appState.kruskalHistory.size())) {
                        badgeText = formatCurrency(cost);
                        badgeBg = sf::Color(239, 246, 255);
                        badgeBorder = sf::Color(37, 99, 235);
                        badgeTextColor = sf::Color(29, 78, 216);
                    }
                } else {
                    // Các chế độ khác hiển thị chiều dài (m) kèm tải hoặc độ trễ hàng đợi
                    if (!edge.isBuilt || !Validation::isStaticFeasible(edge)) {
                        badgeText = std::to_string(static_cast<int>(edge.length)) + "m (Chưa lắp)";
                        badgeBg = sf::Color(248, 250, 252, 220);
                        badgeBorder = sf::Color(203, 213, 225);
                        badgeTextColor = sf::Color(148, 163, 184);
                    } else if (edge.currentLoad > 0.01) {
                        badgeText = std::to_string(static_cast<int>(edge.length)) + "m | Tải:" +
                                    std::to_string(static_cast<int>(std::round(edge.currentLoad * 100.0))) + "%";
                        if (edge.currentLoad >= 0.80) {
                            badgeBg = sf::Color(254, 226, 226);
                            badgeBorder = sf::Color(220, 38, 38);
                            badgeTextColor = sf::Color(185, 28, 28);
                        } else {
                            badgeBg = sf::Color(254, 249, 195);
                            badgeBorder = (appState.viewMode == 1 && inRoutingPath) ? sf::Color(245, 158, 11) : sf::Color(202, 138, 4);
                            badgeTextColor = sf::Color(161, 98, 7);
                        }
                    } else if (edge.mediaType == MediaType::FIBER && std::abs(edge.queueDelay - 0.001) > 1e-6) {
                        char qBuf[64];
                        std::snprintf(qBuf, sizeof(qBuf), "%dm | Q:%.2fms",
                                      static_cast<int>(edge.length), edge.queueDelay * 1000.0);
                        badgeText = qBuf;
                        badgeBg = sf::Color(243, 232, 255);
                        badgeBorder = (appState.viewMode == 1 && inRoutingPath) ? sf::Color(245, 158, 11) : sf::Color(147, 51, 234);
                        badgeTextColor = sf::Color(107, 33, 168);
                    } else {
                        badgeText = std::to_string(static_cast<int>(edge.length)) + "m";
                        if (appState.viewMode == 1 && inRoutingPath) {
                            badgeBg = sf::Color(254, 243, 199);
                            badgeBorder = sf::Color(245, 158, 11);
                            badgeTextColor = sf::Color(180, 83, 9);
                        }
                    }
                }

                drawLinkBadge(window, sfmlFont, badgePos, badgeText, badgeBg, badgeBorder, badgeTextColor);
            }
        }

        // 4. VẼ CÁC TRẠM MẠNG (ROUTER / HOST) VÀ CẢNH BÁO CÔ LẬP
        const std::unordered_set<int> mainNetDraw = mainNetworkNodes(workingGraph, appState.onlyBuiltLinks);
        for (const auto& [nId, pos] : nodePositions) {
            bool isHost = (workingGraph.getNode(nId).type == NodeType::HOST);
            bool isSelectedSrc = (nId == appState.selectedSrcNode);
            bool isSelectedDst = (nId == appState.selectedDstNode);
            bool isHovered = (nId == appState.hoveredNodeId);
            bool isolated = (mainNetDraw.count(nId) == 0);

            // NẾU TRẠM BỊ CÔ LẬP KHỎI MẠNG CHÍNH: Vẽ viền đỏ cảnh báo và huy hiệu [CÔ LẬP]
            if (isolated) {
                float haloR = isHost ? 28.0f : (NODE_RADIUS + 8.0f);
                sf::CircleShape isoHalo(haloR);
                isoHalo.setOrigin({haloR, haloR});
                isoHalo.setPosition(pos);
                isoHalo.setFillColor(sf::Color(254, 226, 226, 100));
                isoHalo.setOutlineColor(sf::Color(239, 68, 68));
                isoHalo.setOutlineThickness(2.5f);
                window.draw(isoHalo);

                if (hasSfmlFont) {
                    drawLinkBadge(window, sfmlFont, sf::Vector2f(pos.x, pos.y - (isHost ? 28.0f : NODE_RADIUS + 15.0f)),
                                  "CÔ LẬP", sf::Color(220, 38, 38), sf::Color(153, 27, 27), sf::Color::White);
                }
            } else if (isSelectedSrc) {
                float haloR = isHost ? 26.0f : (NODE_RADIUS + 7.0f);
                sf::CircleShape halo(haloR);
                halo.setOrigin({haloR, haloR});
                halo.setPosition(pos);
                halo.setFillColor(sf::Color(22, 163, 74, 35));
                halo.setOutlineColor(sf::Color(22, 163, 74));
                halo.setOutlineThickness(2.5f);
                window.draw(halo);

                if (hasSfmlFont) {
                    drawLinkBadge(window, sfmlFont, sf::Vector2f(pos.x, pos.y - (isHost ? 28.0f : NODE_RADIUS + 15.0f)),
                                  "SRC", sf::Color(22, 163, 74), sf::Color(20, 83, 45), sf::Color::White);
                }
            } else if (isSelectedDst) {
                float haloR = isHost ? 26.0f : (NODE_RADIUS + 7.0f);
                sf::CircleShape halo(haloR);
                halo.setOrigin({haloR, haloR});
                halo.setPosition(pos);
                halo.setFillColor(sf::Color(234, 88, 12, 35));
                halo.setOutlineColor(sf::Color(234, 88, 12));
                halo.setOutlineThickness(2.5f);
                window.draw(halo);

                if (hasSfmlFont) {
                    drawLinkBadge(window, sfmlFont, sf::Vector2f(pos.x, pos.y - (isHost ? 28.0f : NODE_RADIUS + 15.0f)),
                                  "DST", sf::Color(234, 88, 12), sf::Color(154, 52, 18), sf::Color::White);
                }
            } else if (isHovered) {
                float haloR = isHost ? 26.0f : (NODE_RADIUS + 5.0f);
                sf::CircleShape halo(haloR);
                halo.setOrigin({haloR, haloR});
                halo.setPosition(pos);
                halo.setFillColor(sf::Color(96, 165, 250, 40));
                halo.setOutlineColor(sf::Color(59, 130, 246));
                halo.setOutlineThickness(2.0f);
                window.draw(halo);
            }

            if (isHost) {
                sf::Vector2f screenSize(36.0f, 26.0f);
                sf::RectangleShape screen(screenSize);
                screen.setOrigin({screenSize.x / 2.0f, screenSize.y / 2.0f + 4.0f});
                screen.setPosition(pos);
                screen.setFillColor(isolated ? sf::Color(254, 242, 242) : sf::Color::White);
                screen.setOutlineColor(isolated ? sf::Color(239, 68, 68) : sf::Color(71, 85, 105));
                screen.setOutlineThickness(2.0f);
                window.draw(screen);

                sf::Vector2f innerSize(28.0f, 18.0f);
                sf::RectangleShape innerScreen(innerSize);
                innerScreen.setOrigin({innerSize.x / 2.0f, innerSize.y / 2.0f + 4.0f});
                innerScreen.setPosition(pos);
                innerScreen.setFillColor(isolated ? sf::Color(254, 202, 202) : sf::Color(226, 232, 240));
                window.draw(innerScreen);

                sf::RectangleShape stand(sf::Vector2f(4.0f, 6.0f));
                stand.setOrigin({2.0f, 0.0f});
                stand.setPosition(sf::Vector2f(pos.x, pos.y + 9.0f));
                stand.setFillColor(isolated ? sf::Color(239, 68, 68) : sf::Color(100, 116, 139));
                window.draw(stand);

                sf::RectangleShape basePlate(sf::Vector2f(16.0f, 3.5f));
                basePlate.setOrigin({8.0f, 0.0f});
                basePlate.setPosition(sf::Vector2f(pos.x, pos.y + 15.0f));
                basePlate.setFillColor(isolated ? sf::Color(239, 68, 68) : sf::Color(100, 116, 139));
                window.draw(basePlate);

                if (hasSfmlFont) {
                    sf::Text hostText(sfmlFont);
                    std::string label = workingGraph.getNode(nId).name.empty() ? ("H" + std::to_string(nId)) : workingGraph.getNode(nId).name;
                    hostText.setString(label);
                    hostText.setCharacterSize(12);
                    hostText.setFillColor(isolated ? sf::Color(185, 28, 28) : sf::Color(30, 41, 59));
                    hostText.setStyle(sf::Text::Bold);
                    sf::FloatRect bounds = hostText.getLocalBounds();
                    hostText.setOrigin({bounds.position.x + bounds.size.x / 2.0f, bounds.position.y + bounds.size.y / 2.0f + 4.0f});
                    hostText.setPosition(pos);
                    window.draw(hostText);
                }
            } else {
                sf::CircleShape routerCircle(NODE_RADIUS);
                routerCircle.setOrigin({NODE_RADIUS, NODE_RADIUS});
                routerCircle.setPosition(pos);
                routerCircle.setFillColor(isolated ? sf::Color(254, 242, 242) : sf::Color::White);
                routerCircle.setOutlineColor(isolated ? sf::Color(239, 68, 68) : sf::Color(37, 99, 235));
                routerCircle.setOutlineThickness(2.5f);
                window.draw(routerCircle);

                float arrowLen = 13.0f;
                sf::Color arrowColor = isolated ? sf::Color(239, 68, 68) : sf::Color(29, 78, 216);
                
                sf::Vertex hLine[2] = {
                    sf::Vertex{sf::Vector2f(pos.x - arrowLen, pos.y), arrowColor, {0.0f, 0.0f}},
                    sf::Vertex{sf::Vector2f(pos.x + arrowLen, pos.y), arrowColor, {0.0f, 0.0f}}
                };
                sf::Vertex vLine[2] = {
                    sf::Vertex{sf::Vector2f(pos.x, pos.y - arrowLen), arrowColor, {0.0f, 0.0f}},
                    sf::Vertex{sf::Vector2f(pos.x, pos.y + arrowLen), arrowColor, {0.0f, 0.0f}}
                };
                window.draw(hLine, 2, sf::PrimitiveType::Lines);
                window.draw(vLine, 2, sf::PrimitiveType::Lines);

                sf::Vertex arrLeft[4] = {
                    sf::Vertex{sf::Vector2f(pos.x - arrowLen, pos.y), arrowColor, {0.0f, 0.0f}},
                    sf::Vertex{sf::Vector2f(pos.x - arrowLen + 4.0f, pos.y - 3.0f), arrowColor, {0.0f, 0.0f}},
                    sf::Vertex{sf::Vector2f(pos.x - arrowLen, pos.y), arrowColor, {0.0f, 0.0f}},
                    sf::Vertex{sf::Vector2f(pos.x - arrowLen + 4.0f, pos.y + 3.0f), arrowColor, {0.0f, 0.0f}}
                };
                window.draw(arrLeft, 4, sf::PrimitiveType::Lines);

                sf::Vertex arrRight[4] = {
                    sf::Vertex{sf::Vector2f(pos.x + 3.0f, pos.y), arrowColor, {0.0f, 0.0f}},
                    sf::Vertex{sf::Vector2f(pos.x + 7.0f, pos.y - 3.0f), arrowColor, {0.0f, 0.0f}},
                    sf::Vertex{sf::Vector2f(pos.x + 3.0f, pos.y), arrowColor, {0.0f, 0.0f}},
                    sf::Vertex{sf::Vector2f(pos.x + 7.0f, pos.y + 3.0f), arrowColor, {0.0f, 0.0f}}
                };
                window.draw(arrRight, 4, sf::PrimitiveType::Lines);

                sf::Vertex arrTop[4] = {
                    sf::Vertex{sf::Vector2f(pos.x, pos.y - arrowLen), arrowColor, {0.0f, 0.0f}},
                    sf::Vertex{sf::Vector2f(pos.x - 3.0f, pos.y - arrowLen + 4.0f), arrowColor, {0.0f, 0.0f}},
                    sf::Vertex{sf::Vector2f(pos.x, pos.y - arrowLen), arrowColor, {0.0f, 0.0f}},
                    sf::Vertex{sf::Vector2f(pos.x + 3.0f, pos.y - arrowLen + 4.0f), arrowColor, {0.0f, 0.0f}}
                };
                window.draw(arrTop, 4, sf::PrimitiveType::Lines);

                sf::Vertex arrBottom[4] = {
                    sf::Vertex{sf::Vector2f(pos.x, pos.y + 3.0f), arrowColor, {0.0f, 0.0f}},
                    sf::Vertex{sf::Vector2f(pos.x - 3.0f, pos.y + 7.0f), arrowColor, {0.0f, 0.0f}},
                    sf::Vertex{sf::Vector2f(pos.x, pos.y + 3.0f), arrowColor, {0.0f, 0.0f}},
                    sf::Vertex{sf::Vector2f(pos.x + 3.0f, pos.y + 7.0f), arrowColor, {0.0f, 0.0f}}
                };
                window.draw(arrBottom, 4, sf::PrimitiveType::Lines);

                if (hasSfmlFont) {
                    sf::Text routerText(sfmlFont);
                    std::string label = workingGraph.getNode(nId).name.empty() ? ("R" + std::to_string(nId)) : workingGraph.getNode(nId).name;
                    routerText.setString(label);
                    routerText.setCharacterSize(13);
                    routerText.setFillColor(isolated ? sf::Color(185, 28, 28) : sf::Color(25, 40, 65));
                    routerText.setStyle(sf::Text::Bold);
                    sf::FloatRect bounds = routerText.getLocalBounds();
                    routerText.setOrigin({bounds.position.x + bounds.size.x / 2.0f, bounds.position.y});
                    routerText.setPosition(sf::Vector2f(pos.x, pos.y + NODE_RADIUS + 4.0f));
                    window.draw(routerText);
                }
            }
        }

        // 5. HIỆU ỨNG SÓNG LAN TỎA (RIPPLE)
        for (const auto& rip : appState.ripples) {
            if (nodePositions.count(rip.nodeId)) {
                sf::Vector2f rPos = nodePositions.at(rip.nodeId);
                float radius = NODE_RADIUS + rip.timer * 35.0f;
                int alpha = static_cast<int>(240.0f * (1.0f - rip.timer));
                sf::CircleShape ripShape(radius);
                ripShape.setOrigin({radius, radius});
                ripShape.setPosition(rPos);
                ripShape.setFillColor(sf::Color::Transparent);
                ripShape.setOutlineColor(sf::Color(22, 163, 74, alpha));
                ripShape.setOutlineThickness(2.5f * (1.0f - rip.timer));
                window.draw(ripShape);
            }
        }

        // 6. GÓI TIN HOẠT HỌA DI CHUYỂN
        for (const auto& pkt : appState.activePackets) {
            if (pkt.isDelivered || pkt.isBlocked || pkt.currentHopIndex >= static_cast<int>(pkt.path.size()) - 1) continue;
            int u = pkt.path[pkt.currentHopIndex];
            int v = pkt.path[pkt.currentHopIndex + 1];
            if (nodePositions.count(u) && nodePositions.count(v)) {
                sf::Vector2f a = nodePositions.at(u);
                sf::Vector2f b = nodePositions.at(v);
                sf::Vector2f pktPos;

                if (isPairMatch(u, v, curvedDiagU, curvedDiagV)) {
                    pktPos = evalQuadraticBezier(a, diagCtrl, b, pkt.progress);
                } else {
                    pktPos = sf::Vector2f(
                        (1.0f - pkt.progress) * a.x + pkt.progress * b.x,
                        (1.0f - pkt.progress) * a.y + pkt.progress * b.y
                    );
                }

                sf::CircleShape glow(12.0f);
                glow.setOrigin({12.0f, 12.0f});
                glow.setPosition(pktPos);
                glow.setFillColor(sf::Color(255, 180, 0, 80));
                window.draw(glow);

                sf::CircleShape pktShape(8.0f);
                pktShape.setOrigin({8.0f, 8.0f});
                pktShape.setPosition(pktPos);
                pktShape.setFillColor(sf::Color(255, 120, 0));
                pktShape.setOutlineColor(sf::Color::White);
                pktShape.setOutlineThickness(2.0f);
                window.draw(pktShape);
            }
        }

        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}
