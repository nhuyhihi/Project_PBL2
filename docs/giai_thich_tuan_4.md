# GIẢI THÍCH CHI TIẾT NHIỆM VỤ TUẦN 4
**Chủ đề: QoS + Congestion + Simulation (Mô phỏng động & Trạng thái)**

Tuần 4 đánh dấu sự chuyển đổi của dự án từ một **đồ thị tĩnh** (chỉ có các Node và Edge đứng yên) sang một **hệ thống mạng động**, nơi gói tin di chuyển, sự kiện xảy ra, và tải (Load) của dây cáp thay đổi liên tục.

Dưới đây là giải thích chi tiết về các file mới đã được tạo và cách chúng hoạt động cùng nhau.

---

## 1. File `include/Scenario.h` - Định nghĩa sự kiện mạng

**Vấn đề:** Để mạng trở nên "động", chúng ta cần có các sự kiện xảy ra theo thời gian (ví dụ: tự nhiên cáp bị nghẽn, hoặc thay đổi thông số delay của một cáp nào đó).

**Giải pháp:** Tạo ra cấu trúc dữ liệu để lưu các sự kiện này.
*   **`ScenarioEvent`:** Là một struct đại diện cho một sự kiện đơn lẻ. Nó lưu 4 thông số:
    *   `eventType`: Loại sự kiện (VD: `LOAD_UPDATE` - Cập nhật tải, `DELAY_UPDATE` - Cập nhật trễ).
    *   `targetId`: Áp dụng sự kiện này lên dây cáp (Edge) nào?
    *   `value`: Giá trị thay đổi là bao nhiêu?
    *   `time`: Sự kiện này xảy ra ở giây thứ mấy?
*   **`Scenario`:** Là một kịch bản hoàn chỉnh, gồm Tên kịch bản và một mảng (vector) chứa hàng loạt các `ScenarioEvent` sẽ xảy ra.

---

## 2. File `include/Simulator.h` và `src/Simulator.cpp` - Bộ não mô phỏng
Đây là nơi chứa toàn bộ logic điều phối của Tuần 4. File này gồm các thành phần chính sau:

### A. `NetworkSnapshot` và `snapshotNetworkState()` (Task B4.1 & B4.2)
*   **Chức năng:** Chụp "ảnh" lại trạng thái của mạng (`isUp`, `load`, `queueDelay`) tại một mili-giây cụ thể và lưu vào các `unordered_map`.
*   **Tại sao cần nó?** Trong lúc thuật toán Dijkstra của Quyến đang chạy để dò đường, hệ thống mô phỏng có thể bất ngờ nhận được các gói tin mới làm thay đổi `load`. Nếu `load` thay đổi ngay lúc Dijkstra đang tính toán, kết quả sẽ bị sai lệch (bị rác dữ liệu). Vì vậy, Dijkstra sẽ không dùng `Graph` trực tiếp, mà sẽ được cung cấp một bản `NetworkSnapshot` (đứng yên) để tính toán cho an toàn.

### B. `applyEvent()` (Task B4.4)
*   **Chức năng:** Dùng để biến các `ScenarioEvent` (đã định nghĩa ở mục 1) thành hiện thực.
*   **Hoạt động:** Hàm này nhận vào một sự kiện. Nếu là `LOAD_UPDATE`, nó tìm đúng cái dây cáp (`targetId`) và gán thẳng mức tải mới (`event.value`) vào `edge.currentLoad`. Có sử dụng `std::max/min` để chặn lỗi dữ liệu âm hoặc quá 1.0.

### C. `sendPacket()` (Task B4.5)
*   **Chức năng:** Đây là hàm "nhạc trưởng", mô phỏng toàn bộ hành trình của gói tin từ đầu này sang đầu kia.
*   **Hoạt động:**
    1.  Duyệt qua từng đoạn cáp mà gói tin đi qua.
    2.  Kiểm tra xem ống (MTU) có đủ to không. Nếu to quá không chui lọt mà cờ `canFragment = false` thì báo lỗi. Ngược lại thì chia nhỏ thành nhiều phần.
    3.  **Quy tắc DRY (Rất quan trọng):** Hàm này KHÔNG TỰ TÍNH toán học. Nó gọi thẳng hàm `DelayModel::calculateTotalDelay(...)` (đã làm ở tuần 3) để lấy tổng độ trễ của đoạn cáp.
    4.  Cộng dồn độ trễ lại. Nếu là giao thức TCP, cộng thêm một khoản phí RTT Overhead duy nhất ở bước cuối cùng.

### D. `updateNetworkState()` (Task B4.6)
*   **Chức năng:** Hiệu ứng vật lý - "Xe chạy qua thì đường phải mòn".
*   **Hoạt động:** Được gọi ngay bên trong vòng lặp của `sendPacket`. Mỗi khi gói tin đi qua xong một cạnh, hàm này sẽ lấy độ trễ truyền tải (`dTrans`) chia cho hằng số thời gian (`LOAD_WINDOW`) để ra lượng tải tăng thêm (`loadDelta`).
*   **Decay (Hao mòn):** Hệ thống có một cơ chế tự phục hồi, nhân Load với `0.99` để mô phỏng việc "khi không có gói tin nào qua, mạng sẽ từ từ thông thoáng trở lại".

---

## 3. Các file `.txt` trong `data/scenarios/` (Task B4.7)
Thay vì code cứng (hard-code) các sự kiện vào trong mã C++, chúng ta thiết kế hệ thống đọc kịch bản từ file ngoài. Việc này giúp Tuần 6 có thể test dễ dàng mà không phải build lại code.

1.  **`congestion.txt`:** Chứa các dòng lệnh nhồi tải (LOAD_UPDATE) lên một cạnh cụ thể. Dùng để test xem Dijkstra có thông minh né đường bị kẹt xe hay không.
2.  **`qos.txt`:** Chứa các dòng lệnh đẩy Delay lên cao. Dùng để chứng minh gói tin `REAL_TIME` sẽ sợ Delay và đổi hướng, còn `BULK_DATA` thì không.
3.  **`mtu.txt`:** File tĩnh không có sự kiện. Được dùng để duy trì luồng đọc file 11-bước chuẩn của hệ thống trong hàm `main()`, giữ cho kiến trúc hệ thống được sạch sẽ, không cần viết lệnh `if-else` ngớ ngẩn chỉ để bỏ qua bước đọc file.

---
**Tổng Kết:** Tuần 4 đóng vai trò ghép nối cái "Xác" (Graph tĩnh tuần 1,2) và các "Công thức" (DelayModel tuần 3) thành một guồng máy **chuyển động thực sự**, chuẩn bị cho việc xử lý đứt cáp ở Tuần 5.
