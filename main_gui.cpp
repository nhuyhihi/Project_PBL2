#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <unordered_map>
#include <iomanip>
#include <sstream>

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
#include "Dijkstra.h"
#include "PathResult.h"
#include "Reporter.h"
#include "Simulator.h"
#include "Scenario.h"

// ============================================================================
// CẤU HÌNH BỐ CỤC MÀN HÌNH TỔNG THỂ & VÙNG AN TOÀN (1280 x 720)
// ============================================================================
constexpr unsigned int WINDOW_WIDTH   = 1280;
constexpr unsigned int WINDOW_HEIGHT  = 720;
constexpr float TOP_BAR_HEIGHT        = 42.0f;
constexpr float STATUS_BAR_HEIGHT     = 28.0f;

// Thông số Vùng an toàn (Safe Zone) & Bố cục mạng mở rộng
constexpr float SAFE_CENTER_X         = 500.0f;
constexpr float SAFE_CENTER_Y         = 370.0f;
constexpr float ROUTER_RADIUS         = 210.0f; // Tăng từ 160 -> 210px để lấp đầy Canvas
constexpr float HOST_DISTANCE         = 120.0f; // Tăng từ 105 -> 120px đẩy Hosts ra góc cao
constexpr float NODE_RADIUS           = 22.0f;
constexpr float CUT_HITBOX_TOLERANCE  = 8.0f;

// Thông số Sidebar Resizable Drawer
constexpr float DRAWER_MIN_WIDTH      = 280.0f;
constexpr float DRAWER_MAX_WIDTH      = 420.0f;
constexpr float DRAWER_DEFAULT_WIDTH  = 380.0f;
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
    int  selectedPacketType = 0; // 0: REAL_TIME, 1: BULK_DATA, 2: CONTROL

    MSTResult mstResult;
    PathResult currentPath;
    BackupResult backupResult;
    bool hasBuiltMST = false;

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

    std::string statusMessage = "Hệ thống sẵn sàng. Chuột trái: Chọn đích | Chuột phải: Đổi nguồn | Rê chuột: Bảng định tuyến.";

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
        selectedPacketType = 0;
        currentKruskalStep = 0;
        currentScenarioStep = 0;
        mstResult = MSTResult();
        currentPath = PathResult();
        backupResult = BackupResult();
        hasBuiltMST = false;
        activePackets.clear();
        ripples.clear();
        statusMessage = "Đã khôi phục mạng về trạng thái sạch ban đầu.";
    }
};

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

// Kiểm tra cạnh có phải là đường chéo R2-R4 cần uốn cong né tâm không
bool isCurvedDiagonal(int u, int v) {
    return (u == 1 && v == 3) || (u == 3 && v == 1);
}

// Tính điểm điều khiển cố định cho đường cong R2 - R4 (uốn cong lệch tâm 55px)
sf::Vector2f getDiagonalCurveControl(const sf::Vector2f& posR2, const sf::Vector2f& posR4) {
    sf::Vector2f mid = (posR2 + posR4) * 0.5f;
    float dx = posR4.x - posR2.x;
    float dy = posR4.y - posR2.y;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < 1.0f) return mid;
    sf::Vector2f normal(-dy / len, dx / len);
    return mid + normal * 55.0f; // Lệch về phía trên-phải để né tâm (500, 370)
}

// Điểm trên đường cong Bezier bậc 2 tại tham số t in [0, 1]
sf::Vector2f evalQuadraticBezier(const sf::Vector2f& p0, const sf::Vector2f& p1, const sf::Vector2f& p2, float t) {
    float u = 1.0f - t;
    return u * u * p0 + 2.0f * u * t * p1 + t * t * p2;
}

// Khoảng cách từ điểm tới cạnh (hỗ trợ cả đường thẳng và đường cong)
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
// HÀM TIỆN ÍCH VẼ VECTOR ĐỒ HỌA SFML (CÁP DÀY, CONG, BADGE CHỈ SỐ)
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
// CLEAN LIGHT THEME (GIAO DIỆN SÁNG, HIỆN ĐẠI, THUẦN CHẤT DESKTOP APP)
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

// Áp dụng trạng thái kịch bản đến một bước chỉ định
void applyScenarioToStep(Graph& workingGraph, const Graph& baseGraph, const Scenario& scenario, int targetStep, AppState& appState) {
    workingGraph = baseGraph; // Khôi phục về baseGraph
    if (targetStep < 0 || targetStep >= static_cast<int>(scenario.events.size())) return;
    for (int i = 0; i <= targetStep; ++i) {
        const auto& ev = scenario.events[i];
        if (workingGraph.hasEdge(ev.targetId)) {
            Simulator::applyEvent(workingGraph, ev);
        }
    }
}

// ============================================================================
// HÀM KHỞI TẠO LỊCH SỬ DUYỆT CẠNH KRUSKAL (MODULE 4: RECORD & REPLAY)
// ============================================================================
void recordKruskalSteps(Graph& g, const CostModel& pricingModel, AppState& appState) {
    appState.kruskalHistory.clear();
    auto costFn = [&pricingModel](const Edge& e) { return pricingModel.calculateCablingCost(e).total; };

    // Bản sao mô phỏng
    Graph simGraph = g;
    simGraph.resetBuildState();
    DSU dsu(simGraph.maxNodeId() + 1);

    std::vector<int> order = simGraph.edgeIds();
    std::sort(order.begin(), order.end(), [&](int x, int y) {
        return compareEdges(simGraph.getEdge(x), simGraph.getEdge(y), costFn);
    });

    for (int edgeId : order) {
        const Edge& e = simGraph.getEdge(edgeId);
        RejectReason reason = checkEdge(simGraph, e, dsu);

        KruskalStep step;
        step.edgeId = edgeId;
        step.u = e.u;
        step.v = e.v;
        step.weight = costFn(e);

        std::string uName = simGraph.getNode(e.u).name.empty() ? std::to_string(e.u) : simGraph.getNode(e.u).name;
        std::string vName = simGraph.getNode(e.v).name.empty() ? std::to_string(e.v) : simGraph.getNode(e.v).name;

        if (reason == RejectReason::NONE) {
            dsu.unite(e.u, e.v);
            simGraph.getNode(e.u).usedPorts++;
            simGraph.getNode(e.v).usedPorts++;
            step.isSelected = true;
            step.description = "Chấp nhận: Nối " + uName + " - " + vName + 
                               ", Chi phí: " + std::to_string(static_cast<int>(step.weight)) + " ĐV";
        } else {
            step.isSelected = false;
            step.description = "Loại bỏ: Nối " + uName + " - " + vName + 
                               " (" + toString(reason) + ")";
        }
        appState.kruskalHistory.push_back(step);
    }
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
    // 1. NẠP DỮ LIỆU BACKEND CỐT LÕI
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

    Graph workingGraph = baseGraph;

    int defaultNodeId = workingGraph.nodeIds().empty() ? 0 : workingGraph.nodeIds().front();
    AppState appState;
    appState.reset(defaultNodeId);

    // Ghi sẵn lịch sử duyệt cạnh Kruskal (Record)
    recordKruskalSteps(workingGraph, pricingModel, appState);

    // Nạp sẵn kịch bản ban đầu
    IOManager::loadScenario("data/scenarios/" + appState.scenarioFiles[0], appState.loadedScenario);
    appState.maxScenarioSteps = static_cast<int>(appState.loadedScenario.events.size());

    // ------------------------------------------------------------------------
    // 2. KHỞI TẠO CỬA SỔ SFML 1280x720 & IMGUI-SFML
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

    // Font SFML cho nhãn Router và Badge trên Canvas
    sf::Font sfmlFont;
    bool hasSfmlFont = sfmlFont.openFromFile("C:/Windows/Fonts/segoeui.ttf") || sfmlFont.openFromFile("C:/Windows/Fonts/arial.ttf");

    sf::Clock deltaClock;

    // ------------------------------------------------------------------------
    // LƯỚI CHẤM KỸ THUẬT (GRID DOTS CISCO PACKET TRACER)
    // ------------------------------------------------------------------------
    sf::VertexArray gridDots(sf::PrimitiveType::Points);
    constexpr float GRID_STEP = 28.0f;
    for (float gx = 20.0f; gx < WINDOW_WIDTH - 20.0f; gx += GRID_STEP) {
        for (float gy = TOP_BAR_HEIGHT + 14.0f; gy < WINDOW_HEIGHT - STATUS_BAR_HEIGHT - 10.0f; gy += GRID_STEP) {
            gridDots.append(sf::Vertex{sf::Vector2f(gx, gy), sf::Color(210, 222, 235, 170), {0.0f, 0.0f}});
        }
    }

    // ------------------------------------------------------------------------
    // BỐ TRÍ TỌA ĐỘ MẠNG 2 TẦNG: CORE ROUTERS Ở GIỮA & EDGE HOSTS Ở NGOÀI
    // Tận dụng hoàn hảo không gian Canvas 880px (R=210px, HostDist=120px)
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

        // 1. Tầng lõi (Core Routers): Vòng tròn bán kính 210px quanh tâm (500, 370)
        int numRouters = static_cast<int>(routerIds.size());
        for (int i = 0; i < numRouters; ++i) {
            float angle = i * (2.0f * 3.14159265f / numRouters) - 3.14159265f / 4.0f;
            nodePositions[routerIds[i]] = sf::Vector2f(
                SAFE_CENTER_X + ROUTER_RADIUS * std::cos(angle),
                SAFE_CENTER_Y + ROUTER_RADIUS * std::sin(angle)
            );
        }

        // 2. Tầng biên (Edge Hosts): Đẩy ra ngoài rìa theo hướng từ tâm qua Router
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

    // Điểm điều khiển cố định cho đoạn chéo cong R2-R4
    sf::Vector2f r2r4Ctrl = getDiagonalCurveControl(nodePositions[1], nodePositions[3]);

    // ------------------------------------------------------------------------
    // 3. SINGLE-THREADED GAME LOOP TUẦN TỰ (60 FPS)
    // ------------------------------------------------------------------------
    while (window.isOpen()) {
        sf::Vector2i mousePixel = sf::Mouse::getPosition(window);
        sf::Vector2f mousePos(static_cast<float>(mousePixel.x), static_cast<float>(mousePixel.y));

        // BƯỚC 3.1: Nhận và xử lý sự kiện tuần tự
        while (const std::optional event = window.pollEvent()) {
            ImGui::SFML::ProcessEvent(window, *event);

            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            // Click chuột trên Canvas (Bảo vệ bởi !WantCaptureMouse)
            if (const auto* mousePress = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (!ImGui::GetIO().WantCaptureMouse) {
                    sf::Vector2f clickPos(static_cast<float>(mousePress->position.x), 
                                          static_cast<float>(mousePress->position.y));

                    // 1. Chế độ Cắt Cáp (Module 1 & 5)
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

                            appState.statusMessage = (edge.isUp ? "Đã phục hồi liên kết #" : "Đã cắt đứt liên kết #") +
                                                     std::to_string(cutEdgeId) + " (" + uName + " - " + vName + ").";
                            
                            // Nếu bảng MST đang mở, tự động cập nhật lại Kruskal ngay lập tức
                            if (appState.isMSTActive) {
                                recordKruskalSteps(workingGraph, pricingModel, appState);
                                appState.currentKruskalStep = static_cast<int>(appState.kruskalHistory.size());
                            }
                        } else {
                            appState.statusMessage = "Không bấm trúng cạnh nào. Đã thoát chế độ Cắt Cáp.";
                        }
                        appState.isCutModeActive = false; // Tự tắt sau 1 lần click
                    }
                    // 2. Chế độ Chọn Node nguồn / đích để định tuyến
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
                                    std::string sName = workingGraph.getNode(nId).name.empty() ? ("Node " + std::to_string(nId)) : workingGraph.getNode(nId).name;
                                    appState.statusMessage = "Đã đặt trạm NGUỒN: " + sName + ". Bấm chuột trái vào trạm khác để định tuyến.";

                                    // Cập nhật lại đường đi nếu đã có đích
                                    if (appState.selectedDstNode != -1) {
                                        Packet qosPkt;
                                        qosPkt.id = 999;
                                        qosPkt.source = appState.selectedSrcNode;
                                        qosPkt.destination = appState.selectedDstNode;
                                        qosPkt.sizeBytes = 1500;
                                        qosPkt.protocol = Protocol::UDP;
                                        qosPkt.canFragment = true;
                                        if (appState.selectedPacketType == 0) qosPkt.type = PacketType::REAL_TIME;
                                        else if (appState.selectedPacketType == 1) qosPkt.type = PacketType::BULK_DATA;
                                        else qosPkt.type = PacketType::CONTROL;

                                        appState.currentPath = dijkstra(workingGraph, appState.selectedSrcNode, appState.selectedDstNode, qosPkt, false);
                                        appState.logicSimResult = Simulator::sendPacket(qosPkt, appState.currentPath, workingGraph);
                                    }
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
                                        
                                        Packet simPkt;
                                        simPkt.id = 999;
                                        simPkt.source = appState.selectedSrcNode;
                                        simPkt.destination = appState.selectedDstNode;
                                        simPkt.sizeBytes = 1500;
                                        simPkt.protocol = Protocol::UDP;
                                        simPkt.canFragment = true;
                                        if (appState.selectedPacketType == 0) simPkt.type = PacketType::REAL_TIME;
                                        else if (appState.selectedPacketType == 1) simPkt.type = PacketType::BULK_DATA;
                                        else simPkt.type = PacketType::CONTROL;

                                        appState.currentPath = dijkstra(workingGraph, appState.selectedSrcNode, appState.selectedDstNode, simPkt, false);
                                        appState.logicSimResult = Simulator::sendPacket(simPkt, appState.currentPath, workingGraph);
                                        appState.hasRoutingInfo = true;

                                        std::string srcName = workingGraph.getNode(appState.selectedSrcNode).name;
                                        std::string dstName = workingGraph.getNode(appState.selectedDstNode).name;
                                        if (srcName.empty()) srcName = "Node " + std::to_string(appState.selectedSrcNode);
                                        if (dstName.empty()) dstName = "Node " + std::to_string(appState.selectedDstNode);

                                        if (appState.currentPath.reachable) {
                                            std::stringstream ss;
                                            ss << "Định tuyến " << srcName << " -> " << dstName 
                                               << " [" << appState.currentPath.hopCount << " hops, Cự ly: " 
                                               << static_cast<int>(appState.currentPath.totalCost) << "m, Độ trễ: " 
                                               << std::fixed << std::setprecision(4) << appState.logicSimResult.totalDelay * 1000.0 << " ms]";
                                            appState.statusMessage = ss.str();
                                        } else {
                                            appState.statusMessage = "Định tuyến từ " + srcName + " đến " + dstName + " [Bị cô lập/Unreachable: Không có đường đi]";
                                        }
                                        
                                        // Tạo gói tin di chuyển (Module 5)
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

        // BƯỚC 3.2: Cập nhật thời gian & Hoạt họa gói tin (Module 5)
        sf::Time dtTime = deltaClock.restart();
        float dt = std::min(dtTime.asSeconds(), 0.033f);
        ImGui::SFML::Update(window, dtTime);

        // Cập nhật di chuyển các gói tin
        for (auto& pkt : appState.activePackets) {
            if (pkt.isDelivered || pkt.isBlocked) continue;
            pkt.progress += pkt.speed * dt;
            if (pkt.progress >= 1.0f) {
                pkt.progress = 0.0f;
                pkt.currentHopIndex++;
                if (pkt.currentHopIndex >= static_cast<int>(pkt.path.size()) - 1) {
                    pkt.isDelivered = true;
                    // Kích hoạt hiệu ứng sóng đích (Ripple)
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
                        PathResult newPath = dijkstra(workingGraph, u, pkt.destNode);
                        if (newPath.reachable) {
                            pkt.path = newPath.nodes;
                            pkt.currentHopIndex = 0;
                            pkt.progress = 0.0f;
                            appState.statusMessage = "Cáp đứt phía trước! Tự động chuyển hướng (Reroute) từ trạm " + std::to_string(u);
                        } else {
                            pkt.isBlocked = true;
                            appState.statusMessage = "LỖI: Tuyến phía trước bị đứt và không có đường thay thế (Unreachable)!";
                        }
                    }
                }
            }
        }

        // Cập nhật hiệu ứng sóng đích (Ripple)
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

            // Kiểm tra hover cạnh khi bật chế độ Cắt Cáp
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
                if (appState.isMSTActive && appState.isSidebarOpen && appState.sidebarMode == 0) {
                    appState.isMSTActive = false;
                    appState.isSidebarOpen = false;
                } else {
                    appState.isMSTActive = true;
                    appState.isScenarioActive = false;
                    appState.isSidebarOpen = true;
                    appState.sidebarMode = 0;
                    appState.switchTabRequested = true;
                    appState.requestedTab = 0;
                    recordKruskalSteps(workingGraph, pricingModel, appState);
                    appState.currentKruskalStep = static_cast<int>(appState.kruskalHistory.size());
                    appState.statusMessage = "Chế độ MST: Đã tính toán lại cây khung và so sánh 3 mức ngân sách.";
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
                    appState.statusMessage = "Chế độ Kịch Bản: Sẵn sàng điều khiển từng bước.";
                }
            }

            // Cụm nút phụ tua kịch bản khi chế độ kịch bản đang mở
            if (appState.isScenarioActive) {
                ImGui::SameLine(0, 8.0f);
                if (ImGui::Button("<")) {
                    if (appState.currentScenarioStep > 0) {
                        appState.currentScenarioStep--;
                        applyScenarioToStep(workingGraph, baseGraph, appState.loadedScenario, appState.currentScenarioStep, appState);
                        const auto& ev = appState.loadedScenario.events[appState.currentScenarioStep];
                        std::string evName = (ev.eventType == EventType::LINK_DOWN ? "LINK_DOWN" : (ev.eventType == EventType::LINK_UP ? "LINK_UP" : (ev.eventType == EventType::LOAD_UPDATE ? "LOAD_UPDATE" : "DELAY_UPDATE")));
                        appState.statusMessage = "Kịch bản [Bước " + std::to_string(appState.currentScenarioStep + 1) + "/" + std::to_string(appState.maxScenarioSteps) + "]: " + evName + " trên Edge #" + std::to_string(ev.targetId);
                    }
                }
                ImGui::SameLine(0, 4.0f);
                ImGui::TextColored(ImVec4(0.3f, 0.3f, 0.35f, 1.0f), "Bước %d/%d", appState.currentScenarioStep + 1, std::max(1, appState.maxScenarioSteps));
                ImGui::SameLine(0, 4.0f);
                if (ImGui::Button(">")) {
                    if (appState.currentScenarioStep < appState.maxScenarioSteps - 1) {
                        appState.currentScenarioStep++;
                        applyScenarioToStep(workingGraph, baseGraph, appState.loadedScenario, appState.currentScenarioStep, appState);
                        const auto& ev = appState.loadedScenario.events[appState.currentScenarioStep];
                        std::string evName = (ev.eventType == EventType::LINK_DOWN ? "LINK_DOWN" : (ev.eventType == EventType::LINK_UP ? "LINK_UP" : (ev.eventType == EventType::LOAD_UPDATE ? "LOAD_UPDATE" : "DELAY_UPDATE")));
                        appState.statusMessage = "Kịch bản [Bước " + std::to_string(appState.currentScenarioStep + 1) + "/" + std::to_string(appState.maxScenarioSteps) + "]: " + evName + " trên Edge #" + std::to_string(ev.targetId);
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
                recordKruskalSteps(workingGraph, pricingModel, appState);
            }
            ImGui::PopStyleColor(4);

            // Nhóm bên phải: Bảng số liệu & Thoát
            ImGui::SameLine(WINDOW_WIDTH - 235.0f);
            std::string drawerBtnLabel = appState.isSidebarOpen ? " Ẩn Bảng [X] " : " Bảng Số Liệu ";
            if (ImGui::Button(drawerBtnLabel.c_str())) {
                appState.isSidebarOpen = !appState.isSidebarOpen;
                if (!appState.isSidebarOpen) {
                    appState.isMSTActive = false;
                    appState.isScenarioActive = false;
                }
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
            ImGui::SetNextWindowPos(ImVec2(20.0f, TOP_BAR_HEIGHT + 8.0f), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(720.0f, 52.0f), ImGuiCond_Always);
            ImGuiWindowFlags pillFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                                         ImGuiWindowFlags_NoSavedSettings;
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(1.0f, 1.0f, 1.0f, 0.96f));
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.12f, 0.46f, 0.85f, 0.70f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.5f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 26.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 9.0f));

            if (ImGui::Begin("##RoutingFloatingPill", nullptr, pillFlags)) {
                std::string sN = workingGraph.getNode(appState.selectedSrcNode).name;
                std::string dN = (appState.selectedDstNode != -1) ? workingGraph.getNode(appState.selectedDstNode).name : "";
                if (sN.empty()) sN = "Node " + std::to_string(appState.selectedSrcNode);
                if (dN.empty() && appState.selectedDstNode != -1) dN = "Node " + std::to_string(appState.selectedDstNode);

                if (appState.selectedDstNode == -1) {
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextColored(ImVec4(0.12f, 0.46f, 0.85f, 1.0f), "Nguồn: %s", sN.c_str());
                    ImGui::SameLine(0, 10.0f);
                    ImGui::TextColored(ImVec4(0.5f, 0.55f, 0.6f, 1.0f), "| Click chuột trái vào trạm đích để định tuyến.");
                } else {
                    if (appState.currentPath.reachable) {
                        std::string pathSummary = "";
                        for (size_t i = 0; i < appState.currentPath.nodes.size(); ++i) {
                            int nid = appState.currentPath.nodes[i];
                            std::string nm = workingGraph.getNode(nid).name;
                            if (nm.empty()) nm = std::to_string(nid);
                            pathSummary += nm + (i + 1 < appState.currentPath.nodes.size() ? " -> " : "");
                        }
                        ImGui::AlignTextToFramePadding();
                        ImGui::TextColored(ImVec4(0.12f, 0.62f, 0.28f, 1.0f), "%s", pathSummary.c_str());
                        ImGui::SameLine(0, 8.0f);
                        ImGui::TextColored(ImVec4(0.4f, 0.45f, 0.5f, 1.0f), "(%dm, %.2fms)", 
                                           static_cast<int>(appState.currentPath.totalCost), 
                                           appState.logicSimResult.totalDelay * 1000.0);
                    } else {
                        ImGui::AlignTextToFramePadding();
                        ImGui::TextColored(ImVec4(0.85f, 0.2f, 0.2f, 1.0f), "%s -> %s: CÔ LẬP (UNREACHABLE)", sN.c_str(), dN.c_str());
                    }

                    // Bộ chọn loại gói QoS
                    ImGui::SameLine(0, 12.0f);
                    ImGui::TextColored(ImVec4(0.75f, 0.78f, 0.82f, 1.0f), "|");
                    ImGui::SameLine(0, 10.0f);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("QoS:");
                    ImGui::SameLine(0, 4.0f);

                    int prevQoS = appState.selectedPacketType;
                    ImGui::RadioButton("Delay", &appState.selectedPacketType, 0);
                    ImGui::SameLine(0, 4.0f);
                    ImGui::RadioButton("Tải", &appState.selectedPacketType, 1);
                    ImGui::SameLine(0, 4.0f);
                    ImGui::RadioButton("Hop", &appState.selectedPacketType, 2);

                    if (prevQoS != appState.selectedPacketType) {
                        Packet qosPkt;
                        qosPkt.id = 999;
                        qosPkt.source = appState.selectedSrcNode;
                        qosPkt.destination = appState.selectedDstNode;
                        qosPkt.sizeBytes = 1500;
                        qosPkt.protocol = Protocol::UDP;
                        qosPkt.canFragment = true;
                        if (appState.selectedPacketType == 0) qosPkt.type = PacketType::REAL_TIME;
                        else if (appState.selectedPacketType == 1) qosPkt.type = PacketType::BULK_DATA;
                        else qosPkt.type = PacketType::CONTROL;

                        appState.currentPath = dijkstra(workingGraph, appState.selectedSrcNode, appState.selectedDstNode, qosPkt, false);
                        appState.logicSimResult = Simulator::sendPacket(qosPkt, appState.currentPath, workingGraph);
                    }

                    // Nút Gửi gói tin
                    ImGui::SameLine(0, 10.0f);
                    if (appState.currentPath.reachable && appState.currentPath.nodes.size() >= 2) {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.62f, 0.28f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                        if (ImGui::Button("Gửi Gói Tin")) {
                            PacketAnim p;
                            p.sourceNode = appState.selectedSrcNode;
                            p.destNode = appState.selectedDstNode;
                            p.path = appState.currentPath.nodes;
                            p.currentHopIndex = 0;
                            p.progress = 0.0f;
                            p.speed = 1.0f;
                            appState.activePackets.push_back(p);
                            appState.statusMessage = "Đang gửi gói tin từ " + sN + " -> " + dN + "...";
                        }
                        ImGui::PopStyleColor(2);
                    }
                }
            }
            ImGui::End();
            ImGui::PopStyleVar(3);
            ImGui::PopStyleColor(2);
        }

        // ====================================================================
        // MODULE 3: SIDEBAR RESIZABLE DRAWER (280px -> 420px, Cao 650px)
        // ====================================================================
        if (appState.isSidebarOpen) {
            ImGui::SetNextWindowPos(ImVec2(WINDOW_WIDTH - DRAWER_DEFAULT_WIDTH, TOP_BAR_HEIGHT), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSizeConstraints(ImVec2(DRAWER_MIN_WIDTH, DRAWER_HEIGHT), ImVec2(DRAWER_MAX_WIDTH, DRAWER_HEIGHT));

            ImGuiWindowFlags drawerFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(1.0f, 1.0f, 1.0f, 0.98f));
            if (ImGui::Begin("Bảng Dữ Liệu##Sidebar", &appState.isSidebarOpen, drawerFlags)) {
                if (ImGui::BeginTabBar("SidebarTabs")) {
                    // TAB 1: KRUSKAL STEP-BY-STEP & SO SÁNH 3 MỨC NGÂN SÁCH (TASK B2.4)
                    ImGuiTabItemFlags tab1Flags = 0;
                    if (appState.switchTabRequested && appState.requestedTab == 0) {
                        tab1Flags |= ImGuiTabItemFlags_SetSelected;
                    }
                    if (ImGui::BeginTabItem(" Quy Hoạch Cáp (MST) ", nullptr, tab1Flags)) {
                        appState.sidebarMode = 0;
                        appState.isMSTActive = true;
                        appState.isScenarioActive = false;

                        // 1. BẢNG SO SÁNH 3 MỨC NGÂN SÁCH (TASK B2.4)
                        auto costFn = [&pricingModel](const Edge& e) { return pricingModel.calculateCablingCost(e).total; };
                        double costAll = 0.0;
                        for (int eId : workingGraph.edgeIds()) {
                            costAll += costFn(workingGraph.getEdge(eId));
                        }

                        Graph gSim = workingGraph;
                        MSTResult mstSummary = buildMST(gSim, costFn);
                        BackupResult bkSummary = selectBackupLinks(gSim, mstSummary, DEFAULT_BACKUP_LINKS, costFn);

                        double costMST = mstSummary.totalCost;
                        double costBackup = bkSummary.totalCost;
                        double costMSTBackup = costMST + costBackup;
                        double savedPct = (costAll > 0.0) ? ((costAll - costMST) / costAll * 100.0) : 0.0;

                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.12f, 0.46f, 0.85f, 1.0f), "SO SÁNH 3 MỨC NGÂN SÁCH (TASK B2.4):");
                        if (ImGui::BeginTable("BudgetCompare", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                            ImGui::TableSetupColumn("Phương Án");
                            ImGui::TableSetupColumn("Chi Phí");
                            ImGui::TableSetupColumn("Bảo Vệ / Lợi Ích");
                            ImGui::TableHeadersRow();

                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("1. Toàn Bộ");
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%.0f ĐV", costAll);
                            ImGui::TableSetColumnIndex(2);
                            ImGui::TextColored(ImVec4(0.85f, 0.25f, 0.25f, 1.0f), "Tốn kém (%d cáp)", static_cast<int>(workingGraph.edgeIds().size()));

                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::TextColored(ImVec4(0.12f, 0.62f, 0.28f, 1.0f), "2. Cây Khung (MST)");
                            ImGui::TableSetColumnIndex(1);
                            ImGui::TextColored(ImVec4(0.12f, 0.62f, 0.28f, 1.0f), "%.0f ĐV", costMST);
                            ImGui::TableSetColumnIndex(2);
                            ImGui::TextColored(ImVec4(0.12f, 0.62f, 0.28f, 1.0f), "-%.1f%% (%d cáp)", savedPct, static_cast<int>(mstSummary.edgeIds.size()));

                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::TextColored(ImVec4(0.2f, 0.45f, 0.85f, 1.0f), "3. MST + Dự Phòng");
                            ImGui::TableSetColumnIndex(1);
                            ImGui::TextColored(ImVec4(0.2f, 0.45f, 0.85f, 1.0f), "%.0f ĐV", costMSTBackup);
                            ImGui::TableSetColumnIndex(2);
                            ImGui::TextColored(ImVec4(0.2f, 0.45f, 0.85f, 1.0f), "%d/%d cạnh cây (+%d bk)", bkSummary.protectedTreeEdges, bkSummary.treeEdges, static_cast<int>(bkSummary.edgeIds.size()));

                            ImGui::EndTable();
                        }

                        ImGui::Spacing();
                        ImGui::Separator();
                        ImGui::TextColored(ImVec4(0.12f, 0.62f, 0.28f, 1.0f), "Thuật toán Kruskal Từng Bước:");
                        
                        // Điều khiển Replay Kruskal
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
                        if (ImGui::BeginChild("KruskalLogList", ImVec2(0, 260), true)) {
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

                    // TAB 2: MÔ PHỎNG KỊCH BẢN (MODULE 5)
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
                        if (ImGui::BeginCombo("##ScenarioCombo", appState.scenarioNames[appState.activeScenarioIdx].c_str())) {
                            for (int i = 0; i < static_cast<int>(appState.scenarioNames.size()); ++i) {
                                bool isSel = (appState.activeScenarioIdx == i);
                                if (ImGui::Selectable(appState.scenarioNames[i].c_str(), isSel)) {
                                    appState.activeScenarioIdx = i;
                                    workingGraph = baseGraph;
                                    IOManager::loadScenario("data/scenarios/" + appState.scenarioFiles[i], appState.loadedScenario);
                                    appState.maxScenarioSteps = static_cast<int>(appState.loadedScenario.events.size());
                                    appState.currentScenarioStep = 0;
                                    applyScenarioToStep(workingGraph, baseGraph, appState.loadedScenario, 0, appState);
                                    appState.statusMessage = "Đã nạp: " + appState.scenarioNames[i];
                                }
                            }
                            ImGui::EndCombo();
                        }

                        ImGui::Separator();
                        ImGui::Text("Tiến trình: Bước %d / %d", appState.maxScenarioSteps == 0 ? 0 : appState.currentScenarioStep + 1, appState.maxScenarioSteps);
                        if (ImGui::Button(" [<< Lùi bước] ") && appState.currentScenarioStep > 0) {
                            appState.currentScenarioStep--;
                            applyScenarioToStep(workingGraph, baseGraph, appState.loadedScenario, appState.currentScenarioStep, appState);
                        }
                        ImGui::SameLine();
                        if (ImGui::Button(" [Tiến bước >>] ") && appState.currentScenarioStep < appState.maxScenarioSteps - 1) {
                            appState.currentScenarioStep++;
                            applyScenarioToStep(workingGraph, baseGraph, appState.loadedScenario, appState.currentScenarioStep, appState);
                        }
                        ImGui::SameLine();
                        if (ImGui::Button(" [Chạy từ đầu] ")) {
                            appState.currentScenarioStep = 0;
                            applyScenarioToStep(workingGraph, baseGraph, appState.loadedScenario, appState.currentScenarioStep, appState);
                        }

                        ImGui::Separator();
                        ImGui::TextColored(ImVec4(0.2f, 0.4f, 0.7f, 1.0f), "Danh sách sự kiện (Bấm trực tiếp để chuyển bước):");
                        if (ImGui::BeginChild("ScenarioEvents", ImVec2(0, 320), true)) {
                            if (appState.loadedScenario.events.empty()) {
                                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "(Kịch bản không có sự kiện động)");
                            }
                            for (int i = 0; i < static_cast<int>(appState.loadedScenario.events.size()); ++i) {
                                const auto& ev = appState.loadedScenario.events[i];
                                std::string evName = (ev.eventType == EventType::LINK_DOWN ? "LINK_DOWN" : (ev.eventType == EventType::LINK_UP ? "LINK_UP" : (ev.eventType == EventType::LOAD_UPDATE ? "LOAD_UPDATE" : "DELAY_UPDATE")));
                                
                                std::string edgeName = "";
                                if (workingGraph.hasEdge(ev.targetId)) {
                                    const auto& ed = workingGraph.getEdge(ev.targetId);
                                    std::string uN = workingGraph.getNode(ed.u).name;
                                    std::string vN = workingGraph.getNode(ed.v).name;
                                    edgeName = " (" + uN + " - " + vN + ")";
                                }

                                char labelBuf[128];
                                if (ev.eventType == EventType::LOAD_UPDATE) {
                                    std::snprintf(labelBuf, sizeof(labelBuf), "[%.1fs] %s trên Edge #%d%s (Tải: %.0f%%)##ev%d", ev.time, evName.c_str(), ev.targetId, edgeName.c_str(), ev.value * 100.0, i);
                                } else if (ev.eventType == EventType::DELAY_UPDATE) {
                                    std::snprintf(labelBuf, sizeof(labelBuf), "[%.1fs] %s trên Edge #%d%s (Delay: %.4fs)##ev%d", ev.time, evName.c_str(), ev.targetId, edgeName.c_str(), ev.value, i);
                                } else {
                                    std::snprintf(labelBuf, sizeof(labelBuf), "[%.1fs] %s trên Edge #%d%s##ev%d", ev.time, evName.c_str(), ev.targetId, edgeName.c_str(), i);
                                }

                                bool isCurrent = (i == appState.currentScenarioStep);
                                if (isCurrent) {
                                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.55f, 0.10f, 1.0f));
                                }
                                if (ImGui::Selectable(labelBuf, isCurrent)) {
                                    appState.currentScenarioStep = i;
                                    applyScenarioToStep(workingGraph, baseGraph, appState.loadedScenario, appState.currentScenarioStep, appState);
                                    appState.statusMessage = "Đã chuyển kịch bản đến bước " + std::to_string(i + 1) + ": " + evName;
                                }
                                if (isCurrent) {
                                    ImGui::PopStyleColor();
                                }
                            }
                        }
                        ImGui::EndChild();

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
        // MODULE 2: NATIVE HOVER TOOLTIP (BẢNG ĐỊNH TUYẾN KHI RÊ CHUỘT)
        // ====================================================================
        if (appState.hoveredNodeId != -1 && !ImGui::GetIO().WantCaptureMouse) {
            std::string hName = workingGraph.getNode(appState.hoveredNodeId).name;
            if (hName.empty()) hName = "Node " + std::to_string(appState.hoveredNodeId);

            ImGui::BeginTooltip();
            ImGui::TextColored(ImVec4(0.12f, 0.46f, 0.85f, 1.0f), "BẢNG ĐỊNH TUYẾN - %s", hName.c_str());
            ImGui::Separator();
            if (ImGui::BeginTable("RoutingHover", 3, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Đích");
                ImGui::TableSetupColumn("Next-Hop");
                ImGui::TableSetupColumn("Cost");
                ImGui::TableHeadersRow();

                for (int destId : workingGraph.nodeIds()) {
                    if (destId == appState.hoveredNodeId) continue;
                    PathResult res = dijkstra(workingGraph, appState.hoveredNodeId, destId);

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
                        ImGui::Text("%.2f", res.totalCost);
                    } else {
                        ImGui::TextColored(ImVec4(0.85f, 0.2f, 0.2f, 1.0f), "INF");
                    }
                }
                ImGui::EndTable();
            }

            // Tích hợp phần tóm tắt lộ trình vào Footer bảng popup nếu đây là trạm đích
            if (appState.selectedDstNode == appState.hoveredNodeId && appState.selectedSrcNode != -1) {
                ImGui::Separator();
                std::string sN = workingGraph.getNode(appState.selectedSrcNode).name;
                if (sN.empty()) sN = "Node " + std::to_string(appState.selectedSrcNode);
                ImGui::TextColored(ImVec4(0.12f, 0.62f, 0.28f, 1.0f), "Lộ trình từ nguồn %s:", sN.c_str());
                if (appState.currentPath.reachable) {
                    std::string pStr = "";
                    for (size_t i = 0; i < appState.currentPath.nodes.size(); ++i) {
                        int nid = appState.currentPath.nodes[i];
                        std::string nm = workingGraph.getNode(nid).name;
                        if (nm.empty()) nm = std::to_string(nid);
                        pStr += nm + (i + 1 < appState.currentPath.nodes.size() ? " -> " : "");
                    }
                    ImGui::Text("%s", pStr.c_str());
                    ImGui::Text("Cự ly: %d m  |  Độ trễ: %.4f ms", static_cast<int>(appState.currentPath.totalCost), appState.logicSimResult.totalDelay * 1000.0);
                } else {
                    ImGui::TextColored(ImVec4(0.85f, 0.2f, 0.2f, 1.0f), "Tuyến bị cô lập (Unreachable)!");
                }
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
            ImGui::TextColored(ImVec4(0.15f, 0.55f, 0.25f, 1.0f), "[TRẠNG THÁI]:");
            ImGui::SameLine();
            ImGui::TextUnformatted(appState.statusMessage.c_str());
        }
        ImGui::End();
        ImGui::PopStyleVar();

        // ====================================================================
        // VẼ ĐỒ THỊ MẠNG TRÊN CANVAS SFML (MỤC 20.2 & 20.5)
        // THỨ TỰ: 1. NỀN & LƯỚI -> 2. CÁP -> 3. BADGES -> 4. TRẠM -> 5. GÓI TIN
        // ====================================================================
        window.clear(sf::Color(248, 250, 252)); // Nền sáng hiện đại #F8FAFC

        // 1. VẼ LƯỚI CHẤM KỸ THUẬT (GRID DOTS)
        window.draw(gridDots);

        // 2. VẼ TOÀN BỘ CÁP QUANG (EDGES)
        for (int eId : workingGraph.edgeIds()) {
            const Edge& edge = workingGraph.getEdge(eId);
            if (!nodePositions.count(edge.u) || !nodePositions.count(edge.v)) continue;

            sf::Vector2f p1 = nodePositions[edge.u];
            sf::Vector2f p2 = nodePositions[edge.v];

            // Kiểm tra xem cạnh này có nằm trên tuyến định tuyến đang chọn không
            bool inRoutingPath = false;
            if (appState.currentPath.reachable && appState.currentPath.nodes.size() >= 2) {
                for (size_t i = 0; i < appState.currentPath.nodes.size() - 1; ++i) {
                    int u = appState.currentPath.nodes[i];
                    int v = appState.currentPath.nodes[i + 1];
                    if ((edge.u == u && edge.v == v) || (edge.u == v && edge.v == u)) {
                        inRoutingPath = true;
                        break;
                    }
                }
            }

            // Xác định màu sắc và độ dày theo trạng thái
            sf::Color edgeColor(180, 190, 202, 170); // Mặc định xám nhạt
            float thickness = 2.0f;

            bool isEdgeHovered = (eId == hoveredEdgeId && appState.isCutModeActive);

            if (!edge.isUp) {
                edgeColor = sf::Color(239, 68, 68, 255); // Cáp bị đứt: Đỏ rực
                thickness = 2.5f;
            } else if (inRoutingPath) {
                edgeColor = sf::Color(245, 140, 20, 255); // Đường định tuyến: Cam đậm nổi bật
                thickness = 3.6f;
            } else if (appState.isMSTActive && appState.sidebarMode == 0) {
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
                    edgeColor = sf::Color(245, 175, 25, 255); // Vàng kim nhấp nháy
                    thickness = 3.8f;
                } else if (inMST) {
                    edgeColor = sf::Color(35, 170, 75, 255);  // Cây khung MST: Xanh lá đậm
                    thickness = 3.2f;
                }
            } else if (edge.isBackup) {
                edgeColor = sf::Color(59, 130, 246, 200); // Cáp dự phòng: Xanh dương
                thickness = 2.6f;
            } else if (edge.isBuilt) {
                edgeColor = sf::Color(35, 170, 75, 220);  // Cáp đã lắp: Xanh lá
                thickness = 3.0f;
            }

            // Nếu đang hover trong chế độ Cắt Cáp: Vẽ hào quang đỏ nhấp nháy
            if (isEdgeHovered) {
                edgeColor = sf::Color(239, 68, 68, 255);
                thickness = 5.0f;
            }

            // Vẽ đường cáp (Uốn cong nếu là đường chéo R2-R4, thẳng nếu là cạnh khác)
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

        // 3. VẼ NHÃN THÔNG SỐ BADGE TRÊN MỖI ĐƯỜNG TRUYỀN (LINK METRICS BADGE)
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

                // Cạnh R1-R3 đi qua tâm (500, 370) -> Đặt badge hơi dịch nhẹ để tránh che tâm
                if ((edge.u == 0 && edge.v == 2) || (edge.u == 2 && edge.v == 0)) {
                    badgePos.x -= 22.0f;
                    badgePos.y += 18.0f;
                }

                bool inRoutingPath = false;
                if (appState.currentPath.reachable && appState.currentPath.nodes.size() >= 2) {
                    for (size_t i = 0; i < appState.currentPath.nodes.size() - 1; ++i) {
                        int u = appState.currentPath.nodes[i];
                        int v = appState.currentPath.nodes[i + 1];
                        if ((edge.u == u && edge.v == v) || (edge.u == v && edge.v == u)) {
                            inRoutingPath = true;
                            break;
                        }
                    }
                }

                std::string badgeText = "";
                sf::Color badgeBg(255, 255, 255, 230);
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
                    } else if (edge.isBackup) {
                        badgeBg = sf::Color(239, 246, 255);
                        badgeBorder = sf::Color(59, 130, 246);
                        badgeTextColor = sf::Color(29, 78, 216);
                    }
                } else { // Lớp Logic (Routing)
                    badgeText = std::to_string(static_cast<int>(edge.length)) + "m";
                    if (inRoutingPath) {
                        badgeBg = sf::Color(254, 243, 199);
                        badgeBorder = sf::Color(245, 158, 11);
                        badgeTextColor = sf::Color(180, 83, 9);
                    }
                }

                drawLinkBadge(window, sfmlFont, badgePos, badgeText, badgeBg, badgeBorder, badgeTextColor);
            }
        }

        // 4. VẼ CÁC TRẠM MẠNG (ROUTER / HOST) VÀ HUY HIỆU SRC, DST
        for (const auto& [nId, pos] : nodePositions) {
            bool isHost = (workingGraph.getNode(nId).type == NodeType::HOST);
            bool isSelectedSrc = (nId == appState.selectedSrcNode);
            bool isSelectedDst = (nId == appState.selectedDstNode);
            bool isHovered = (nId == appState.hoveredNodeId);

            // VẼ VÒNG HÀO QUANG LỰA CHỌN (SELECTION HALO)
            if (isSelectedSrc) {
                float haloR = isHost ? 26.0f : (NODE_RADIUS + 7.0f);
                sf::CircleShape halo(haloR);
                halo.setOrigin({haloR, haloR});
                halo.setPosition(pos);
                halo.setFillColor(sf::Color(22, 163, 74, 35));
                halo.setOutlineColor(sf::Color(22, 163, 74));
                halo.setOutlineThickness(2.5f);
                window.draw(halo);

                // Huy hiệu [SRC] phía trên node
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

                // Huy hiệu [DST] phía trên node
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
                // ==========================================
                // VẼ HOST (MÁY TRẠM PC HIỆN ĐẠI)
                // ==========================================
                // 1. Màn hình ngoài
                sf::Vector2f screenSize(36.0f, 26.0f);
                sf::RectangleShape screen(screenSize);
                screen.setOrigin({screenSize.x / 2.0f, screenSize.y / 2.0f + 4.0f});
                screen.setPosition(pos);
                screen.setFillColor(sf::Color::White);
                screen.setOutlineColor(sf::Color(71, 85, 105)); // Slate #475569
                screen.setOutlineThickness(2.0f);
                window.draw(screen);

                // 2. Viền panel bên trong màn hình (xám xanh nhạt #E2E8F0)
                sf::Vector2f innerSize(28.0f, 18.0f);
                sf::RectangleShape innerScreen(innerSize);
                innerScreen.setOrigin({innerSize.x / 2.0f, innerSize.y / 2.0f + 4.0f});
                innerScreen.setPosition(pos);
                innerScreen.setFillColor(sf::Color(226, 232, 240));
                window.draw(innerScreen);

                // 3. Chân đế PC
                sf::RectangleShape stand(sf::Vector2f(4.0f, 6.0f));
                stand.setOrigin({2.0f, 0.0f});
                stand.setPosition(sf::Vector2f(pos.x, pos.y + 9.0f));
                stand.setFillColor(sf::Color(100, 116, 139));
                window.draw(stand);

                sf::RectangleShape basePlate(sf::Vector2f(16.0f, 3.5f));
                basePlate.setOrigin({8.0f, 0.0f});
                basePlate.setPosition(sf::Vector2f(pos.x, pos.y + 15.0f));
                basePlate.setFillColor(sf::Color(100, 116, 139));
                window.draw(basePlate);

                // 4. Tên trạm H1, H2
                if (hasSfmlFont) {
                    sf::Text hostText(sfmlFont);
                    std::string label = workingGraph.getNode(nId).name.empty() ? ("H" + std::to_string(nId)) : workingGraph.getNode(nId).name;
                    hostText.setString(label);
                    hostText.setCharacterSize(12);
                    hostText.setFillColor(sf::Color(30, 41, 59));
                    hostText.setStyle(sf::Text::Bold);
                    sf::FloatRect bounds = hostText.getLocalBounds();
                    hostText.setOrigin({bounds.position.x + bounds.size.x / 2.0f, bounds.position.y + bounds.size.y / 2.0f + 4.0f});
                    hostText.setPosition(pos);
                    window.draw(hostText);
                }
            } else {
                // ==========================================
                // VẼ ROUTER (HÌNH TRÒN CHUẨN CISCO XANH DƯƠNG #2563EB)
                // ==========================================
                sf::CircleShape routerCircle(NODE_RADIUS);
                routerCircle.setOrigin({NODE_RADIUS, NODE_RADIUS});
                routerCircle.setPosition(pos);
                routerCircle.setFillColor(sf::Color::White);
                routerCircle.setOutlineColor(sf::Color(37, 99, 235)); // Cisco Blue #2563EB
                routerCircle.setOutlineThickness(2.5f);
                window.draw(routerCircle);

                // 4 Mũi tên Cisco xanh đậm #1D4ED8
                float arrowLen = 13.0f;
                sf::Color arrowColor(29, 78, 216);
                
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

                // Đầu mũi tên Cisco
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

                // Tên trạm Router bên dưới vòng tròn
                if (hasSfmlFont) {
                    sf::Text routerText(sfmlFont);
                    std::string label = workingGraph.getNode(nId).name.empty() ? ("R" + std::to_string(nId)) : workingGraph.getNode(nId).name;
                    routerText.setString(label);
                    routerText.setCharacterSize(13);
                    routerText.setFillColor(sf::Color(25, 40, 65));
                    routerText.setStyle(sf::Text::Bold);
                    sf::FloatRect bounds = routerText.getLocalBounds();
                    routerText.setOrigin({bounds.position.x + bounds.size.x / 2.0f, bounds.position.y});
                    routerText.setPosition(sf::Vector2f(pos.x, pos.y + NODE_RADIUS + 4.0f));
                    window.draw(routerText);
                }
            }
        }

        // 5. VẼ HIỆU ỨNG SÓNG LAN TỎA KHI GÓI TIN ĐẾN ĐÍCH (RIPPLE)
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

        // 6. VẼ HOẠT HỌA GÓI TIN DI CHUYỂN
        for (const auto& pkt : appState.activePackets) {
            if (pkt.isDelivered || pkt.isBlocked || pkt.currentHopIndex >= static_cast<int>(pkt.path.size()) - 1) continue;
            int u = pkt.path[pkt.currentHopIndex];
            int v = pkt.path[pkt.currentHopIndex + 1];
            if (nodePositions.count(u) && nodePositions.count(v)) {
                sf::Vector2f a = nodePositions[u];
                sf::Vector2f b = nodePositions[v];
                sf::Vector2f pktPos;

                if (isCurvedDiagonal(u, v)) {
                    if (u == 1) {
                        pktPos = evalQuadraticBezier(a, r2r4Ctrl, b, pkt.progress);
                    } else {
                        pktPos = evalQuadraticBezier(a, r2r4Ctrl, b, pkt.progress);
                    }
                } else {
                    pktPos = sf::Vector2f(
                        (1.0f - pkt.progress) * a.x + pkt.progress * b.x,
                        (1.0f - pkt.progress) * a.y + pkt.progress * b.y
                    );
                }

                // Hào quang tỏa sáng xung quanh gói tin
                sf::CircleShape glow(12.0f);
                glow.setOrigin({12.0f, 12.0f});
                glow.setPosition(pktPos);
                glow.setFillColor(sf::Color(255, 180, 0, 80));
                window.draw(glow);

                // Thân gói tin hình tròn màu cam/vàng đậm nổi bật
                sf::CircleShape pktShape(8.0f);
                pktShape.setOrigin({8.0f, 8.0f});
                pktShape.setPosition(pktPos);
                pktShape.setFillColor(sf::Color(255, 120, 0));
                pktShape.setOutlineColor(sf::Color::White);
                pktShape.setOutlineThickness(2.0f);
                window.draw(pktShape);
            }
        }

        // Render ImGui & hiển thị khung hình
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}
