# TỔNG HỢP KẾT QUẢ 6 BỘ KỊCH BẢN DEMO (TASK B6.6)
**Dự án:** Thiết kế & Mô phỏng Hệ thống Mạng Viễn thông (PBL2)

---

## 1. TỔNG QUAN 6 KỊCH BẢN MÔ PHỎNG

Hệ thống cung cấp 6 bộ dữ liệu kịch bản chuẩn đặt trong `data/scenarios/`. Mỗi kịch bản có mục tiêu kiểm chứng riêng biệt và có thể chạy trực tiếp từ Menu tương tác `[1-6]` mà không cần biên dịch lại mã nguồn:

| Menu | Tên Kịch Bản | File Kịch Bản | File Nhật Ký Log | Mục Tiêu Kiểm Chứng |
| :---: | :--- | :--- | :--- | :--- |
| **[1]** | **BASIC** | `data/scenarios/basic.txt` | `data/logs/log_basic.txt` | Mạng hoạt động trong điều kiện lý tưởng, tải thấp đồng đều. |
| **[2]** | **QoS** | `data/scenarios/qos.txt` | `data/logs/log_qos.txt` | Phân hóa hành vi: REAL_TIME né độ trễ, BULK_DATA giữ đường. |
| **[3]** | **CONGESTION** | `data/scenarios/congestion.txt` | `data/logs/log_congestion.txt` | Tải vượt mốc 80%, hệ số phạt tăng vọt, tự động bẻ lái né đường đông. |
| **[4]** | **MTU** | `data/scenarios/mtu.txt` | `data/logs/log_mtu.txt` | Kiểm tra cơ chế chia nhỏ mảnh IP (Fragmentation) và cờ DF. |
| **[5]** | **FAILURE** | `data/scenarios/failure.txt` | `data/logs/log_failure.txt` | 7 ca sự cố: Đứt cáp nhánh ngoài, đứt đường chính, phục hồi, DROP cô lập. |
| **[6]** | **REDUNDANCY** | `data/scenarios/redundancy.txt` | `data/logs/log_redundancy.txt` | Cáp chính trên cây MST bị đứt toàn bộ $\to$ Kích hoạt cáp Backup cứu mạng. |

---

## 2. CHI TIẾT KẾT QUẢ CHẠY THỰC TẾ TỪNG KỊCH BẢN

### Kịch bản 1: BASIC (Môi trường lý tưởng)
- **Thiết lập:** Tải trên toàn mạng ở mức $10\%$.
- **Kết quả mô phỏng:**
  - `Packet #0 (REAL_TIME)`: Tuyến $0 \to 2 \to 3$, Độ trễ: $0.002256\text{s}$, Số mảnh: $1$, Trạng thái: `SUCCESS`.
  - `Packet #1 (BULK_DATA)`: Tuyến $0 \to 2 \to 3$, Độ trễ: $0.020427\text{s}$, Số mảnh: $3$, Trạng thái: `SUCCESS`.
- **Nhận xét:** Tuyến $0 \to 2 \to 3$ là tuyến cáp quang trực diện có băng thông $10\text{Gbps}$, tối ưu nhất cho cả hai luồng.

---

### Kịch bản 2: QoS (Chất lượng dịch vụ theo thời gian)
- **Thiết lập:** Tăng thời gian chờ hàng đợi (Queue Delay) trên Edge #2 lên $0.05\text{s}$ rồi $0.20\text{s}$.
- **Kết quả mô phỏng:**
  - Tại $t = 1.5\text{s}$ và $t = 3.0\text{s}$:
    - `Packet #0 (REAL_TIME)`: Nhận diện trễ tăng vọt $\to$ **Lập tức đổi đường [DA DOI DUONG (REROUTE)]** sang tuyến $0 \to 1 \to 3$ (Hops: 2, Delay: $0.002233\text{s}$).
    - `Packet #1 (BULK_DATA)`: Chỉ quan tâm đến băng thông lớn $\to$ **Kiên quyết giữ lộ trình cũ** $0 \to 2 \to 3$.
  - Tại $t = 4.5\text{s}$: Hàng đợi hạ xuống $0.002\text{s} \to$ `Packet #0` tự động quay trở lại tuyến tối ưu $0 \to 2 \to 3$.

---

### Kịch bản 3: CONGESTION (Chống tắc nghẽn giao thông mạng)
- **Thiết lập:** Đẩy tải trọng của Edge #1 từ $0.40 \to 0.85 \to 0.95 \to 1.00$.
- **Kết quả mô phỏng:**
  - Khi tải trọng Edge #1 vượt quá ngưỡng $0.80$, hàm phạt `CongestionPenalty` nhân hệ số mũ $\to$ Chi phí định tuyến qua Router 1 tăng đột ngột.
  - Các gói tin đang đi qua nhánh $0 \to 1 \to 3$ tự động reroute chuyển hướng sang nhánh $0 \to 2 \to 3$ để giải tỏa áp lực đường truyền.

---

### Kịch bản 4: MTU & Phân mảnh gói tin (Fragmentation)
- **Thiết lập:** Kích thước MTU của liên kết cáp quang là $1500\text{ bytes}$.
- **Kết quả mô phỏng:**
  - `Packet #0 (size = 500 bytes, DF = 1)`: Kích thước nhỏ hơn MTU $\to$ Không cần phân mảnh (`Frags: 1`), truyền thành công.
  - `Packet #1 (size = 3000 bytes, canFragment = 1)`: Kích thước vượt quá $1500\text{ bytes} \to$ Hệ thống tự động phân tách thành $3\text{ mảnh}$ (`Frags: 3`), truyền an toàn đến đích.
  - Nếu gói tin lớn hơn MTU mà bật cờ `DF = 0` (Don't Fragment) $\to$ Hệ thống trả về trạng thái `DROP (DF_BLOCKED_BY_MTU)`.

---

### Kịch bản 5: FAILURE & RECOVERY (Khắc phục sự cố đứt cáp)
- **Thiết lập:** Chuỗi sự kiện `LINK_DOWN` và `LINK_UP` theo dòng thời gian thực tế:
  - $t = 1.0\text{s}$: Đứt Edge #4 ngoài lộ trình $\to$ Tuyến $0 \to 2 \to 3$ giữ nguyên.
  - $t = 2.0\text{s}$: Đứt Edge #2 trên đường chính $\to$ Dijkstra tự động reroute sang $0 \to 1 \to 3$ thành công.
  - $t = 3.0\text{s}$: Phục hồi Edge #2 $\to$ Tuyến chính $0 \to 2 \to 3$ được khôi phục.
  - $t = 4.5\text{s}$: Đứt đồng thời cả Edge #2 và Edge #0 $\to$ Không còn lối thoát ra khỏi Router 0 $\to$ Thông báo `DROP (UNREACHABLE)`.
  - $t = 5.0\text{s}$: Phục hồi Edge #0 $\to$ Thoát khỏi tình trạng cô lập, chuyển sang $0 \to 1 \to 3$.
  - $t = 6.0\text{s}$: Đứt cạnh cầu Edge #6 (nối Host 4) $\to$ Host 4 mất kết nối hoàn toàn, luồng 0-3 không bị ảnh hưởng.
  - $t = 7.0\text{s}$: Phục hồi toàn mạng $\to$ Trở về trạng thái lý tưởng ban đầu.

---

### Kịch bản 6: REDUNDANCY (Vai trò sống còn của Cáp Dự Phòng)
- **Thiết lập:** Mô phỏng sự cố nghiêm trọng làm tê liệt toàn bộ các tuyến cáp thuộc cây khung MST nối hai nửa mạng:
  - $t = 1.0\text{s}$: Đứt Edge #1 (1-3) trên cây MST $\to$ Cáp dự phòng Backup #3 (2-3) lập tức được kích hoạt để thế chỗ.
  - $t = 2.0\text{s}$: Đứt tiếp Edge #0 (0-1) trên cây MST $\to$ Toàn bộ nhánh chính tê liệt, hệ thống dồn toàn bộ lưu lượng qua cặp cáp dự phòng Backup #2 (0-2) và Backup #3 (2-3) $\to$ Gói tin vẫn truyền thành công `SUCCESS`.
  - $t = 3.0\text{s}$: Các cáp chính được khắc phục $\to$ Hệ thống tự động chuyển lưu lượng về trạng thái cân bằng.
