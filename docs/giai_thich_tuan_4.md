# BÁO CÁO VÀ GIẢI THÍCH CHI TIẾT TẤT CẢ CÁC FILE TUẦN 4
**Chủ đề:** QoS + Congestion Penalty + Dynamic Cost + Simulation (Mô phỏng mạng động)

---

Tài liệu này tổng hợp toàn bộ các đoạn code quan trọng nhất của **Tuần 4**, kèm comment giải thích chi tiết ngay tại từng dòng code và phân tích cơ chế hoạt động.

---

## 1. Task A4.1 – Hệ số phạt tắc nghẽn (`congestionPenalty`)

* **File:** [include/RoutingCost.h](file:///d:/PBL2/Project_PBL2/include/RoutingCost.h) & [src/RoutingCost.cpp](file:///d:/PBL2/Project_PBL2/src/RoutingCost.cpp)
* **Ý nghĩa:** Trong mạng viễn thông thực tế, khi hàng đợi router đầy quá 80%, hiện tượng nghẽn (bufferbloat) xuất hiện làm trễ hàng đợi tăng vọt. Thuật toán áp dụng hệ số phạt phi tuyến tính theo từng mốc tải.

```cpp
// Hàm tính hệ số phạt dựa theo tỷ lệ tải của cạnh (load nằm trong đoạn [0.0, 1.0])
double congestionPenalty(double load) {
    // Mốc 1: Dưới 80% tải - Mạng thông thoáng, hoạt động bình thường, không phạt
    if (load <= 0.80) return 1.0;

    // Mốc 2: Từ 80% đến 85% tải - Bắt đầu chớm nghẽn nhẹ, phạt tăng 56%
    if (load <= 0.85) return 1.56;

    // Mốc 3: Từ 85% đến 90% tải - Nghẽn đáng kể, độ trễ hàng đợi bị nhân hơn 3 lần
    if (load <= 0.90) return 3.25;

    // Mốc 4: Từ 90% đến 95% tải - Nghẽn nghiêm trọng, phạt gấp 6 lần
    if (load <= 0.95) return 6.06;

    // Mốc 5: Trên 95% đến 100% tải - Hàng đợi gần tràn (bão hòa), phạt cực đại gấp 10 lần
    // Mục đích: Ép Dijkstra coi chi phí qua cạnh này cực đắt để tự động bẻ lái sang đường khác
    return 10.00;
}
```

* **Điểm cốt lõi:**
  - Áp dụng nguyên tắc DRY: Hàm `DelayModel::calcQueueDelay()` cũng tái sử dụng trực tiếp hàm này để tính trễ hàng đợi thực tế, đảm bảo không nhân bản logic.

---

## 2. Task A4.2 – Cấu hình phân loại chất lượng dịch vụ QoS (`QoSWeight`)

* **File:** [include/Config.h](file:///d:/PBL2/Project_PBL2/include/Config.h)
* **Ý nghĩa:** Chuẩn hóa các bộ trọng số thành `struct QoSWeight` giúp mỗi loại gói tin có một tiêu chí đánh giá đường đi riêng biệt.

```cpp
// Cấu trúc nhóm 3 trọng số đánh giá chi phí theo chất lượng dịch vụ (QoS)
struct QoSWeight {
    double wTrans;  // Trọng số cho độ trễ truyền dữ liệu (Transmission Delay)
    double wProp;   // Trọng số cho độ trễ lan truyền vật lý (Propagation Delay)
    double wQueue;  // Trọng số cho độ trễ hàng đợi (Queue Delay)
};

// 1. Gói REAL_TIME (thoại VoIP, video call trực tiếp):
// Trọng số hàng đợi cực cao (wQueue = 4.0). Sẵn sàng đi vòng đường xa (chấp nhận wTrans, wProp) 
// miễn là đường vắng, hàng đợi rảnh để tránh hiện tượng giật lag, trễ khung hình.
const QoSWeight QOS_WEIGHT_REAL_TIME = {1.0, 1.0, 4.0};

// 2. Gói BULK_DATA (tải file dung lượng lớn qua FTP, HTTP download):
// Ưu tiên đường có băng thông lớn (wTrans = 2.0), chấp nhận chờ hàng đợi vừa phải (wQueue = 2.0).
const QoSWeight QOS_WEIGHT_BULK_DATA = {2.0, 1.0, 2.0};

// 3. Gói CONTROL (gói điều khiển mạng, định tuyến OSPF/BGP):
// Trọng số cân bằng chuẩn mực (1.0, 1.0, 1.0).
const QoSWeight QOS_WEIGHT_CONTROL   = {1.0, 1.0, 1.0};
```

---

## 3. Task A4.3 – Chi phí định tuyến động (`calculateRoutingCost`)

* **File:** [src/RoutingCost.cpp](file:///d:/PBL2/Project_PBL2/src/RoutingCost.cpp)
* **Ý nghĩa:** Tính chi phí "ảo" của một cạnh để Dijkstra so sánh tìm đường đi tối ưu nhất theo từng loại gói tin và mức độ nghẽn tức thời.

```cpp
double calculateRoutingCost(const Packet& packet, const Edge& edge) {
    // 1. Kiểm tra an toàn vật lý: Nếu dây cáp đang bị đứt (isUp == false)
    // -> Gán chi phí bằng vô cực (INF) để Dijkstra tuyệt đối không đi qua dây này
    if (!edge.isUp) return Config::INF;

    // 2. Kiểm tra MTU & Cờ DF (Don't Fragment):
    // Gọi hàm calcFragmentCount của DelayModel để kiểm tra tính khả thi
    int fragments = DelayModel::calcFragmentCount(packet.sizeBytes, edge.mtu, packet.canFragment);
    // Nếu gói tin cấm phân mảnh (canFragment = false) mà kích thước lại vượt quá MTU của dây
    // -> Hàm trả về 0 mảnh -> Cạnh này không thể chuyển tải gói tin -> Báo chi phí INF
    if (fragments == 0) return Config::INF;

    // 3. Lựa chọn bộ trọng số QoS dựa trên loại gói tin
    const Config::QoSWeight* w = nullptr;
    switch (packet.type) {
        case PacketType::REAL_TIME: w = &Config::QOS_WEIGHT_REAL_TIME; break;
        case PacketType::BULK_DATA: w = &Config::QOS_WEIGHT_BULK_DATA; break;
        case PacketType::CONTROL:   w = &Config::QOS_WEIGHT_CONTROL;   break;
        default:                    w = &Config::QOS_WEIGHT_CONTROL;   break;
    }

    // 4. Tái sử dụng các thành phần trễ chuẩn từ DelayModel (tránh viết lại logic)
    // - Trễ truyền tải: dTrans = Packet_Size / Bandwidth
    double dTrans = DelayModel::calcTransmissionDelay(packet.sizeBytes, edge.bandwidthMbps);

    // - Xác định tốc độ truyền ánh sáng dựa theo loại cáp quang hay sóng vô tuyến
    double propSpeed = (edge.mediaType == MediaType::WIRELESS)
                            ? Config::SPEED_OF_LIGHT_AIR
                            : Config::SPEED_OF_LIGHT_FIBER;
    // - Trễ lan truyền: dProp = Length / Speed
    double dProp = DelayModel::calcPropagationDelay(edge.length, propSpeed);

    // - Trễ hàng đợi đã nhân hệ số phạt: Càng tải cao, dQueue càng bị đội lên nhiều lần
    double dQueue = edge.queueDelay * congestionPenalty(edge.currentLoad);

    // 5. Tính phí phạt nếu gói tin phải xé nhỏ (phân mảnh):
    // Cứ mỗi mảnh dư thừa ngoài mảnh gốc sẽ bị cộng thêm chi phí xử lý đóng gói
    double fragmentPenalty = (fragments > 1)
                                  ? (fragments - 1) * Config::FRAGMENT_PENALTY_PER_EXTRA
                                  : 0.0;

    // 6. Tổng hợp chi phí định tuyến theo công thức QoS
    // Không cộng dProc (trễ xử lý router đích) vì dProc không phụ thuộc vào cạnh đang xét
    double cost = w->wTrans * dTrans + w->wProp * dProp + w->wQueue * dQueue + fragmentPenalty;

    // 7. Lập trình phòng thủ (Defensive Programming):
    // Nếu xảy ra lỗi chia cho 0 hoặc tràn số làm ra NaN, ép trả về INF để Dijkstra không bị hỏng
    if (std::isnan(cost)) return Config::INF;

    return cost; // Trả về chi phí định tuyến hợp lệ
}
```

---

## 4. Task A4.4 – Đóng gói hàm trọng số Dijkstra (`makeWeightFn`)

* **File:** [src/RoutingCost.cpp](file:///d:/PBL2/Project_PBL2/src/RoutingCost.cpp)
* **Ý nghĩa:** Biến đổi hàm tính toán chi phí phức tạp phụ thuộc cả `Packet` và `Edge` thành một lambda closure chỉ nhận `const Edge& e`, khớp hoàn toàn với signature `WeightFn` của thuật toán Dijkstra.

```cpp
WeightFn makeWeightFn(const Packet& packet) {
    // Sử dụng C++ Lambda bắt giữ bản sao (capture by value) của packet
    // Nhờ đó, Dijkstra chỉ cần gọi w(e) mà không cần biết packet hay QoS là gì
    // Đảm bảo nguyên lý Separation of Concerns (Tách biệt thuật toán và nghiệp vụ)
    return [packet](const Edge& e) {
        return calculateRoutingCost(packet, e);
    };
}
```

---

## 5. Task B4.1 & B4.2 – Chụp ảnh trạng thái mạng (`snapshotNetworkState`)

* **File:** [include/Simulator.h](file:///d:/PBL2/Project_PBL2/include/Simulator.h) & [src/Simulator.cpp](file:///d:/PBL2/Project_PBL2/src/Simulator.cpp)
* **Ý nghĩa:** Cung cấp ảnh chụp đóng băng trạng thái của toàn bộ các cạnh tại một thời điểm, giúp việc tính toán định tuyến nhất quán.

```cpp
// Cấu trúc lưu ảnh chụp trạng thái mạng tại một thời điểm
struct NetworkSnapshot {
    std::unordered_map<int, bool> isUp;         // Bản sao trạng thái sống/chết của từng cạnh
    std::unordered_map<int, double> load;       // Bản sao mức tải hiện thời của từng cạnh
    std::unordered_map<int, double> queueDelay; // Bản sao độ trễ hàng đợi của từng cạnh
};

// Hàm sao chép nhanh trạng thái hiện thời của Graph vào NetworkSnapshot
NetworkSnapshot Simulator::snapshotNetworkState(const Graph& graph) {
    NetworkSnapshot snap;
    
    // Duyệt qua tất cả ID cạnh có trong đồ thị
    for (int edgeId : graph.edgeIds()) {
        const Edge& edge = graph.getEdge(edgeId);
        
        // Lưu độc lập vào từng bảng băm (hash map) theo edgeId
        snap.isUp[edgeId] = edge.isUp;
        snap.load[edgeId] = edge.currentLoad;
        snap.queueDelay[edgeId] = edge.queueDelay;
    }
    
    return snap; // Trả về snapshot độc lập, thay đổi trên graph sau đó không ảnh hưởng snapshot này
}
```

---

## 6. Task B4.3 & B4.4 – Kịch bản sự kiện và thực thi sự kiện (`applyEvent`)

* **File:** [src/Simulator.cpp](file:///d:/PBL2/Project_PBL2/src/Simulator.cpp)
* **Ý nghĩa:** Tác động các biến động động (tải, trễ hàng đợi, sự cố đứt nối cáp) vào đồ thị theo dòng thời gian.

```cpp
void Simulator::applyEvent(Graph& graph, const ScenarioEvent& event) {
    try {
        // Truy xuất cạnh mục tiêu theo targetId được chỉ định trong file kịch bản
        Edge& targetEdge = graph.getEdge(event.targetId);

        // Xử lý biến đổi thông số tương ứng theo từng loại sự kiện
        switch (event.eventType) {
            case EventType::LOAD_UPDATE:
                // Cập nhật mức tải mới, chặn biên chặt chẽ: tải chỉ được phép nằm trong đoạn [0.0, 1.0]
                targetEdge.currentLoad = std::max(0.0, std::min(1.0, event.value));
                break;

            case EventType::DELAY_UPDATE:
                // Cập nhật độ trễ hàng đợi mới, chặn biên không cho phép giá trị âm
                targetEdge.queueDelay = std::max(0.0, event.value);
                break;

            case EventType::LINK_DOWN:
                // Đánh dấu dây cáp bị đứt hoặc cổng thiết bị bị tắt
                targetEdge.isUp = false;
                break;

            case EventType::LINK_UP:
                // Đánh dấu dây cáp đã được sửa chữa và thông tuyến trở lại
                targetEdge.isUp = true;
                break;

            default:
                break;
        }
    } catch (const std::exception& e) {
        // Nếu kịch bản truyền sai ID cạnh không tồn tại, bắt lỗi để không làm sập chương trình
    }
}
```

---

## 7. Task B4.5 – Quá trình truyền gói tin (`sendPacket`)

* **File:** [src/Simulator.cpp](file:///d:/PBL2/Project_PBL2/src/Simulator.cpp)
* **Ý nghĩa:** Mô phỏng quá trình gói tin di chuyển dọc theo đường dẫn tìm được bởi Dijkstra, kiểm tra lỗi MTU, tính tổng độ trễ 4 thành phần và kích hoạt tăng tải trên các cạnh đi qua.

```cpp
SimulationResult Simulator::sendPacket(const Packet& packet, const PathResult& path, Graph& graph) {
    // Khởi tạo kết quả mặc định: Thành công, trễ ban đầu = 0, số mảnh ban đầu = 1
    SimulationResult result = {true, 0.0, "", 1};
    
    // 1. Kiểm tra tính liên thông: Nếu Dijkstra báo không có đường đi đến đích
    if (!path.reachable || path.edges.empty()) {
        result.success = false;
        result.failReason = "NO_ROUTE_AVAILABLE"; // Gói tin bị hủy (DROP) do không tìm thấy đường
        return result;
    }

    // 2. Lần lượt đi qua từng cạnh trên hành trình
    for (size_t i = 0; i < path.edges.size(); ++i) {
        int edgeId = path.edges[i];
        try {
            Edge& edge = graph.getEdge(edgeId);
            
            // Xác định node kế tiếp để tính Processing Delay (trễ xử lý tại router nhận)
            int nextNodeId = (i + 1 < path.nodes.size()) ? path.nodes[i + 1] : graph.otherEndpoint(path.nodes[i], edgeId);
            const Node& nextNode = graph.getNode(nextNodeId);

            // Kiểm tra MTU & phân mảnh trên cạnh này
            int fragments = DelayModel::calcFragmentCount(packet.sizeBytes, edge.mtu, packet.canFragment);
            if (fragments == 0) { 
                // Cờ DF bật mà kích thước gói vượt MTU -> Hủy gói ngay lập tức
                result.success = false;
                result.failReason = "DF_BLOCKED_BY_MTU_AT_EDGE_" + std::to_string(edgeId);
                return result; 
            }
            // Ghi nhận số mảnh lớn nhất phát sinh trên toàn tuyến
            result.fragmentCount = std::max(result.fragmentCount, fragments);

            // Tính đầy đủ 4 thành phần trễ (Trans + Prop + Proc + Queue) qua DelayModel
            double edgeDelay = DelayModel::calculateTotalDelay(packet, edge, nextNode);
            result.totalDelay += edgeDelay; // Cộng dồn vào tổng thời gian truyền

            // Tính thời gian truyền dTrans trên cạnh này để làm cơ sở tăng tải
            double dTrans = DelayModel::calcTransmissionDelay(packet.sizeBytes, edge.bandwidthMbps);
            
            // 3. Tác động làm tăng tải cho cạnh vừa truyền gói tin qua
            updateNetworkState(edge, dTrans);

        } catch (const std::exception& e) {
            result.success = false;
            result.failReason = "EDGE_NOT_FOUND_ERROR";
            return result;
        }
    }

    // 4. Nếu là giao thức TCP: Cộng thêm chi phí bắt tay 3 bước (Handshake overhead tính theo RTT)
    if (packet.protocol == Protocol::TCP) {
        // TCP overhead chỉ cộng 1 lần duy nhất trên toàn bộ hành trình
        result.totalDelay += DelayModel::calcProtocolOverhead(packet, result.totalDelay);
    }

    return result; // Trả về kết quả truyền gói tin thành công
}
```

---

## 8. Task B4.6 – Cập nhật và tiêu hao tải mạng (`updateNetworkState`)

* **File:** [src/Simulator.cpp](file:///d:/PBL2/Project_PBL2/src/Simulator.cpp)
* **Ý nghĩa:** Cập nhật tỷ lệ tải `currentLoad` tăng lên khi có gói tin chiếm dụng băng thông và mô phỏng cơ chế nguội dần (decay) tự nhiên của lưu lượng.

```cpp
void Simulator::updateNetworkState(Edge& edge, double dTrans) {
    // Khung cửa sổ thời gian quan trắc tải (1.0 giây)
    double loadWindow = 1.0; 
    // Hệ số suy giảm tự nhiên (decay 1% sau mỗi chu kỳ để đường truyền dần rảnh rỗi trở lại)
    double loadDecay = 0.99;
    
    // Tải tăng thêm tỷ lệ thuận với thời gian truyền gói tin dTrans
    // loadDelta = dTrans / LOAD_WINDOW_SECONDS
    double loadDelta = dTrans / loadWindow; 
    
    // Cộng dồn tải mới và khống chế chặt chẽ trong khoảng [0.0, 1.0] (tối đa 100% băng thông)
    edge.currentLoad = std::max(0.0, std::min(1.0, edge.currentLoad + loadDelta));
    
    // Áp dụng độ suy hao (Decay): Giúp đường truyền không bị kẹt ở mức 100% mãi mãi
    edge.currentLoad *= loadDecay;
}
```

---

## 9. Tóm tắt kết quả kiểm thử Tuần 4
* `tests/test_routing_cost.cpp`: 19/19 bài test PASSED. Đảm bảo chính xác từng mốc penalty, bắt biên INF/NaN, kiểm chứng gói `REAL_TIME` né đường nghẽn thành công so với `BULK_DATA`.
* `tests/test_simulator/test_simulator.cpp`: Tất cả PASSED. Đảm bảo tính độc lập của Snapshot, bắt đúng lỗi MTU `DF_BLOCKED_BY_MTU`, và xác nhận cơ chế tăng tải hoạt động chính xác.
