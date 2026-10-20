# KIẾN TRÚC HỆ THỐNG VÀ QUY TRÌNH 11 BƯỚC (TASK B6.6)
**Dự án:** Thiết kế & Mô phỏng Hệ thống Mạng Viễn thông (PBL2)

---

## 1. TỔNG QUAN KIẾN TRÚC PHÂN LỚP (LAYERED ARCHITECTURE)

Hệ thống được tổ chức theo kiến trúc phân tầng dạng module độc lập (Separation of Concerns), tuân thủ chặt chẽ nguyên lý Single Responsibility và DRY:

```text
+-----------------------------------------------------------------------+
|                           PRESENTATION LAYER                          |
|             main.cpp (Menu Orchestrator) | Reporter | Logger          |
+-----------------------------------------------------------------------+
                                   |
                                   v
+-----------------------------------------------------------------------+
|                            SIMULATION LAYER                           |
|       Simulator (Pipeline, Snapshot, Event Apply, Packet Transmit)     |
+-----------------------------------------------------------------------+
                                   |
                                   v
+-----------------------------------------------------------------------+
|                             ROUTING LAYER                             |
|       Dijkstra | RoutingCost (QoS & Congestion Penalty) | DelayModel   |
+-----------------------------------------------------------------------+
                                   |
                                   v
+-----------------------------------------------------------------------+
|                            TOPOLOGY LAYER                             |
|       Kruskal (MST) | Backup Link Selection | Reliability (Failover)  |
+-----------------------------------------------------------------------+
                                   |
                                   v
+-----------------------------------------------------------------------+
|                           CORE DATA LAYER                             |
|       Graph | Node | Edge | Packet | CostModel | IOManager | DSU      |
+-----------------------------------------------------------------------+
```

### Chi tiết vai trò từng tầng:
1. **Presentation Layer:**
   - `main.cpp`: Bộ điều phối chính, cung cấp giao diện console menu trực quan [0-8], gọi các module mà không chứa logic thuật toán bên trong.
   - `Reporter`: Định dạng và xuất báo cáo quy hoạch cáp (MST), bảng định tuyến toàn mạng, báo cáo độ trễ gói tin và báo cáo xử lý sự cố.
   - `Logger`: Tự động xuất toàn bộ dòng sự kiện và lịch sử truyền gói tin ra file văn bản chuẩn `.txt` (`data/logs/log_<scenario>.txt`).
2. **Simulation Layer:**
   - `Simulator`: Quản lý thời gian ảo (discrete simulation), nhận diện các sự kiện động (`LINK_DOWN`, `LINK_UP`, `LOAD_UPDATE`, `DELAY_UPDATE`), chụp snapshot trạng thái mạng và tiến hành truyền gói tin.
3. **Routing Layer:**
   - `Dijkstra`: Tìm kiếm đường đi ngắn nhất theo hàm trọng số tùy biến.
   - `RoutingCost`: Cung cấp hàm trọng số động kết hợp QoS (độ ưu tiên dịch vụ) và phạt tắc nghẽn (Congestion Penalty).
   - `DelayModel`: Tính toán 4 thành phần trễ chuẩn viễn thông (Truyền dẫn, Lan truyền, Xử lý, Hàng đợi) cùng độ trễ giao thức TCP (3-way handshake).
4. **Topology Layer:**
   - `Kruskal`: Xây dựng cây khung nhỏ nhất (MST) tối ưu hóa chi phí lắp đặt cáp viễn thông.
   - `Backup Link Selection & Reliability`: Phân tích thành phần liên thông khi đứt cạnh cây khung, tự động chọn cáp dự phòng tốt nhất kết nối hai nhánh.
5. **Core Data Layer:**
   - Đồ thị danh sách kề `Graph`, cấu trúc dữ liệu `DSU` (Disjoint Set Union với path compression và rank union), nạp dữ liệu và kiểm tra ràng buộc `IOManager` & `Validation`.

---

## 2. QUY TRÌNH HOẠT ĐỘNG 11 BƯỚC (11-STEP FLOW)

Toàn bộ vòng đời xử lý của hệ thống từ lúc khởi chạy cho đến khi kết thúc mô phỏng diễn ra tuần tự qua 11 bước tiêu chuẩn:

```text
[B1: Load Data] -> [B2: Validate] -> [B3: Build MST] -> [B4: Select Backup]
       |
       v
[B5: Init Routing] -> [B6: Interactive Menu] -> [B7: Load Scenario]
       |
       v
[B8: Group Events] -> [B9: Apply & Snapshot] -> [B10: Reroute & Transmit]
       |
       v
[B11: Report & Log]
```

### Mô tả chi tiết từng bước:
1. **Bước 1 (Load Core Data):**
   - Đọc dữ liệu từ `data/nodes.txt`, `data/edges.txt`, `data/packets.txt`, và `data/pricing.txt` thông qua `IOManager`.
2. **Bước 2 (Validation):**
   - Module `Validation` kiểm tra tính hợp lệ của từng bản ghi:
     - Node: ID duy nhất, số cổng $>0$, thời gian xử lý $\ge 0$.
     - Edge: Hai đầu nút $u, v$ tồn tại, chiều dài $>0$, băng thông $>0$, MTU $>0$. Loại bỏ ngay các cạnh vi phạm hoặc không khả thi về mặt vật lý (vượt quá khoảng cách tối đa của môi trường truyền dẫn).
     - Packet: Kích thước $>0$, node nguồn và đích tồn tại trong đồ thị.
3. **Bước 3 (Build MST - Quy hoạch mạng chính):**
   - Thuật toán `Kruskal` sử dụng cấu trúc `DSU` để tìm tập hợp $V-1$ cạnh tạo thành Cây khung có tổng chi phí lắp đặt thấp nhất. Đánh dấu `edge.isBuilt = true`.
4. **Bước 4 (Select Backup Links - Quy hoạch cáp dự phòng):**
   - Thuật toán duyệt qua các cạnh ngoài cây khung (Non-tree edges), lựa chọn $K$ cạnh có chi phí hợp lý nhất để làm đường dự phòng. Đánh dấu `edge.isBuilt = true` và `edge.isBackup = true`.
5. **Bước 5 (Initial Routing Tables):**
   - Dùng thuật toán `Dijkstra` tính toán bảng định tuyến (Routing Tables) cho mọi cặp nút $(u, v)$ trong mạng trên hạ tầng đã xây dựng (`onlyBuilt = true`).
6. **Bước 6 (Interactive Menu):**
   - Hiển thị menu điều hướng 9 chức năng `[0-8]` cho phép người dùng chọn xem báo cáo quy hoạch, bảng định tuyến hoặc chọn chạy 1 trong 6 kịch bản mô phỏng.
7. **Bước 7 (Load Scenario & Network Reset):**
   - Khi chọn kịch bản, hệ thống sao chép đồ thị sạch ban đầu (`Graph testGraph = baseGraph;`) để đảm bảo các kịch bản độc lập tuyệt đối, không bị lưu vết trạng thái cũ. Đọc kịch bản sự kiện từ `data/scenarios/<scenario>.txt`.
8. **Bước 8 (Group Events by Timestamp):**
   - Bộ mô phỏng `Simulator` nhóm các sự kiện xảy ra cùng thời điểm $t$ lại với nhau để tối ưu hóa việc in ấn và tránh tính toán trùng lặp.
9. **Bước 9 (Apply Events & State Snapshot):**
   - Cập nhật trạng thái mạng tại mốc thời gian $t$:
     - `LINK_DOWN` / `LINK_UP`: Bật/tắt cờ `isUp` của liên kết.
     - `LOAD_UPDATE`: Thay đổi tải trọng `currentLoad` của cáp.
     - `DELAY_UPDATE`: Cập nhật thời gian chờ hàng đợi `queueDelay`.
   - Chụp `NetworkSnapshot` đóng băng trạng thái mạng tại mốc $t$.
10. **Bước 10 (Dynamic Routing & Packet Transmission):**
    - Với mỗi gói tin trong danh sách:
      - Thuật toán Dijkstra tìm tuyến đường tốt nhất dựa trên hàm trọng số của gói tin (QoS + Congestion Penalty).
      - Phát hiện sự thay đổi lộ trình so với mốc thời gian trước đó để đánh dấu cờ `[DA DOI DUONG (REROUTE)]`.
      - Hàm `Simulator::sendPacket()` duyệt qua từng chặng: kiểm tra MTU, tính toán số mảnh phân đoạn, cộng dồn độ trễ lan truyền, truyền dẫn, hàng đợi và xử lý của router.
      - Nếu gói tin là TCP, cộng thêm thời gian thiết lập kết nối (3-way handshake).
11. **Bước 11 (Report & Logging):**
    - Xuất kết quả chi tiết ra màn hình console (Path, Hop, Delay, Fragment, Status, Failure).
    - Tự động gọi `Logger::exportSimulationLog()` ghi nhận toàn bộ lịch sử ra file `data/logs/log_<scenario>.txt`.
