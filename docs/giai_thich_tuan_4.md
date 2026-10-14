# TÀI LIỆU GIẢI THÍCH CHI TIẾT TUẦN 4
**Chủ đề:** QoS + Congestion Penalty + Dynamic Cost + Simulation (Mô phỏng mạng động)

---

## 🌟 1. TỔNG QUAN TUẦN 4

Nếu như **Tuần 1-3** xây dựng nên hạ tầng tĩnh (Graph, Kruskal, Dijkstra, công thức Delay đơn lẻ), thì **Tuần 4** là bước ngoặt đưa mạng vào hoạt động **động (dynamic)**:
- **Tải trọng thay đổi theo thời gian thực:** Cáp không còn rảnh rỗi mà có lưu lượng tải (`currentLoad`) tăng/giảm theo từng gói tin đi qua.
- **Hệ số phạt tắc nghẽn (Congestion Penalty):** Khi tải vượt ngưỡng an toàn ($> 0.80$), độ trễ hàng đợi bị khuếch đại phi tuyến tính.
- **Định tuyến thích ứng QoS (Quality of Service):** Gói `REAL_TIME` (thoại, video) và `BULK_DATA` (tải file) có tiêu chí chọn đường hoàn toàn khác nhau.
- **Bộ máy mô phỏng (Simulator):** Cho phép chụp ảnh mạng (`Snapshot`), phát sự kiện (`Event`), truyền gói tin (`sendPacket`) và cập nhật tải (`updateNetworkState`).

```text
Luồng dữ liệu Tuần 4:
Packet -> Snapshot -> RoutingCost (QoS + Penalty) -> Dijkstra -> Simulator::sendPacket -> updateNetworkState -> Route thích ứng
```

---

## 👨‍💻 2. PHẦN VIỆC CỦA BẠN QUYẾN (Task A4.1 – A4.4)

### A. Task A4.1: Hệ số phạt tắc nghẽn (`congestionPenalty`)
* **File:** [include/RoutingCost.h](file:///d:/PBL2/Project_PBL2/include/RoutingCost.h) & [src/RoutingCost.cpp](file:///d:/PBL2/Project_PBL2/src/RoutingCost.cpp)
* **Ý nghĩa thực tế:** Trong mạng viễn thông, khi hàng đợi router đầy quá 80%, hiện tượng nghẽn (bufferbloat) xuất hiện làm trễ hàng đợi tăng vọt.
* **Mô hình tính toán:**
  $$\text{Penalty}(load) = \begin{cases} 
  1.00 & \text{khi } load \le 0.80 \\
  1.56 & \text{khi } 0.80 < load \le 0.85 \\
  3.25 & \text{khi } 0.85 < load \le 0.90 \\
  6.06 & \text{khi } 0.90 < load \le 0.95 \\
  10.00 & \text{khi } 0.95 < load \le 1.00 
  \end{cases}$$
* **Nguyên tắc DRY:** Hàm `DelayModel::calcQueueDelay()` của Ý gọi trực tiếp hàm này để không nhân bản logic tính penalty ở hai nơi.

### B. Task A4.2: Phân loại chất lượng dịch vụ QoS
* **File:** [include/Config.h](file:///d:/PBL2/Project_PBL2/include/Config.h)
* **Bộ trọng số được chuẩn hóa thành struct `QoSWeight`:**
  - `QOS_WEIGHT_REAL_TIME = {wTrans: 1.0, wProp: 1.0, wQueue: 4.0}`: Trọng số hàng đợi cực cao ($w_{queue} = 4.0$), sẵn sàng đi vòng đường xa miễn là đường vắng, không giật lag.
  - `QOS_WEIGHT_BULK_DATA = {wTrans: 2.0, wProp: 1.0, wQueue: 2.0}`: Quan tâm băng thông cao ($w_{trans} = 2.0$), chấp nhận độ trễ hàng đợi vừa phải.
  - `QOS_WEIGHT_CONTROL = {wTrans: 1.0, wProp: 1.0, wQueue: 1.0}`: Gói tin điều khiển, trung tính.
* **Cấu hình độc lập:** Tất cả trọng số đặt trong `Config.h`, tuyệt đối không hard-code số trong code C++.

### C. Task A4.3: Chi phí định tuyến động (`calculateRoutingCost`)
* **Công thức tổng hợp:**
  $$\text{Cost} = w_{trans} \cdot d_{trans} + w_{prop} \cdot d_{prop} + w_{queue} \cdot d_{queue} \cdot \text{Penalty}(load) + \text{FragmentPenalty}$$
* **Xử lý biên an toàn (Defensive Programming):**
  - Cáp bị đứt (`!edge.isUp`) $\to$ Trả về `Config::INF`.
  - Gói tin cấm phân mảnh (`canFragment = false`) mà vượt MTU $\to$ Trả về `Config::INF`.
  - Kiểm tra `std::isnan(cost)` để không bao giờ để giá trị `NaN` lọt vào làm hỏng Dijkstra.

### D. Task A4.4: Đóng gói tích hợp Dijkstra (`makeWeightFn`)
* **Ý nghĩa thiết kế:** Tạo một lambda function chuyển `calculateRoutingCost(packet, edge)` thành `WeightFn(const Edge&)` để truyền vào `dijkstra()`.
* **Lợi ích:** Thuật toán Dijkstra giữ nguyên tính tổng quát, không cần biết chi tiết gói tin hay QoS là gì (Separation of Concerns).

---

## 🕵️ 3. PHẦN VIỆC CỦA BẠN Ý (Task B4.1 – B4.7)

### A. Task B4.1 & B4.2: Ảnh chụp trạng thái mạng (`NetworkSnapshot`)
* **File:** [include/Simulator.h](file:///d:/PBL2/Project_PBL2/include/Simulator.h) & [src/Simulator.cpp](file:///d:/PBL2/Project_PBL2/src/Simulator.cpp)
* **Mục đích:** Khi một thuật toán tìm đường đang chạy, nếu mạng bị sự kiện làm thay đổi tải hoặc đứt cáp giữa chừng thì kết quả sẽ thiếu nhất quán.
* **Cách hoạt động:** Hàm `Simulator::snapshotNetworkState(graph)` sao chép toàn bộ `isUp`, `load`, `queueDelay` của các cạnh vào cấu trúc `NetworkSnapshot` độc lập.

### B. Task B4.3 & B4.4: Kịch bản sự kiện (`Scenario` & `applyEvent`)
* **Struct `ScenarioEvent`:** Lưu `time` (thời điểm xảy ra), `eventType` (`LOAD_UPDATE`, `DELAY_UPDATE`), `targetId` (cạnh bị tác động), `value` (giá trị mới).
* **Hàm `applyEvent()`:** Nhận sự kiện và áp dụng ngay lập tức vào đồ thị, có chặn lề bảo vệ dữ liệu ($0.0 \le load \le 1.0$ và $delay \ge 0.0$).

### C. Task B4.5: Quá trình truyền gói tin (`sendPacket`)
* **File:** [src/Simulator.cpp](file:///d:/PBL2/Project_PBL2/src/Simulator.cpp)
* **Luồng xử lý:**
  1. Kiểm tra tính liên thông (`path.reachable`).
  2. Duyệt qua từng cạnh trên đường đi, gọi `DelayModel::calcFragmentCount` kiểm tra MTU.
  3. Gọi `DelayModel::calculateTotalDelay` để cộng dồn trễ 4 thành phần (Transmission, Propagation, Processing, Queue).
  4. Sau khi đi qua cạnh, gọi `updateNetworkState` cập nhật tải của cạnh đó.
  5. Nếu giao thức là TCP, cộng thêm overhead bắt tay RTT một lần duy nhất.

### D. Task B4.6: Cập nhật tải mạng (`updateNetworkState`)
* **Công thức tăng tải:**
  $$\Delta Load = \frac{d_{trans}}{\text{LOAD\_WINDOW\_SECONDS}}$$
  $$Load_{new} = \min(1.0, Load_{old} + \Delta Load)$$
* **Hao mòn tự nhiên (Decay):** $Load = Load \times 0.99$ mô phỏng lưu lượng nguội dần khi không có gói tin đi qua.

### E. Task B4.7: Bộ dữ liệu kịch bản (`data/scenarios/`)
* [congestion.txt](file:///d:/PBL2/Project_PBL2/data/scenarios/congestion.txt): Tăng dần tải một cạnh từ $0.40 \to 0.85 \to 0.95 \to 1.00$.
* [qos.txt](file:///d:/PBL2/Project_PBL2/data/scenarios/qos.txt): Đẩy thời gian hàng đợi lên cao để gói REAL_TIME đổi hướng né trễ.
* [mtu.txt](file:///d:/PBL2/Project_PBL2/data/scenarios/mtu.txt): Kịch bản kiểm thử phân mảnh gói tin lớn.

---

## 🧪 4. KẾT QUẢ KIỂM THỬ TUẦN 4

Dự án có 2 file kiểm thử tự động độc lập:
1. `tests/test_routing_cost.cpp`: 19/19 test passed (Kiểm tra penalty mốc, trường hợp biên INF/NaN, khác biệt QoS giữa REAL_TIME và BULK_DATA, Dijkstra đổi đường).
2. `tests/test_simulator/test_simulator.cpp`: Tất cả test passed (Kiểm tra Snapshot độc lập, applyEvent chặn biên, sendPacket thành công, bắt lỗi MTU và NO_ROUTE).
