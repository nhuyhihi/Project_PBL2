# TÀI LIỆU GIẢI THÍCH CHI TIẾT TUẦN 5
**Chủ đề:** Failure Simulation + Backup Analysis + Rerouting + Failure Report (Xử lý sự cố & Khả năng chịu lỗi)

---

## 🌟 1. TỔNG QUAN TUẦN 5

Trong thực tế vận hành mạng viễn thông, sự cố đứt cáp hoặc thiết bị hỏng hóc là điều không thể tránh khỏi (do thời tiết, đào đường, quá tải thiết bị). Mục tiêu trọng tâm của **Tuần 5** là:
- **Mô phỏng sự cố mạng động:** Hỗ trợ các sự kiện đứt cáp (`LINK_DOWN`) và phục hồi cáp (`LINK_UP`) theo dòng thời gian.
- **Phân tích vai trò của Cáp Dự Phòng (Backup Analysis):** Khi một dây cáp trong cây khung chính (Tree Edge) bị đứt, hệ thống phải tự động tính toán xem **cáp dự phòng nào** sẽ đứng ra cứu mạng và giải thích rõ nguyên nhân.
- **Khả năng tự hồi phục (Self-healing & Reroute):** Thuật toán tự động phát hiện đường đi cũ bị hỏng để tìm đường mới qua cáp dự phòng. Nếu mạng bị cô lập hoàn toàn thì thông báo hủy gói (`DROP`) an toàn.
- **Báo cáo sự cố chuẩn hóa (Failure Report):** Trình bày trực quan luồng `OLD PATH -> FAILED EDGE -> NEW PATH`.

---

## 🕵️ 2. PHẦN VIỆC CỦA BẠN Ý (Task B5.1 – B5.3)

### A. Task B5.1: Kịch bản sự cố và phục hồi (`failure.txt`)
* **File kịch bản:** [data/scenarios/failure.txt](file:///d:/PBL2/Project_PBL2/data/scenarios/failure.txt)
* **File triển khai xử lý:** [src/Simulator.cpp](file:///d:/PBL2/Project_PBL2/src/Simulator.cpp) (hàm `Simulator::applyEvent`)
* **Cơ chế hoạt động:**
  - Bổ sung xử lý `EventType::LINK_DOWN`: gán `edge.isUp = false`.
  - Bổ sung xử lý `EventType::LINK_UP`: gán `edge.isUp = true`.
* **5 trường hợp sự cố (Test Cases) được thiết kế chi tiết:**
  1. **Case 1 (Link ngoài path DOWN):** Làm đứt Edge #4 (1-2), không nằm trên lộ trình gói tin $0 \to 3$ đang đi $\to$ Đường đi $0 \to 2 \to 3$ giữ nguyên, không reroute thừa thãi.
  2. **Case 2 (Link trên path DOWN):** Làm đứt Edge #2 (0-2) thuộc đường chính $\to$ Dijkstra phát hiện `isUp = false` $\to$ Trả về chi phí vô cực $\to$ Lập tức bẻ lái gói tin sang nhánh dự phòng $0 \to 1 \to 3$.
  3. **Case 3 (LINK_UP phục hồi):** Đóng lại Edge #2 $\to$ Hệ thống tự động nhận biết cáp đã thông $\to$ Tuyến tối ưu $0 \to 2 \to 3$ được tự động khôi phục.
  4. **Case 4 (Multiple failures - Đứt hàng loạt):** Đứt đồng thời cả Edge #2 và Edge #0 $\to$ Không còn lối thoát ra khỏi Node 0 $\to$ Gói tin bị `DROP` với lý do `NO_ROUTE_AVAILABLE`.
  5. **Case 5 (Cạnh cầu Bridge DOWN):** Cạnh #6 (0-4) là đường duy nhất nối tới Host 4. Khi cạnh này đứt, Host 4 bị cô lập hoàn toàn $\to$ Báo cáo rõ không thể kết nối.

---

### B. Task B5.2: Phân tích cáp dự phòng (`Backup Analysis`)
* **File:** [include/Reliability.h](file:///d:/PBL2/Project_PBL2/include/Reliability.h) & [src/Reliability.cpp](file:///d:/PBL2/Project_PBL2/src/Reliability.cpp)
* **Hàm chính:** `Reliability::findBackupForTreeEdge(graph, mstTreeEdges, failedTreeEdgeId)`
* **Thuật toán nâng cao theo quy chuẩn:**
  1. **Tách Graph khi giả định tree edge bị đứt:** 
     - Khi cạnh cây khung $e = (u, v)$ đứt, cây MST bị chia làm đúng 2 thành phần liên thông riêng biệt: Nhánh A chứa đỉnh $u$ (`compA`) và Nhánh B chứa đỉnh $v$ (`compB`).
     - Sử dụng hàm BFS/DFS `getComponentNodes` để xác định tập hợp tất cả các node thuộc `compA` và `compB`.
  2. **Tìm cạnh ngoài cây kết nối hai phía:**
     - Quét toàn bộ các cạnh còn lại trong đồ thị (không thuộc MST).
     - Kiểm tra điều kiện: Cáp phải đang hoạt động (`isUp = true`), thỏa mãn độ dài tối đa (`isStaticFeasible`), và có một đầu thuộc `compA`, một đầu thuộc `compB`.
  3. **Lựa chọn tối ưu & Giải thích nguyên nhân (`reason`):**
     - **Ưu tiên 1:** Các cạnh đã được quy hoạch xây dựng dự phòng trước đó (`isBuilt = true, isBackup = true`).
     - **Ưu tiên 2:** Chi phí lắp đặt thấp nhất.
     - **Cạnh cầu nguy cấp (Critical Bridge):** Nếu không tìm thấy bất kỳ dây nào nối được hai nhánh (như cạnh nối Host 1 cổng), thuật toán trả về `-1` và cảnh báo: *"Không tìm thấy cáp dự phòng (Cạnh cầu nguy cấp)"*.

---

### C. Task B5.3: Báo cáo sự cố & Khôi phục (`Failure Report`)
* **File:** [include/Reporter.h](file:///d:/PBL2/Project_PBL2/include/Reporter.h) & [src/Reporter.cpp](file:///d:/PBL2/Project_PBL2/src/Reporter.cpp)
* **Hàm thực hiện:**
  ```cpp
  void printFailureRecoveryReport(const Graph& graph,
                                  const Packet& pkt,
                                  const PathResult& oldPath,
                                  int failedEdgeId,
                                  const PathResult& newPath,
                                  const std::string& note);
  ```
* **Mẫu hiển thị chuẩn theo cấu trúc `OLD PATH -> FAILED EDGE -> NEW PATH`:**
  ```text
  ==============================================================
             BAO CAO SU CO & DINH TUYEN LAI (TASK B5.3)         
  ==============================================================
  Packet ID    : #1 (0 -> 3)
  OLD PATH     : 0 -> 2 -> 3 (Hops: 2, Cost: 0.008006)
  FAILED EDGE  : Edge #2 (0 - 2) bi LINK_DOWN
  NEW PATH     : 0 -> 1 -> 3 (Hops: 2, Cost: 0.008013)
  KET QUA      : [THANH CONG] Da tim duong thay the thanh cong.
  GHI CHU      : Duong chinh bi dut -> Kich hoat nhanh du phong qua Router 1
  ==============================================================
  ```
  *(Khi mất liên lạc hoàn toàn, dòng `KET QUA` sẽ in: `[DROP] Khong the dinh tuyen den dich (Bi co lap / Mat lien lac hoan toan)`).*

---

## 👨‍💻 3. PHẦN VIỆC CỦA BẠN QUYẾN (Task A5.1 – A5.5)

1. **Task A5.1 (`setLinkState`):** Hỗ trợ bật/tắt trạng thái cáp (`isUp`).
2. **Task A5.2 (`isConnected`):** Dùng thuật toán duyệt đồ thị (BFS hoặc DFS) kiểm tra toàn mạng có còn liên thông hay không (bỏ qua các cạnh có `isUp = false`).
3. **Task A5.3 (`rerouteIfNeeded`):** Kiểm tra xem trên đường đi cũ có cạnh nào bị đứt không. Nếu có thì mới chạy lại Dijkstra, nếu không thì giữ nguyên.
4. **Task A5.4 (`handleFailureEvent`):** Hàm điều phối cấp cao gom các bước: nhận sự cố $\to$ cập nhật trạng thái $\to$ reroute $\to$ xuất kết quả.
5. **Task A5.5 (Fault-tolerance metric):** Đo lường chỉ số độ tin cậy:
   $$\text{Tỷ lệ kết nối} = \frac{\text{Số cặp node còn liên lạc được}}{\text{Tổng số cặp node trong mạng}}$$
   So sánh mạng chỉ có MST so với mạng có MST + Backup.

---

## 🧪 4. DANH SÁCH TEST VÀ KẾT QUẢ ĐẠT ĐƯỢC

Các bài kiểm thử đã được chạy và vượt qua 100%:
- [tests/test_failure_scenario.cpp](file:///d:/PBL2/Project_PBL2/tests/test_failure_scenario.cpp): Kiểm tra trọn vẹn 5 ca sự cố trong `failure.txt`.
- [tests/test_backup_analysis.cpp](file:///d:/PBL2/Project_PBL2/tests/test_backup_analysis.cpp): Kiểm tra tách cây thành công, tìm đúng cáp backup #2 và #3, phát hiện đúng cạnh cầu Edge #6.
- [tests/test_failure_report.cpp](file:///d:/PBL2/Project_PBL2/tests/test_failure_report.cpp): Kiểm tra báo cáo in chuẩn đẹp cho cả 2 trường hợp Reroute thành công và DROP.
