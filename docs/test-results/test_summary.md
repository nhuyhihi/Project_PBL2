# BÁO CÁO KẾT QUẢ KIỂM THỬ TOÀN DIỆN (TASK B6.6)
**Dự án:** Thiết kế & Mô phỏng Hệ thống Mạng Viễn thông (PBL2)

---

## 1. TỔNG HỢP KẾT QUẢ KIỂM THỬ HỆ THỐNG

Hệ thống đã trải qua quá trình kiểm thử tự động trên toàn bộ các đơn vị mô đun (Unit Tests) và kiểm thử hồi quy (Regression Tests). Tất cả các bộ test đều **PASS 100%**, không xảy ra lỗi phân đoạn (Segmentation Fault), rò rỉ bộ nhớ hay xung đột kiểu dữ liệu.

| STT | Tên Bộ Kiểm Thử | File Kiểm Thử | Phạm vi kiểm tra | Trạng Thái | Chi tiết kết quả |
| :---: | :--- | :--- | :--- | :---: | :--- |
| 1 | **Validation & IO** | `tests/test_validation.cpp` | Kiểm tra tính hợp lệ dữ liệu đầu vào | **PASS** | Bắt và loại bỏ đúng các Node âm, Edge nối nút ảo, Packet âm. |
| 2 | **Graph & Kruskal** | `tests/test_graph_kruskal.cpp`| Đồ thị danh sách kề và MST Kruskal | **PASS** | Xây dựng MST thành công, không tạo chu trình, bảo đảm liên thông. |
| 3 | **Cost Model** | `tests/test_cost.cpp` | 4 thành phần chi phí vật tư & thi công | **PASS** | Tính chính xác chi phí cáp, nhân công theo địa hình, thiết bị & bảo trì. |
| 4 | **Delay Model** | `tests/test_delay.cpp` | 4 thành phần trễ viễn thông + TCP | **PASS** | Khớp công thức lan truyền, truyền dẫn, hàng đợi, xử lý và handshake. |
| 5 | **Dijkstra** | `tests/test_dijkstra.cpp` | Định tuyến ngắn nhất trên Min-Heap | **PASS** | **36/36 test cases đạt**, khôi phục đúng lộ trình, xử lý đa cạnh tốt. |
| 6 | **QoS & Routing Cost**| `tests/test_routing_cost.cpp`| Hàm trọng số thích ứng QoS & Tắc nghẽn | **PASS** | Phân luồng chính xác giữa REAL_TIME và BULK_DATA; phạt tải nặng $>80\%$. |
| 7 | **Simulator Core** | `tests/test_simulator/` | Pipeline mô phỏng và Snapshot | **PASS** | Đóng băng và cập nhật trạng thái mạng chính xác theo dòng thời gian. |
| 8 | **Failure Scenario** | `tests/test_failure_scenario.cpp`| Sự kiện đứt cáp và khôi phục liên kết | **PASS** | Xử lý hoàn hảo LINK_DOWN, LINK_UP và các trường hợp đa sự cố. |
| 9 | **Backup Analysis** | `tests/test_backup_analysis.cpp`| Tìm kiếm cáp dự phòng cứu mạng | **PASS** | Phân tách 2 nhánh liên thông, tìm đúng cáp Backup thế chỗ; cảnh báo Bridge. |
| 10 | **Reporter & Output**| `tests/test_reporter.cpp` | Định dạng báo cáo chuẩn đầu ra | **PASS** | In đầy đủ MST, Routing Table, Delay, Hop, Fragment, Failure Recovery. |

---

## 2. KẾT QUẢ CHI TIẾT CỦA CÁC MODULE TRỌNG TÂM

### A. Kiểm thử Định tuyến Dijkstra (`tests/test_dijkstra.cpp`):
```text
[PathResult co ban]
[Dijkstra weight=1 va weight=length]
[Canh song song]
[Relaxation cap nhat lai duong tot hon]
[Reconstruct path dung thu tu]
[Mang mau that (data/nodes.txt, data/edges.txt)]
REJECT Node ID 99: totalPorts không được âm
REJECT Edge ID 99: Node v (999) không tồn tại

Ket qua: 36 dat, 0 loi
>>> TAT CA CAC TEST DIJKSTRA DEU PASS! <<<
```

### B. Kiểm thử Phân tích Cáp Dự Phòng (`tests/test_backup_analysis.cpp`):
```text
=== BAT DAU KIEM THU TASK B5.2 (BACKUP ANALYSIS) ===
[CostModel] Nhan cau hinh gia thanh cong tu: data/pricing.txt
-> Danh sach canh cay MST: 6 7 4 0 1 
-> Danh sach cap Backup da xay: 2 3 
[PASS] Edge #1 (1-3) bi dut -> Tim thay cap du phong cuu nguy: Edge #3 
       (Ly do: Cap du phong da duoc xay dung noi hai nhanh 2 va 3)
[PASS] Edge #0 (0-1) bi dut -> Tim thay cap du phong cuu nguy: Edge #2 
       (Ly do: Cap du phong da duoc xay dung noi hai nhanh 0 va 2)
[PASS] Canh cau duy nhat Edge #6 bi dut -> Bao dung khong co backup 
       (Ly do: Canh cau nguy cap / Critical Bridge)
[PASS] findAllBackupsForTreeEdge tim duoc 2 ung vien cho Edge #1.

>>> CHUC MUNG! BAN DA PASS 100% YEU CAU CUA TASK B5.2! <<<
```

### C. Kiểm thử Nhập Xuất & Tính Hợp Lệ Dữ Liệu (`tests/test_validation.cpp`):
```text
========================================
 TEST MODULE VALIDATION & IO (File Input)
========================================
--- 1. Testing Node Loading & Validation ---
REJECT Node ID 99: totalPorts không được âm
=> Valid Node Count in Graph: 6 (Expected: 6)

--- 2. Testing Edge Loading & Validation ---
REJECT Edge ID 99: Node v (999) không tồn tại
=> Valid Edge Count in Graph: 9 (Expected: 9)

--- 3. Testing Packet Loading & Validation ---
REJECT Packet ID 2: sizeBytes không được âm
=> Valid Packet Count: 2 (Expected: 2)

>>> VALIDATION MODULE PASSED ALL TESTS! <<<
```

---

## 3. KẾT LUẬN KIỂM THỬ HỒI QUY (REGRESSION CONCLUSION)
- Toàn bộ các module cũ (từ Tuần 1 đến Tuần 5) tiếp tục hoạt động ổn định khi tích hợp thêm các chức năng mới của Tuần 6 (Menu Orchestrator, Reporter, Logger).
- Không xuất hiện tình trạng suy giảm hiệu năng (Performance Regression) hay lỗi phụ thuộc chéo (Circular Dependency).
