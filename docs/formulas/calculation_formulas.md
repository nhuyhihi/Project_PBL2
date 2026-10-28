# TỔNG HỢP CÔNG THỨC TOÁN HỌC VÀ MÔ HÌNH TÍNH TOÁN (TASK B6.6)
**Dự án:** Thiết kế & Mô phỏng Hệ thống Mạng Viễn thông (PBL2)

---

## 1. MÔ HÌNH CHI PHÍ XÂY DỰNG MẠNG (COST MODEL)

Chi phí lắp đặt của một tuyến cáp $e = (u, v)$ được tính toán tổng hợp từ 4 thành phần thực tế:

$$\text{TotalCost}(e) = \text{CableCost} + \text{InstallCost} + \text{EquipCost} + \text{MaintCost}$$

Trong đó:
1. **Chi phí vật tư cáp (Cable Cost):**
   $$\text{CableCost} = \text{length} \times \text{unitPrice}(\text{mediaType})$$
2. **Chi phí thi công lắp đặt theo địa hình (Install Cost):**
   $$\text{InstallCost} = \text{length} \times \text{laborCost}(\text{mediaType}) \times \text{terrainMultiplier}$$
   - *Đồng bằng (FLAT):* hệ số $1.0$
   - *Đồi núi (MOUNTAIN):* hệ số $1.2$
   - *Đô thị phức tạp (URBAN):* hệ số $1.5$
3. **Chi phí thiết bị hai đầu liên kết (Equipment Cost):**
   $$\text{EquipCost} = \text{nodeEquip}(u) + \text{nodeEquip}(v)$$
4. **Chi phí bảo trì dự phòng (Maintenance Cost):**
   $$\text{MaintCost} = \text{fixedMaintenanceCost}$$

---

## 2. THUẬT TOÁN QUY HOẠCH CÂY KHUNG NHỎ NHẤT (KRUSKAL'S ALGORITHM)

- **Mục tiêu:** Tìm một đồ thị con liên thông không chu trình chứa toàn bộ $V$ nút mạng sao cho tổng chi phí xây dựng cáp là tối thiểu:
  $$\min \sum_{e \in T} \text{TotalCost}(e)$$
- **Độ phức tạp:** $\mathcal{O}(E \log E)$ sử dụng cấu trúc tập hợp rời rạc DSU (Disjoint Set Union):
  - Khởi tạo mỗi nút là một tập hợp riêng biệt.
  - Sắp xếp các cạnh hợp lệ theo chiều tăng dần của chi phí.
  - Với mỗi cạnh $e = (u, v)$, nếu $\text{Find}(u) \neq \text{Find}(v)$, gộp $\text{Union}(u, v)$ và đưa $e$ vào cây khung MST cho đến khi đủ $V - 1$ cạnh.

---

## 3. THUẬT TOÁN ĐỊNH TUYẾN NGẮN NHẤT (DIJKSTRA'S ALGORITHM)

- **Mục tiêu:** Tìm đường đi tối ưu từ nút nguồn $S$ tới nút đích $D$ trên đồ thị có trọng số không âm:
  $$\text{dist}[v] = \min_{(u, v) \in E} (\text{dist}[u] + w(u, v))$$
- **Hàng đợi ưu tiên (Min-Heap / Priority Queue):**
  - Trạng thái lưu: `(current_cost, u)`.
  - Độ phức tạp: $\mathcal{O}((V + E) \log V)$.
  - Trọng số $w(e)$ được tính toán linh động qua hàm trọng số `WeightFn` (kết hợp QoS, tải, trở ngại độ trễ).

---

## 4. MÔ HÌNH ĐỘ TRỄ VIỄN THÔNG (DELAY MODEL)

Độ trễ toàn phần trên mỗi chặng (hop) từ nút $u$ qua liên kết $e$ đến nút tiếp theo $v$ bao gồm 4 thành phần:

$$D_{\text{total}} = D_{\text{prop}} + D_{\text{trans}} + D_{\text{queue}} + D_{\text{proc}}$$

### A. Độ trễ lan truyền (Propagation Delay):
Là thời gian sóng tín hiệu truyền dọc theo sợi cáp vật lý:
$$D_{\text{prop}} = \frac{\text{length}}{v_{\text{prop}}}$$
Trong đó:
- *Cáp quang (FIBER):* $v_{\text{prop}} \approx 200,000\text{ km/s} = 2 \times 10^8\text{ m/s}$ (vận tốc ánh sáng trong lõi silica).
- *Cáp đồng (COPPER):* $v_{\text{prop}} \approx 230,000\text{ km/s} = 2.3 \times 10^8\text{ m/s}$.
- *Sóng không dây (WIRELESS):* $v_{\text{prop}} \approx 300,000\text{ km/s} = 3 \times 10^8\text{ m/s}$.

### B. Độ trễ truyền dẫn (Transmission Delay):
Là thời gian đưa toàn bộ các bit của gói tin lên đường truyền:
$$D_{\text{trans}} = \frac{\text{sizeBytes} \times 8}{\text{bandwidthMbps} \times 10^6}\text{ (giây)}$$

### C. Độ trễ hàng đợi (Queuing Delay):
Thời gian gói tin phải xếp hàng tại bộ đệm router trước khi được xử lý:
$$D_{\text{queue}} = \text{edge.queueDelay}$$

### D. Độ trễ xử lý (Processing Delay):
Thời gian CPU của router $v$ đọc header, tra bảng định tuyến và tính checksum:
$$D_{\text{proc}} = \text{node.processingTime}$$

---

## 5. MÔ HÌNH PHÂN MẢNH GÓI TIN (MTU & FRAGMENTATION)

Gói tin kích thước $L = \text{sizeBytes}$ đi qua liên kết có giới hạn $\text{MTU}$:
- Nếu $L \le \text{MTU}$: Số mảnh $\text{fragments} = 1$.
- Nếu $L > \text{MTU}$:
  - Nếu cờ Don't Fragment bật (`canFragment == false`): Hủy gói tin ngay lập tức $\to \text{DROP}$ với lỗi `DF_BLOCKED_BY_MTU`.
  - Nếu cho phép phân mảnh (`canFragment == true`):
    Mỗi mảnh mang payload tối đa là phần chia hết cho 8 (theo chuẩn IPv4/IPv6):
    $$\text{maxPayload} = \lfloor \frac{\text{MTU} - \text{IP\_HEADER\_SIZE}}{8} \rfloor \times 8$$
    Số lượng mảnh sinh ra:
    $$\text{fragmentCount} = \lceil \frac{L - \text{IP\_HEADER\_SIZE}}{\text{maxPayload}} \rceil$$

---

## 6. ĐỘ TRỄ GIAO THỨC (TCP vs UDP OVERHEAD)

- **Giao thức UDP (User Datagram Protocol):**
  - Không kết nối (connectionless), truyền tin trực tiếp $\to \text{Overhead} = 0$.
- **Giao thức TCP (Transmission Control Protocol):**
  - Đòi hỏi thiết lập phiên bắt tay 3 bước (3-way Handshake: SYN $\to$ SYN-ACK $\to$ ACK) nếu là kết nối mới (`pkt.newConnection == true`):
    $$\text{HandshakeDelay} = 1.5 \times \text{RTT}$$
    *(với $\text{RTT} \approx 2 \times D_{\text{total}}$)*.
  - Mỗi gói tin gửi đi phải chờ gói tin ACK phản hồi từ phía nhận:
    $$\text{AckDelay} = 0.5 \times \text{RTT} \approx D_{\text{total}}$$
  - Tổng phụ phí giao thức TCP:
    $$\text{TCP\_Overhead} = \begin{cases} 2.5 \times D_{\text{total}} & \text{khi newConnection = true} \\ 0.5 \times D_{\text{total}} & \text{khi kết nối đã thiết lập} \end{cases}$$

---

## 7. ĐỊNH TUYẾN THEO CHẤT LƯỢNG DỊCH VỤ VÀ PHẠT TẮC NGHẼN (QoS & CONGESTION)

Hàm trọng số tính toán chi phí cạnh $w(e)$ phục vụ Dijkstra được định nghĩa:

$$w(e) = \alpha \times \text{DelayCost}(e) + \beta \times \text{BandwidthCost}(e) + \text{CongestionPenalty}(e)$$

### A. Phân loại dịch vụ (QoS Priority):
1. **Gói tin thời gian thực (REAL_TIME: Thoại, Video call, Gaming):**
   - Rất nhạy cảm với trễ: $\alpha = 0.8$, $\beta = 0.2$. Ưu tiên tối đa đường có độ trễ nhỏ nhất.
2. **Gói tin truyền dữ liệu lớn (BULK_DATA: Tải file, Sao lưu đám mây):**
   - Đòi hỏi thông lượng cao: $\alpha = 0.1$, $\beta = 0.9$. Ưu tiên các tuyến cáp quang băng thông lớn.
3. **Gói tin điều khiển mạng (CONTROL: ICMP, BGP, OSPF):**
   - Cân bằng độ tin cậy: $\alpha = 0.5$, $\beta = 0.5$.

### B. Hệ số phạt tắc nghẽn (Congestion Penalty):
Khi tải trọng $U = \text{currentLoad}$ trên cáp tăng cao:
$$\text{Penalty}(U) = \begin{cases} 0 & \text{nếu } U < 0.5 \\ 10 \times (U - 0.5)^2 & \text{nếu } 0.5 \le U < 0.8 \\ 100 \times e^{5(U - 0.8)} & \text{nếu } U \ge 0.8 \text{ (Tắc nghẽn nghiêm trọng)} \end{cases}$$
Khi $U \ge 0.8$, chi phí trọng số tăng đột biến, ép thuật toán Dijkstra bẻ lái luồng dữ liệu sang đường vòng vắng tải hơn để bảo toàn chất lượng dịch vụ.

---

## 8. CƠ CHẾ CHỊU LỖI VÀ CHỌN CÁP DỰ PHÒNG (FAILURE & REDUNDANCY)

Khi một liên kết chính $e_f = (u, v)$ thuộc cây khung MST bị đứt (`isUp = false`):
1. Đồ thị cây bị phân tách thành 2 thành phần liên thông riêng biệt $V_A$ và $V_B$ ($u \in V_A, v \in V_B$).
2. Tập hợp các cạnh dự phòng ứng viên là tất cả các cạnh $e' = (x, y)$ thỏa mãn:
   $$x \in V_A \land y \in V_B \land e'.\text{isUp} = \text{true}$$
3. Lựa chọn cạnh thay thế tối ưu:
   $$e^* = \arg\min_{e' \in \text{Candidates}} \text{TotalCost}(e')$$
   *(Ưu tiên các cạnh đã được đánh dấu `isBackup = true` trong giai đoạn quy hoạch)*.
4. Nếu $\text{Candidates} = \emptyset$: Báo cáo cạnh đứt là Cạnh cầu nguy cấp (Critical Bridge) và thông báo mạng bị chia cắt không thể định tuyến.
