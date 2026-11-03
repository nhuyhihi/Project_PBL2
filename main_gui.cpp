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
#include "Reliability.h"
#include "Reporter.h"
#include "Simulator.h"
#include "Scenario.h"
#include "Logger.h"
#include "Validation.h"
#include "EnumUtils.h"

// ============================================================================
// CẤU HÌNH BỐ CỤC MÀN HÌNH TỔNG THỂ & VÙNG AN TOÀN (1280 x 720)
// ============================================================================
constexpr unsigned int WINDOW_WIDTH   = 1280;
constexpr unsigned int WINDOW_HEIGHT  = 720;
constexpr float TOP_BAR_HEIGHT        = 42.0f;
constexpr float STATUS_BAR_HEIGHT     = 28.0f;

// Thông số Vùng an toàn (Safe Zone) & Bố cục mạng mở rộng
constexpr float SAFE_CENTER_X         = 500.0f;
constexpr float SAFE_CENTER_Y         = 375.0f;
constexpr float ROUTER_RADIUS         = 210.0f;
constexpr float HOST_DISTANCE         = 120.0f;
constexpr float NODE_RADIUS           = 22.0f;
constexpr float CUT_HITBOX_TOLERANCE  = 8.0f;

// Thông số Sidebar Resizable Drawer
constexpr float DRAWER_MIN_WIDTH      = 300.0f;
constexpr float DRAWER_MAX_WIDTH      = 430.0f;
constexpr float DRAWER_DEFAULT_WIDTH  = 395.0f;
constexpr float DRAWER_HEIGHT         = 650.0f;

// ============================================================================
// CẤU TRÚC DỮ LIỆU ĐIỀU KHIỂN GIAO DIỆN
// ============================================================================
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
    float timer = 0.0f; // 0.0f -> 1.0f
};

struct AppState {
    bool isMSTActive = false;
    bool isScenarioActive = false;
    bool isCutModeActive = false;

    bool isSidebarOpen = false;
    int  sidebarMode = 0; // 0: MST Step-by-Step, 1: Scenario Step-by-Step
    bool switchTabRequested = false;
    int  requestedTab = 0;
    int  hoveredNodeId = -1;
    int  selectedSrcNode = 0;
    int  selectedDstNode = -1;

    int  viewMode = 0; // 0: Vật lý (MST/Backup), 1: Logic (Routing)
    int  selectedPacketPreset = 0; // 0: Chuẩn 1500B, 1: Pkt#0 (500B RT), 2: Pkt#1 (3000B TCP Frag), 3: Test DF-MTU (3000B DF)
    int  selectedPacketType = 0;   // 0: REAL_TIME, 1: BULK_DATA, 2: CONTROL
    bool onlyBuiltLinks = true;    // Định tuyến trên hạ tầng đã xây (MST + Backup)

    MSTResult mstResult;
    PathResult currentPath;
    BackupResult backupResult;

    // Theo dõi đổi đường (Dynamic Reroute Detection)
    int lastRouteSrc = -1;
    int lastRouteDst = -1;
    std::vector<int> lastRouteNodes;
    bool isRerouted = false;
    std::string previousRouteStr = "";

    // Máy trạng thái Kruskal
    std::vector<KruskalStep> kruskalHistory;
    int currentKruskalStep = 0;

    // Quản lý kịch bản
    int activeScenarioIdx = 0;
    std::vector<std::string> scenarioFiles = {
        "basic.txt", "qos.txt", "congestion.txt", "mtu.txt", "failure.txt", "redundancy.txt"
    };
    std::vector<std::string> scenarioNames = {
        "1. Cơ Bản (Basic)", "2. Phân Luồng QoS", "3. Tắc Nghẽn (Congestion)",
        "4. Phân Mảnh MTU", "5. Đứt Cáp & Phục Hồi (Failure)", "6. Đóng Góp Dự Phòng (Redundancy)"
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

    std::string statusMessage = "Hệ thống sẵn sàng. Chọn Nguồn/Đích trên thanh điều khiển hoặc click trực tiếp trên Canvas.";

    void reset(int defaultNodeId = 0) {
        isMSTActive = false;
        isScenarioActive = false;
        isCutModeActive = false;
        isSidebarOpen = false;
        sidebarMode = 0;
        hoveredNodeId = -1;
        selectedSrcNode = defaultNodeId;
        selectedDstNode = -1;
        viewMode = 0;
        selectedPacketPreset = 0;
        selectedPacketType = 0;
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
        previousRouteStr.clear();
        activePackets.clear();
        ripples.clear();
        hasRoutingInfo = false;
        statusMessage = "Đã khôi phục mạng về trạng thái sạch ban đầu.";
    }
};

// Tạo cấu hình Packet đồng nhất cho cả 3 vị trí gọi Dijkstra (Pill, Hover Tooltip, Reroute)
Packet buildCurrentPacket(const AppState& appState, int srcNode, int dstNode) {
    Packet pkt;
    pkt.id = 999;
    pkt.source = srcNode;
    pkt.destination = dstNode;

    if (appState.selectedPacketPreset == 1) {
        // Pkt #0 từ data/packets.txt: 500B, UDP, DF=Chặn
        pkt.id = 0;
        pkt.sizeBytes = 500;
        pkt.protocol = Protocol::UDP;
        pkt.canFragment = false;
        pkt.newConnection = false;
    } else if (appState.selectedPacketPreset == 2) {
        // Pkt #1 từ data/packets.txt: 3000B, TCP, Cho phân mảnh (3 mảnh), Kết nối mới (+RTT)
        pkt.id = 1;
        pkt.sizeBytes = 3000;
        pkt.protocol = Protocol::TCP;
        pkt.canFragment = true;
        pkt.newConnection = true;
    } else if (appState.selectedPacketPreset == 3) {
        // Gói tin kiểm thử chặn DF-MTU: 3000B > MTU 1500B nhưng canFragment = false -> DROP
        pkt.id = 2;
        pkt.sizeBytes = 3000;
        pkt.protocol = Protocol::TCP;
        pkt.canFragment = false;
        pkt.newConnection = true;
    } else {
        // Gói chuẩn 1500B, UDP, Cho phân mảnh
        pkt.sizeBytes = 1500;
        pkt.protocol = Protocol::UDP;
        pkt.canFragment = true;
        pkt.newConnection = false;
    }

    if (appState.selectedPacketType == 0) pkt.type = PacketType::REAL_TIME;
    else if (appState.selectedPacketType == 1) pkt.type = PacketType::BULK_DATA;
    else pkt.type = PacketType::CONTROL;

    return pkt;
}

// Tìm tập hợp các trạm còn liên thông được với trạm lõi đầu tiên trong mạng
std::unordered_set<int> getConnectedComponentFromCore(const Graph& g) {
    std::unordered_set<int> visited;
    std::vector<int> ids = g.nodeIds();
    if (ids.empty()) return visited;
    int rootId = ids.front();

    std::vector<int> q = {rootId};
    visited.insert(rootId);
    size_t head = 0;
    while (head < q.size()) {
        int u = q[head++];
        for (int eId : g.getNeighbors(u)) {
            const auto& e = g.getEdge(eId);
            if (e.isUp) {
                int v = g.otherEndpoint(u, eId);
                if (v != -1 && !visited.count(v)) {
                    visited.insert(v);
                    q.push_back(v);
                }
            }
        }
    }
    return visited;
}

// Kiểm tra trạm có bị cô lập hoàn toàn hoặc bị chia cắt khỏi mạng lõi không
bool isNodeIsolated(const Graph& g, int nId) {
    bool hasActiveLink = false;
    for (int eId : g.getNeighbors(nId)) {
        if (g.getEdge(eId).isUp) {
            hasActiveLink = true;
            break;
        }
    }
    if (!hasActiveLink) return true;

    std::vector<int> ids = g.nodeIds();
    if (!ids.empty() && nId == ids.front()) return false;
    auto reachable = getConnectedComponentFromCore(g);
    return !reachable.count(nId);
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

// ============================================================================
// HÌNH HỌC & ĐƯỜNG CONG TÁCH CÁP CHÉO (BEZIER ARCS & HITBOX)
// ============================================================================
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

bool isCurvedDiagonal(int u, int v) {
    return (u == 1 && v == 3) || (u == 3 && v == 1);
}

sf::Vector2f getDiagonalCurveControl(const sf::Vector2f& posR2, const sf::Vector2f& posR4) {
    sf::Vector2f mid = (posR2 + posR4) * 0.5f;
    float dx = posR4.x - posR2.x;
    float dy = posR4.y - posR2.y;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < 1.0f) return mid;
    sf::Vector2f normal(-dy / len, dx / len);
    return mid + normal * 55.0f;
}

sf::Vector2f evalQuadraticBezier(const sf::Vector2f& p0, const sf::Vector2f& p1, const sf::Vector2f& p2, float t) {
    float u = 1.0f - t;
    return u * u * p0 + 2.0f * u * t * p1 + t * t * p2;
}

float getDistanceToEdge(const sf::Vector2f& clickPos, int u, int v, 
                        const sf::Vector2f& p1, const sf::Vector2f& p2, const sf::Vector2f& ctrl) {
    if (isCurvedDiagonal(u, v)) {
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

// ============================================================================
// HÀM TIỆN ÍCH VẼ VECTOR ĐỒ HỌA SFML
// ============================================================================
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

// ============================================================================
// HÀM TIỆN ÍCH TOP TOOLBAR: UNIFIED STATE PATTERN
// ============================================================================
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

// ============================================================================
// CLEAN LIGHT THEME
// ============================================================================
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

// ============================================================================
// HÀM KHỞI TẠO LỊCH SỬ DUYỆT CẠNH KRUSKAL & CHỌN CÁP DỰ PHÒNG BACKUP
// (Tái sử dụng 100% buildMST & selectBackupLinks của Backend, tuân thủ DRY)
// ============================================================================
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
                               ", Chi phí: " + std::to_string(static_cast<int>(step.weight)) + " ĐV";
        } else {
            step.isSelected = false;
            step.description = "Loại bỏ: Nối " + uName + " - " + vName + 
                               " (" + toString(rejectMap[edgeId]) + ")";
        }
        appState.kruskalHistory.push_back(step);
    }
}

// ============================================================================
// CẬP NHẬT ĐỊNH TUYẾN DIJKSTRA VỚI GÓI TIN HIỆN TẠI (KHÔNG GÂY SIDE-EFFECT)
// ============================================================================
void updateRoutingPath(const Graph& g, AppState& appState) {
    if (appState.selectedSrcNode == -1 || appState.selectedDstNode == -1 || appState.selectedSrcNode == appState.selectedDstNode) {
        appState.currentPath = PathResult();
        appState.hasRoutingInfo = false;
        appState.isRerouted = false;
        return;
    }

    Packet qosPkt = buildCurrentPacket(appState, appState.selectedSrcNode, appState.selectedDstNode);

    // Chạy Dijkstra với onlyBuiltLinks (mặc định true: chỉ đi trên cáp đã lắp đặt MST + Backup)
    appState.currentPath = dijkstra(g, appState.selectedSrcNode, appState.selectedDstNode, qosPkt, appState.onlyBuiltLinks);

    // Dùng bản sao simCopy để tính độ trễ mà KHÔNG làm biến đổi ngầm currentLoad của workingGraph
    Graph simCopy = g;
    appState.logicSimResult = Simulator::sendPacket(qosPkt, appState.currentPath, simCopy);
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
            ss << " [ĐÃ ĐỔI ĐƯỜNG (REROUTE) từ " << appState.previousRouteStr << "]";
        }
        appState.statusMessage = ss.str();
    } else {
        if (qosPkt.sizeBytes > 1500 && !qosPkt.canFragment) {
            appState.statusMessage = "DROP [DF_BLOCKED_BY_MTU]: Gói tin " + std::to_string(qosPkt.sizeBytes) +
                                     "B vượt quá MTU (1500B) nhưng bật cờ DF (Không cho phân mảnh)!";
        } else {
            appState.statusMessage = "DROP [UNREACHABLE]: Không còn đường đi hợp lệ từ " + srcName + " đến " + dstName + "!";
        }
    }
}

void applyScenarioToStep(Graph& workingGraph, const Graph& baseGraph, const CostModel& pricingModel,
                         const Scenario& scenario, int targetStep, AppState& appState) {
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
    // Cập nhật lịch sử Kruskal (nhưng giữ nguyên hạ tầng isBuilt của baseGraph để mô phỏng đúng chuẩn)
    recordKruskalSteps(workingGraph, pricingModel, appState, /*updateGraphBuildFlags=*/false);
    if (appState.isMSTActive) {
        appState.currentKruskalStep = static_cast<int>(appState.kruskalHistory.size());
    }
    // Tự động cập nhật ngay lập tức lộ trình và độ trễ ở Lớp Logic
    updateRoutingPath(workingGraph, appState);
}

// ============================================================================
// CHƯƠNG TRÌNH CHÍNH
// ============================================================================
int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    // ------------------------------------------------------------------------
    // 1. NẠP DỮ LIỆU BACKEND & QUY HOẠCH HẠ TẦNG CƠ SỞ (MST + BACKUP)
    // ------------------------------------------------------------------------
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

    // Thực hiện Bước 3 & Bước 4 trong Kiến trúc 11 bước trên baseGraph
    MSTResult initialMST = buildMST(baseGraph, costFn);
    selectBackupLinks(baseGraph, initialMST, DEFAULT_BACKUP_LINKS, costFn);

    Graph workingGraph = baseGraph;

    int defaultNodeId = workingGraph.nodeIds().empty() ? 0 : workingGraph.nodeIds().front();
    AppState appState;
    appState.reset(defaultNodeId);

    recordKruskalSteps(workingGraph, pricingModel, appState, /*updateGraphBuildFlags=*/true);

    IOManager::loadScenario("data/scenarios/" + appState.scenarioFiles[0], appState.loadedScenario);
    appState.maxScenarioSteps = static_cast<int>(appState.loadedScenario.events.size());

    std::vector<int> allNodeIds = workingGraph.nodeIds();
    std::vector<std::string> nodeLabels;
    for (int id : allNodeIds) {
        std::string n = workingGraph.getNode(id).name;
        nodeLabels.push_back(n.empty() ? ("Node " + std::to_string(id)) : n);
    }

    const std::vector<std::string> packetPresetLabels = {
        "Chuẩn 1500B (UDP, Cho phân mảnh)",
        "Pkt #0 [packets.txt]: 500B | REAL_TIME | UDP | DF=Chặn",
        "Pkt #1 [packets.txt]: 3000B | BULK_DATA | TCP (3 mảnh + RTT)",
        "Test Chặn MTU: 3000B | CONTROL | TCP | DF=Chặn (Rớt gói)"
    };

    // ------------------------------------------------------------------------
    // 2. KHỞI TẠO CỬA SỔ SFML & IMGUI-SFML
    // ------------------------------------------------------------------------
    sf::ContextSettings settings;
    settings.antiAliasingLevel = 8;

    std::string winTitle = "PBL2 - Thiết Kế & Mô Phỏng Mạng Viễn Thông (SFML 3 + Dear ImGui)";
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

    // ------------------------------------------------------------------------
    // LƯỚI CHẤM KỸ THUẬT (GRID DOTS)
    // ------------------------------------------------------------------------
    sf::VertexArray gridDots(sf::PrimitiveType::Points);
    constexpr float GRID_STEP = 28.0f;
    for (float gx = 20.0f; gx < WINDOW_WIDTH - 20.0f; gx += GRID_STEP) {
        for (float gy = TOP_BAR_HEIGHT + 14.0f; gy < WINDOW_HEIGHT - STATUS_BAR_HEIGHT - 10.0f; gy += GRID_STEP) {
            gridDots.append(sf::Vertex{sf::Vector2f(gx, gy), sf::Color(210, 222, 235, 170), {0.0f, 0.0f}});
        }
    }

    // ------------------------------------------------------------------------
    // BỐ TRÍ TỌA ĐỘ MẠNG 2 TẦNG
    // ------------------------------------------------------------------------
    std::unordered_map<int, sf::Vector2f> nodePositions;
    auto recomputeLayout = [&]() {
        nodePositions.clear();
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
                sf::Vector2f rPos = nodePositions[connectedRouter];
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

    sf::Vector2f r2r4Ctrl = getDiagonalCurveControl(nodePositions[1], nodePositions[3]);

    // ------------------------------------------------------------------------
    // 3. GAME LOOP TUẦN TỰ (60 FPS)
    // ------------------------------------------------------------------------
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

                    // 1. Chế độ Cắt Cáp
                    if (appState.isCutModeActive && mousePress->button == sf::Mouse::Button::Left) {
                        int cutEdgeId = -1;
                        float minDist = CUT_HITBOX_TOLERANCE + 1.0f;
                        for (int eId : workingGraph.edgeIds()) {
                            const Edge& e = workingGraph.getEdge(eId);
                            if (nodePositions.count(e.u) && nodePositions.count(e.v)) {
                                float d = getDistanceToEdge(clickPos, e.u, e.v, nodePositions[e.u], nodePositions[e.v], r2r4Ctrl);
                                if (d <= CUT_HITBOX_TOLERANCE && d < minDist) {
                                    minDist = d;
                                    cutEdgeId = eId;
                                }
                            }
                        }

                        if (cutEdgeId != -1) {
                            Edge& edge = workingGraph.getEdge(cutEdgeId);
                            edge.isUp = !edge.isUp; // Đảo trạng thái đứt / nối lại
                            
                            std::string uName = workingGraph.getNode(edge.u).name.empty() ? std::to_string(edge.u) : workingGraph.getNode(edge.u).name;
                            std::string vName = workingGraph.getNode(edge.v).name.empty() ? std::to_string(edge.v) : workingGraph.getNode(edge.v).name;

                            // Tự động tính toán lại Kruskal & Backup theo trạng thái cáp vừa cắt
                            recordKruskalSteps(workingGraph, pricingModel, appState, /*updateGraphBuildFlags=*/true);
                            if (appState.isMSTActive) {
                                appState.currentKruskalStep = static_cast<int>(appState.kruskalHistory.size());
                            }
                            updateRoutingPath(workingGraph, appState);

                            appState.statusMessage = (edge.isUp ? "Đã phục hồi liên kết #" : "Đã cắt đứt liên kết #") +
                                                     std::to_string(cutEdgeId) + " (" + uName + " - " + vName + ").";
                        } else {
                            appState.statusMessage = "Không bấm trúng cạnh nào. Đã thoát chế độ Cắt Cáp.";
                        }
                        appState.isCutModeActive = false;
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
                                    updateRoutingPath(workingGraph, appState);
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
                                        updateRoutingPath(workingGraph, appState);

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
                        // Đồng bộ hàm trọng số QoS và cờ onlyBuiltLinks khi gói tin đang bay phải Reroute
                        Packet reroutePkt = buildCurrentPacket(appState, u, pkt.destNode);
                        PathResult newPath = dijkstra(workingGraph, u, pkt.destNode, reroutePkt, appState.onlyBuiltLinks);
                        if (newPath.reachable) {
                            pkt.path = newPath.nodes;
                            pkt.currentHopIndex = 0;
                            pkt.progress = 0.0f;
                            std::string uName = workingGraph.getNode(u).name.empty() ? std::to_string(u) : workingGraph.getNode(u).name;
                            appState.statusMessage = "Cáp đứt phía trước! Tự động chuyển hướng (Reroute) từ trạm " + uName;
                        } else {
                            pkt.isBlocked = true;
                            appState.statusMessage = "DROP: Tuyến phía trước bị đứt và không có đường thay thế (Unreachable)!";
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

            if (appState.isCutModeActive) {
                float minCutDist = CUT_HITBOX_TOLERANCE;
                for (int eId : workingGraph.edgeIds()) {
                    const Edge& e = workingGraph.getEdge(eId);
                    if (nodePositions.count(e.u) && nodePositions.count(e.v)) {
                        float d = getDistanceToEdge(mousePos, e.u, e.v, nodePositions[e.u], nodePositions[e.v], r2r4Ctrl);
                        if (d < minCutDist) {
                            minCutDist = d;
                            hoveredEdgeId = eId;
                        }
                    }
                }
            }
        }

        // ====================================================================
        // MODULE 1: TOP TOOLBAR (Y: 0 -> 42px)
        // ====================================================================
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

            ImGui::RadioButton("Lớp Vật Lý (MST)", &appState.viewMode, 0);
            ImGui::SameLine(0, 10.0f);
            ImGui::RadioButton("Lớp Logic (Routing)", &appState.viewMode, 1);

            ImGui::SameLine(0, 16.0f);
            ImGui::TextColored(ImVec4(0.75f, 0.78f, 0.82f, 1.0f), "|");
            ImGui::SameLine(0, 16.0f);

            // 1. Nút Chạy MST
            if (DrawToggleButton("Chạy MST", appState.isMSTActive, ImVec4(0.12f, 0.46f, 0.85f, 1.0f))) {
                appState.isMSTActive = !appState.isMSTActive;
                if (appState.isMSTActive) {
                    appState.isScenarioActive = false;
                    appState.isSidebarOpen = true;
                    appState.sidebarMode = 0;
                    appState.switchTabRequested = true;
                    appState.requestedTab = 0;
                    recordKruskalSteps(workingGraph, pricingModel, appState, /*updateGraphBuildFlags=*/true);
                    appState.currentKruskalStep = static_cast<int>(appState.kruskalHistory.size());
                    appState.statusMessage = "Chế độ MST: Đã tính toán cây khung (Xanh lá) và cáp dự phòng (Xanh dương).";
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
                    appState.isSidebarOpen = true;
                    appState.sidebarMode = 1;
                    appState.switchTabRequested = true;
                    appState.requestedTab = 1;
                    applyScenarioToStep(workingGraph, baseGraph, pricingModel, appState.loadedScenario, appState.currentScenarioStep, appState);
                    appState.statusMessage = "Chế độ Kịch Bản: Đang kích hoạt " + appState.scenarioNames[appState.activeScenarioIdx];
                }
            }

            if (appState.isScenarioActive) {
                ImGui::SameLine(0, 8.0f);
                if (ImGui::Button("<")) {
                    if (appState.currentScenarioStep > 0) {
                        appState.currentScenarioStep--;
                        applyScenarioToStep(workingGraph, baseGraph, pricingModel, appState.loadedScenario, appState.currentScenarioStep, appState);
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
                        applyScenarioToStep(workingGraph, baseGraph, pricingModel, appState.loadedScenario, appState.currentScenarioStep, appState);
                    }
                }
            }
            ImGui::SameLine(0, 8.0f);

            // 3. Nút Cắt Cáp
            const char* cutLabel = appState.isCutModeActive ? "Đang Chờ Click Cáp..." : "Cắt Cáp";
            if (DrawToggleButton(cutLabel, appState.isCutModeActive, ImVec4(0.86f, 0.22f, 0.22f, 1.0f))) {
                appState.isCutModeActive = !appState.isCutModeActive;
                if (appState.isCutModeActive) {
                    appState.statusMessage = "Chế độ Cắt Cáp: Rê chuột và click trực tiếp vào đoạn cáp trên Canvas để ngắt/nối lại.";
                } else {
                    appState.statusMessage = "Đã hủy chế độ Cắt Cáp.";
                }
            }
            ImGui::SameLine(0, 8.0f);

            // 4. Nút Khôi Phục
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.16f, 0.65f, 0.30f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f, 0.74f, 0.38f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.12f, 0.54f, 0.24f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
            if (ImGui::Button(" Khôi Phục ")) {
                workingGraph = baseGraph;
                appState.reset(defaultNodeId);
                recordKruskalSteps(workingGraph, pricingModel, appState, /*updateGraphBuildFlags=*/true);
            }
            ImGui::PopStyleColor(4);

            // Nhóm bên phải: Bảng số liệu & Thoát
            ImGui::SameLine(WINDOW_WIDTH - 235.0f);
            std::string drawerBtnLabel = appState.isSidebarOpen ? " Ẩn Bảng [X] " : " Bảng Số Liệu ";
            if (ImGui::Button(drawerBtnLabel.c_str())) {
                appState.isSidebarOpen = !appState.isSidebarOpen;
            }
            ImGui::SameLine(0, 8.0f);
            if (ImGui::Button(" Thoát ")) {
                window.close();
            }
        }
        ImGui::End();
        ImGui::PopStyleVar();

        // ====================================================================
        // THẺ NỔI ĐỊNH TUYẾN DẠNG PILL (CHẾ ĐỘ LỚP LOGIC - ROUTING CARD)
        // ====================================================================
        if (appState.viewMode == 1) {
            ImGui::SetNextWindowPos(ImVec2(16.0f, TOP_BAR_HEIGHT + 6.0f), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(855.0f, 66.0f), ImGuiCond_Always);
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
                    updateRoutingPath(workingGraph, appState);
                }

                ImGui::SameLine(0, 6.0f);
                ImGui::TextColored(ImVec4(0.85f, 0.35f, 0.1f, 1.0f), "-> Đích:");
                ImGui::SameLine(0, 3.0f);
                ImGui::SetNextItemWidth(65.0f);
                int curDstIdx = -1;
                for (int i = 0; i < static_cast<int>(allNodeIds.size()); ++i) {
                    if (allNodeIds[i] == appState.selectedDstNode) curDstIdx = i;
                }
                const char* dstPreview = (curDstIdx >= 0) ? nodeLabels[curDstIdx].c_str() : "(Chọn)";
                if (ImGui::BeginCombo("##DstCombo", dstPreview)) {
                    for (int i = 0; i < static_cast<int>(allNodeIds.size()); ++i) {
                        if (allNodeIds[i] == appState.selectedSrcNode) continue;
                        bool isSel = (curDstIdx == i);
                        if (ImGui::Selectable(nodeLabels[i].c_str(), isSel)) {
                            appState.selectedDstNode = allNodeIds[i];
                            updateRoutingPath(workingGraph, appState);
                        }
                    }
                    ImGui::EndCombo();
                }

                ImGui::SameLine(0, 8.0f);
                ImGui::Text("Gói:");
                ImGui::SameLine(0, 3.0f);
                ImGui::SetNextItemWidth(195.0f);
                if (ImGui::BeginCombo("##PktPresetCombo", packetPresetLabels[appState.selectedPacketPreset].c_str())) {
                    for (int i = 0; i < static_cast<int>(packetPresetLabels.size()); ++i) {
                        bool isSel = (appState.selectedPacketPreset == i);
                        if (ImGui::Selectable(packetPresetLabels[i].c_str(), isSel)) {
                            appState.selectedPacketPreset = i;
                            // Tự động đồng bộ loại QoS tương ứng với gói tin trong packets.txt
                            if (i == 1) appState.selectedPacketType = 0;      // REAL_TIME
                            else if (i == 2) appState.selectedPacketType = 1; // BULK_DATA
                            else if (i == 3) appState.selectedPacketType = 2; // CONTROL
                            updateRoutingPath(workingGraph, appState);
                        }
                    }
                    ImGui::EndCombo();
                }

                ImGui::SameLine(0, 8.0f);
                ImGui::Text("QoS:");
                ImGui::SameLine(0, 3.0f);

                int prevQoS = appState.selectedPacketType;
                ImGui::RadioButton("REAL_TIME", &appState.selectedPacketType, 0);
                ImGui::SameLine(0, 4.0f);
                ImGui::RadioButton("BULK", &appState.selectedPacketType, 1);
                ImGui::SameLine(0, 4.0f);
                ImGui::RadioButton("CONTROL", &appState.selectedPacketType, 2);

                if (prevQoS != appState.selectedPacketType) {
                    updateRoutingPath(workingGraph, appState);
                }

                ImGui::SameLine(0, 8.0f);
                if (appState.currentPath.reachable && appState.currentPath.nodes.size() >= 2) {
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
                Packet activePkt = buildCurrentPacket(appState, appState.selectedSrcNode, appState.selectedDstNode);

                if (appState.selectedDstNode == -1) {
                    ImGui::TextColored(ImVec4(0.45f, 0.50f, 0.55f, 1.0f),
                        "Chọn trạm Đích ở trên hoặc click chuột trái vào trạm trên sơ đồ (Chuột phải: Đổi trạm Nguồn).");
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
                    } else {
                        ImGui::SameLine(0, 8.0f);
                        if (appState.selectedPacketType == 0) {
                            ImGui::TextColored(ImVec4(0.85f, 0.45f, 0.1f, 1.0f), "[REAL_TIME: Né trễ hàng đợi wQ=4]");
                        } else if (appState.selectedPacketType == 1) {
                            ImGui::TextColored(ImVec4(0.15f, 0.45f, 0.85f, 1.0f), "[BULK: Ưu tiên cáp 10Gbps wT=2]");
                        } else {
                            ImGui::TextColored(ImVec4(0.4f, 0.45f, 0.5f, 1.0f), "[CONTROL: Trọng số cân bằng 1:1:1]");
                        }
                    }
                } else {
                    if (activePkt.sizeBytes > 1500 && !activePkt.canFragment) {
                        ImGui::TextColored(ImVec4(0.85f, 0.18f, 0.18f, 1.0f),
                            "[DROP - DF_BLOCKED_BY_MTU] Gói %dB vượt MTU (1500B) nhưng bật cờ DF (Cấm phân mảnh)!",
                            activePkt.sizeBytes);
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

        // ====================================================================
        // MODULE 3: SIDEBAR RESIZABLE DRAWER
        // ====================================================================
        if (appState.isSidebarOpen) {
            ImGui::SetNextWindowPos(ImVec2(WINDOW_WIDTH - DRAWER_DEFAULT_WIDTH, TOP_BAR_HEIGHT), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSizeConstraints(ImVec2(DRAWER_MIN_WIDTH, DRAWER_HEIGHT), ImVec2(DRAWER_MAX_WIDTH, DRAWER_HEIGHT));

            ImGuiWindowFlags drawerFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(1.0f, 1.0f, 1.0f, 0.98f));
            if (ImGui::Begin("Bảng Dữ Liệu##Sidebar", &appState.isSidebarOpen, drawerFlags)) {
                if (ImGui::BeginTabBar("SidebarTabs")) {
                    // TAB 1: KRUSKAL STEP-BY-STEP, NGÂN SÁCH (TASK B2.4) & PHÂN TÍCH DỰ PHÒNG (RELIABILITY B5.2)
                    ImGuiTabItemFlags tab1Flags = 0;
                    if (appState.switchTabRequested && appState.requestedTab == 0) {
                        tab1Flags |= ImGuiTabItemFlags_SetSelected;
                    }
                    if (ImGui::BeginTabItem(" Quy Hoạch Cáp (MST) ", nullptr, tab1Flags)) {
                        appState.sidebarMode = 0;
                        appState.isMSTActive = true;
                        appState.isScenarioActive = false;

                        // 1. BẢNG SO SÁNH 3 MỨC NGÂN SÁCH (TASK B2.4)
                        double costAll = 0.0;
                        for (int eId : baseGraph.edgeIds()) {
                            costAll += costFn(baseGraph.getEdge(eId));
                        }

                        double costMST = appState.mstResult.totalCost;
                        double costBackup = appState.backupResult.totalCost;
                        double costMSTBackup = costMST + costBackup;
                        double savedPct = (costAll > 0.0 && !std::isinf(costAll) && !std::isinf(costMST)) ? ((costAll - costMST) / costAll * 100.0) : 0.0;

                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.12f, 0.46f, 0.85f, 1.0f), "SO SÁNH 3 MỨC NGÂN SÁCH (TASK B2.4):");
                        if (ImGui::BeginTable("BudgetCompare", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                            ImGui::TableSetupColumn("Phương Án");
                            ImGui::TableSetupColumn("Chi Phí");
                            ImGui::TableSetupColumn("Bảo Vệ / Lợi Ích");
                            ImGui::TableHeadersRow();

                            // 1. Toàn Bộ
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("1. Toàn Bộ");
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%.0f ĐV", costAll);
                            ImGui::TableSetColumnIndex(2);
                            ImGui::TextColored(ImVec4(0.85f, 0.25f, 0.25f, 1.0f), "Tốn kém (%d cáp)", static_cast<int>(baseGraph.edgeIds().size()));

                            // 2. Cây Khung (MST)
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::TextColored(ImVec4(0.12f, 0.62f, 0.28f, 1.0f), "2. Cây Khung (MST)");
                            ImGui::TableSetColumnIndex(1);
                            if (std::isinf(costMST) || !appState.mstResult.connected) {
                                ImGui::TextColored(ImVec4(0.85f, 0.2f, 0.2f, 1.0f), "Không khả thi");
                            } else {
                                ImGui::TextColored(ImVec4(0.12f, 0.62f, 0.28f, 1.0f), "%.0f ĐV", costMST);
                            }
                            ImGui::TableSetColumnIndex(2);
                            if (appState.mstResult.connected) {
                                ImGui::TextColored(ImVec4(0.12f, 0.62f, 0.28f, 1.0f), "-%.1f%% (%d cáp)", savedPct, static_cast<int>(appState.mstResult.edgeIds.size()));
                            } else {
                                ImGui::TextColored(ImVec4(0.85f, 0.2f, 0.2f, 1.0f), "Bị chia cắt (%d cáp)", static_cast<int>(appState.mstResult.edgeIds.size()));
                            }

                            // 3. MST + Dự Phòng (Cùng mất đi nếu MST không liên thông)
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::TextColored(ImVec4(0.15f, 0.45f, 0.95f, 1.0f), "3. MST + Dự Phòng");
                            ImGui::TableSetColumnIndex(1);
                            if (std::isinf(costMST) || !appState.mstResult.connected) {
                                ImGui::TextColored(ImVec4(0.85f, 0.2f, 0.2f, 1.0f), "Không khả thi");
                            } else {
                                ImGui::TextColored(ImVec4(0.15f, 0.45f, 0.95f, 1.0f), "%.0f ĐV", costMSTBackup);
                            }
                            ImGui::TableSetColumnIndex(2);
                            if (appState.mstResult.connected) {
                                ImGui::TextColored(ImVec4(0.15f, 0.45f, 0.95f, 1.0f), "%d/%d cạnh (+%d bk)", appState.backupResult.protectedTreeEdges, appState.backupResult.treeEdges, static_cast<int>(appState.backupResult.edgeIds.size()));
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

                        // 2. PHÂN TÍCH ĐỘ TIN CẬY & CÁP DỰ PHÒNG (MODULE RELIABILITY - TASK B5.2)
                        ImGui::Spacing();
                        if (ImGui::CollapsingHeader("Phân Tích Cáp Dự Phòng (Task B5.2)", ImGuiTreeNodeFlags_DefaultOpen)) {
                            if (ImGui::BeginTable("ReliabilityTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                                ImGui::TableSetupColumn("Cạnh MST", ImGuiTableColumnFlags_WidthFixed, 105.0f);
                                ImGui::TableSetupColumn("Phương Án Cứu Hộ Khi Đứt");
                                ImGui::TableHeadersRow();

                                for (int tEdgeId : appState.mstResult.edgeIds) {
                                    const Edge& te = workingGraph.getEdge(tEdgeId);
                                    std::string uN = workingGraph.getNode(te.u).name;
                                    std::string vN = workingGraph.getNode(te.v).name;

                                    BackupCandidate cand = Reliability::findBackupForTreeEdge(
                                        workingGraph, appState.mstResult.edgeIds, tEdgeId, costFn
                                    );

                                    ImGui::TableNextRow();
                                    ImGui::TableSetColumnIndex(0);
                                    ImGui::Text("#%d (%s-%s)", tEdgeId, uN.c_str(), vN.c_str());

                                    ImGui::TableSetColumnIndex(1);
                                    if (cand.backupEdgeId != -1) {
                                        const Edge& be = workingGraph.getEdge(cand.backupEdgeId);
                                        std::string bu = workingGraph.getNode(be.u).name;
                                        std::string bv = workingGraph.getNode(be.v).name;
                                        if (be.isBackup) {
                                            ImGui::TextColored(ImVec4(0.15f, 0.45f, 0.95f, 1.0f),
                                                "Backup #%d (%s-%s, %.0f ĐV)", cand.backupEdgeId, bu.c_str(), bv.c_str(), cand.cost);
                                        } else {
                                            ImGui::TextColored(ImVec4(0.45f, 0.50f, 0.55f, 1.0f),
                                                "Ứng viên #%d (%s-%s, %.0f ĐV)", cand.backupEdgeId, bu.c_str(), bv.c_str(), cand.cost);
                                        }
                                    } else {
                                        ImGui::TextColored(ImVec4(0.85f, 0.25f, 0.25f, 1.0f), "Cạnh cầu (Critical Bridge)");
                                    }
                                }
                                ImGui::EndTable();
                            }
                        }

                        // 3. THUẬT TOÁN KRUSKAL TỪNG BƯỚC
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
                        if (ImGui::BeginChild("KruskalLogList", ImVec2(0, 175), true)) {
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
                                ImGui::TextColored(ImVec4(0.9f, 0.65f, 0.1f, 1.0f), ">> Đang xét: %s - %s (Chi phí: %.0f ĐV)", uName.c_str(), vName.c_str(), nextSt.weight);
                            }
                        }
                        ImGui::EndChild();

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
                                    // Khi chọn kịch bản, tự động đặt cặp Nguồn R1 (0) -> Đích R4 (3) để thấy ngay kết quả
                                    if (appState.selectedDstNode == -1 && workingGraph.hasNode(0) && workingGraph.hasNode(3)) {
                                        appState.selectedSrcNode = 0;
                                        appState.selectedDstNode = 3;
                                    }
                                    applyScenarioToStep(workingGraph, baseGraph, pricingModel, appState.loadedScenario, 0, appState);
                                    appState.statusMessage = "Đã nạp kịch bản: " + appState.scenarioNames[i];
                                }
                            }
                            ImGui::EndCombo();
                        }

                        ImGui::Separator();
                        ImGui::Text("Tiến trình: Bước %d / %d", appState.maxScenarioSteps == 0 ? 0 : appState.currentScenarioStep + 1, appState.maxScenarioSteps);
                        if (ImGui::Button(" [<< Lùi bước] ") && appState.currentScenarioStep > 0) {
                            appState.currentScenarioStep--;
                            applyScenarioToStep(workingGraph, baseGraph, pricingModel, appState.loadedScenario, appState.currentScenarioStep, appState);
                        }
                        ImGui::SameLine();
                        if (ImGui::Button(" [Tiến bước >>] ") && appState.currentScenarioStep < appState.maxScenarioSteps - 1) {
                            appState.currentScenarioStep++;
                            applyScenarioToStep(workingGraph, baseGraph, pricingModel, appState.loadedScenario, appState.currentScenarioStep, appState);
                        }
                        ImGui::SameLine();
                        if (ImGui::Button(" [Chạy từ đầu] ")) {
                            appState.currentScenarioStep = 0;
                            applyScenarioToStep(workingGraph, baseGraph, pricingModel, appState.loadedScenario, appState.currentScenarioStep, appState);
                        }

                        ImGui::Separator();
                        ImGui::TextColored(ImVec4(0.2f, 0.4f, 0.7f, 1.0f), "Danh sách sự kiện (Click để chuyển bước):");
                        if (ImGui::BeginChild("ScenarioEvents", ImVec2(0, 175), true)) {
                            if (appState.loadedScenario.events.empty()) {
                                ImGui::TextColored(ImVec4(0.45f, 0.50f, 0.55f, 1.0f),
                                    "Kịch bản tĩnh (Kiểm tra phân mảnh MTU 1500B trực tiếp trên gói tin 500B & 3000B bên dưới).");
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
                                    applyScenarioToStep(workingGraph, baseGraph, pricingModel, appState.loadedScenario, appState.currentScenarioStep, appState);
                                }
                                if (isCurrent) {
                                    ImGui::PopStyleColor();
                                }
                            }
                        }
                        ImGui::EndChild();

                        // BẢNG KẾT QUẢ TRUYỀN CÁC GÓI TIN TRONG PACKETS.TXT TẠI BƯỚC HIỆN TẠI
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.12f, 0.46f, 0.85f, 1.0f), "Kết quả truyền gói tin (data/packets.txt):");
                        if (ImGui::BeginTable("ScenarioPacketsTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                            ImGui::TableSetupColumn("Gói Tin", ImGuiTableColumnFlags_WidthFixed, 92.0f);
                            ImGui::TableSetupColumn("Lộ Trình");
                            ImGui::TableSetupColumn("Mảnh", ImGuiTableColumnFlags_WidthFixed, 42.0f);
                            ImGui::TableSetupColumn("Trễ / TT", ImGuiTableColumnFlags_WidthFixed, 78.0f);
                            ImGui::TableHeadersRow();

                            Graph snapG = workingGraph;
                            for (const Packet& pkt : packets) {
                                PathResult pRes = dijkstra(snapG, pkt.source, pkt.destination, pkt, /*onlyBuilt=*/true);
                                SimulationResult sRes = Simulator::sendPacket(pkt, pRes, snapG);

                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                ImGui::Text("#%d (%dB %s)", pkt.id, pkt.sizeBytes,
                                            pkt.type == PacketType::REAL_TIME ? "RT" : "BULK");

                                ImGui::TableSetColumnIndex(1);
                                if (pRes.reachable) {
                                    ImGui::TextColored(ImVec4(0.12f, 0.62f, 0.28f, 1.0f), "%s",
                                                       formatNodePath(workingGraph, pRes.nodes).c_str());
                                } else {
                                    ImGui::TextColored(ImVec4(0.85f, 0.2f, 0.2f, 1.0f), "UNREACHABLE");
                                }

                                ImGui::TableSetColumnIndex(2);
                                ImGui::Text("%d", pRes.reachable ? sRes.fragmentCount : 0);

                                ImGui::TableSetColumnIndex(3);
                                if (pRes.reachable && sRes.success) {
                                    ImGui::Text("%.2f ms", sRes.totalDelay * 1000.0);
                                } else {
                                    ImGui::TextColored(ImVec4(0.85f, 0.2f, 0.2f, 1.0f), "DROP");
                                }
                            }
                            ImGui::EndTable();
                        }

                        // NÚT XUẤT FILE LOG (.TXT) SỬ DỤNG MODULE LOGGER (TASK B6.5)
                        ImGui::Spacing();
                        if (ImGui::Button(" [Xuất Nhật Ký Kịch Bản Ra File Log (.txt)] ", ImVec2(-1.0f, 0.0f))) {
                            std::filesystem::create_directories("data/logs");
                            std::string logFile = "data/logs/log_" + appState.scenarioFiles[appState.activeScenarioIdx];
                            Graph logGraph = baseGraph;
                            Simulator::runScenarioSimulation(logGraph, packets, appState.loadedScenario, logFile);
                            appState.statusMessage = "Đã xuất nhật ký mô phỏng thành công ra file: " + logFile;
                        }

                        ImGui::EndTabItem();
                    }

                    ImGui::EndTabBar();
                }
                appState.switchTabRequested = false;
            }
            ImGui::End();
            ImGui::PopStyleColor();
        }

        // ====================================================================
        // MODULE 2: NATIVE HOVER TOOLTIP (ĐỒNG BỘ 100% HÀM TRỌNG SỐ QOS & ONLYBUILT)
        // ====================================================================
        if (appState.hoveredNodeId != -1 && !ImGui::GetIO().WantCaptureMouse) {
            std::string hName = workingGraph.getNode(appState.hoveredNodeId).name;
            if (hName.empty()) hName = "Node " + std::to_string(appState.hoveredNodeId);

            const char* qosModeStr = (appState.selectedPacketType == 0) ? "REAL_TIME" :
                                     (appState.selectedPacketType == 1) ? "BULK_DATA" : "CONTROL";

            ImGui::BeginTooltip();
            ImGui::TextColored(ImVec4(0.12f, 0.46f, 0.85f, 1.0f), "BẢNG ĐỊNH TUYẾN - %s (QoS: %s)", hName.c_str(), qosModeStr);
            ImGui::Separator();
            if (ImGui::BeginTable("RoutingHover", 4, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Đích");
                ImGui::TableSetupColumn("Next-Hop");
                ImGui::TableSetupColumn("Lộ Trình");
                ImGui::TableSetupColumn("Trễ (ms)");
                ImGui::TableHeadersRow();

                for (int destId : workingGraph.nodeIds()) {
                    if (destId == appState.hoveredNodeId) continue;
                    Packet hoverPkt = buildCurrentPacket(appState, appState.hoveredNodeId, destId);
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
                        Graph hoverSimG = workingGraph;
                        SimulationResult hSim = Simulator::sendPacket(hoverPkt, res, hoverSimG);
                        ImGui::Text("%.3f ms", hSim.totalDelay * 1000.0);
                    } else {
                        ImGui::TextColored(ImVec4(0.85f, 0.2f, 0.2f, 1.0f), "INF");
                    }
                }
                ImGui::EndTable();
            }

            ImGui::EndTooltip();
        }

        // ====================================================================
        // STATUS BAR (Y: 692 -> 720px)
        // ====================================================================
        ImGui::SetNextWindowPos(ImVec2(0.0f, WINDOW_HEIGHT - STATUS_BAR_HEIGHT));
        ImGui::SetNextWindowSize(ImVec2(static_cast<float>(WINDOW_WIDTH), STATUS_BAR_HEIGHT));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14.0f, 5.0f));
        if (ImGui::Begin("##StatusBar", nullptr, toolbarFlags)) {
            std::string isolatedWarn = "";
            for (int nId : workingGraph.nodeIds()) {
                if (isNodeIsolated(workingGraph, nId)) {
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
                ImGui::TextColored(ImVec4(0.15f, 0.55f, 0.25f, 1.0f), "[TRẠNG THÁI]:");
                ImGui::SameLine();
            }
            ImGui::TextUnformatted(appState.statusMessage.c_str());
        }
        ImGui::End();
        ImGui::PopStyleVar();

        // ====================================================================
        // VẼ ĐỒ THỊ MẠNG TRÊN CANVAS SFML
        // ====================================================================
        window.clear(sf::Color(248, 250, 252));

        // 1. LƯỚI CHẤM KỸ THUẬT
        window.draw(gridDots);

        // 2. VẼ TOÀN BỘ CÁP QUANG (EDGES)
        for (int eId : workingGraph.edgeIds()) {
            const Edge& edge = workingGraph.getEdge(eId);
            if (!nodePositions.count(edge.u) || !nodePositions.count(edge.v)) continue;

            sf::Vector2f p1 = nodePositions[edge.u];
            sf::Vector2f p2 = nodePositions[edge.v];

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

            sf::Color edgeColor(180, 190, 202, 140);
            float thickness = 1.8f;

            bool isEdgeHovered = (eId == hoveredEdgeId && appState.isCutModeActive);

            if (!edge.isUp) {
                edgeColor = sf::Color(239, 68, 68, 255); // Cáp bị đứt: Đỏ rực
                thickness = 2.5f;
            } else if (inRoutingPath) {
                edgeColor = sf::Color(245, 140, 20, 255); // Đường định tuyến: Cam đậm nổi bật
                thickness = 3.8f;
            } else if (appState.isMSTActive) {
                // CHỈ KHI BẬT NÚT "CHẠY MST" MỚI HIỆN MÀU XANH LÁ (MST) VÀ XANH DƯƠNG (BACKUP)
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
                } else if (isBackup && appState.mstResult.connected) {
                    edgeColor = sf::Color(37, 99, 235, 255);  // Cáp dự phòng: XANH DƯƠNG NỔI BẬT (#2563EB)
                    thickness = 2.8f;
                } else {
                    edgeColor = sf::Color(180, 190, 202, 130);
                    thickness = 1.6f;
                }
            } else {
                // SƠ ĐỒ BAN ĐẦU HOẶC CHẾ ĐỘ LỚP LOGIC
                if (appState.viewMode == 1 && !edge.isBuilt) {
                    // Cáp không được lắp đặt (Edge #5 quá đắt, Edge #8 quá dài >100m): vẽ mờ nhẹ
                    edgeColor = sf::Color(203, 213, 225, 95);
                    thickness = 1.3f;
                } else if (appState.viewMode == 1 && edge.currentLoad >= 0.80) {
                    // Cáp đang bị tắc nghẽn cao trong kịch bản: tô màu vàng cam cảnh báo
                    edgeColor = sf::Color(234, 88, 12, 210);
                    thickness = 2.6f;
                } else {
                    edgeColor = sf::Color(180, 190, 202, 165);
                    thickness = 2.0f;
                }
            }

            if (isEdgeHovered) {
                edgeColor = sf::Color(239, 68, 68, 255);
                thickness = 5.0f;
            }

            if (isCurvedDiagonal(edge.u, edge.v)) {
                if (isEdgeHovered) {
                    drawCurvedCable(window, p1, r2r4Ctrl, p2, sf::Color(254, 202, 202, 200), thickness + 4.0f);
                }
                drawCurvedCable(window, p1, r2r4Ctrl, p2, edgeColor, thickness);
            } else {
                if (isEdgeHovered) {
                    drawThickLine(window, p1, p2, sf::Color(254, 202, 202, 200), thickness + 4.0f);
                }
                drawThickLine(window, p1, p2, edgeColor, thickness);
            }
        }

        // 3. VẼ NHÃN THÔNG SỐ BADGE TRÊN CÁP
        if (hasSfmlFont) {
            for (int eId : workingGraph.edgeIds()) {
                const Edge& edge = workingGraph.getEdge(eId);
                if (!nodePositions.count(edge.u) || !nodePositions.count(edge.v)) continue;

                sf::Vector2f p1 = nodePositions[edge.u];
                sf::Vector2f p2 = nodePositions[edge.v];
                sf::Vector2f badgePos;

                if (isCurvedDiagonal(edge.u, edge.v)) {
                    badgePos = evalQuadraticBezier(p1, r2r4Ctrl, p2, 0.5f);
                } else {
                    badgePos = (p1 + p2) * 0.5f;
                }

                if ((edge.u == 0 && edge.v == 2) || (edge.u == 2 && edge.v == 0)) {
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
                } else if (appState.viewMode == 0) { // Lớp Vật Lý (MST)
                    double cost = pricingModel.calculateCablingCost(edge).total;
                    badgeText = std::to_string(static_cast<int>(cost)) + " ĐV";
                    
                    if (appState.isMSTActive) {
                        bool inMST = false;
                        for (int i = 0; i < appState.currentKruskalStep; ++i) {
                            if (appState.kruskalHistory[i].edgeId == eId && appState.kruskalHistory[i].isSelected) {
                                inMST = true;
                                break;
                            }
                        }
                        if (inMST) {
                            badgeBg = sf::Color(236, 253, 245);
                            badgeBorder = sf::Color(34, 197, 94);
                            badgeTextColor = sf::Color(21, 128, 61);
                        } else if (isBackup && appState.mstResult.connected) {
                            badgeBg = sf::Color(239, 246, 255);
                            badgeBorder = sf::Color(37, 99, 235);
                            badgeTextColor = sf::Color(29, 78, 216);
                        }
                    }
                } else { // Lớp Logic (Routing) - Hiển thị mét + băng thông + tải / trễ hàng đợi động
                    if (!Validation::isStaticFeasible(edge)) {
                        badgeText = std::to_string(static_cast<int>(edge.length)) + "m (>100m)";
                        badgeBg = sf::Color(248, 250, 252, 220);
                        badgeBorder = sf::Color(203, 213, 225);
                        badgeTextColor = sf::Color(148, 163, 184);
                    } else if (!edge.isBuilt && appState.onlyBuiltLinks) {
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
                            badgeBorder = sf::Color(202, 138, 4);
                            badgeTextColor = sf::Color(161, 98, 7);
                        }
                    } else if (edge.mediaType == MediaType::FIBER && std::abs(edge.queueDelay - 0.001) > 1e-6) {
                        char qBuf[64];
                        std::snprintf(qBuf, sizeof(qBuf), "%dm | Q:%.2fms",
                                      static_cast<int>(edge.length), edge.queueDelay * 1000.0);
                        badgeText = qBuf;
                        badgeBg = sf::Color(243, 232, 255);
                        badgeBorder = sf::Color(147, 51, 234);
                        badgeTextColor = sf::Color(107, 33, 168);
                    } else {
                        if (edge.bandwidthMbps >= 10000.0) {
                            badgeText = std::to_string(static_cast<int>(edge.length)) + "m (10G)";
                        } else if (edge.bandwidthMbps >= 1000.0) {
                            badgeText = std::to_string(static_cast<int>(edge.length)) + "m (1G)";
                        } else {
                            badgeText = std::to_string(static_cast<int>(edge.length)) + "m";
                        }
                        if (inRoutingPath) {
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
        for (const auto& [nId, pos] : nodePositions) {
            bool isHost = (workingGraph.getNode(nId).type == NodeType::HOST);
            bool isSelectedSrc = (nId == appState.selectedSrcNode);
            bool isSelectedDst = (nId == appState.selectedDstNode);
            bool isHovered = (nId == appState.hoveredNodeId);
            bool isolated = isNodeIsolated(workingGraph, nId);

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
                sf::Vector2f rPos = nodePositions[rip.nodeId];
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
                sf::Vector2f a = nodePositions[u];
                sf::Vector2f b = nodePositions[v];
                sf::Vector2f pktPos;

                if (isCurvedDiagonal(u, v)) {
                    pktPos = evalQuadraticBezier(a, r2r4Ctrl, b, pkt.progress);
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
