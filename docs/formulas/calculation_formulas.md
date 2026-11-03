# TỔNG HỢP CÔNG THỨC TOÁN HỌC VÀ MÔ HÌNH TÍNH TOÁN (TASK B6.6)
**Dự án:** Thiết kế & Mô phỏng Hệ thống Mạng Viễn thông (PBL2)  
**Trạng thái:** Đồng bộ 100% với mã nguồn thực tế trong `include/` và `src/`

---

## 1. MÔ HÌNH CHI PHÍ XÂY DỰNG MẠNG (`CostModel.h` / `CostModel.cpp`)

Chi phí lắp đặt của một tuyến cáp $e = (u, v)$ được tính toán tổng hợp từ 4 thành phần thực tế dựa trên bảng đơn giá `data/pricing.txt`:

$$\text{TotalCost}(e) = \begin{cases} \infty \text{ (Config::INF)} & \text{nếu } e.\text{isUp} = \text{false} \\ \text{install} + \text{material} + \text{equipment} + \text{maintenance} & \text{nếu } e.\text{isUp} = \text{true} \end{cases}$$

Trong đó (`CostBreakdown`):
1. **Chi phí nhân công lắp đặt cơ bản (`install`):**
   $$\text{install} = \text{INSTALLATION\_COST} \quad (\text{mặc định } 0.0\text{ ĐV trong pricing.txt})$$
2. **Chi phí vật tư cáp theo địa hình (`material`):**
   $$\text{material} = \text{length} \times \text{unitPrice}(\text{mediaType}) \times \text{terrainFactor}$$
   - Đơn giá vật tư $\text{unitPrice}$: Cáp đồng (`COPPER`) = $1.0\text{ ĐV/m}$; Cáp quang (`FIBER`) = $5.0\text{ ĐV/m}$; Không dây (`WIRELESS`) = $0.0\text{ ĐV/m}$.
   - Hệ số địa hình $\text{terrainFactor}$: Đồng bằng $= 1.0$; Đồi núi $= 1.2$; Đô thị phức tạp $= 1.5$.
3. **Chi phí thiết bị hai đầu liên kết (`equipment`):**
   $$\text{equipment} = \text{EQUIPMENT\_COST}(\text{mediaType})$$
   - `COPPER`: $50.0\text{ ĐV}$; `FIBER`: $50.0\text{ ĐV}$; `WIRELESS`: $80.0\text{ ĐV}$.
4. **Chi phí bảo trì định kỳ (`maintenance`):**
   $$\text{maintenance} = \text{MAINTENANCE\_COST}(\text{mediaType})$$
   - `COPPER`: $30.0\text{ ĐV}$; `FIBER`: $48.0\text{ ĐV}$; `WIRELESS`: $20.0\text{ ĐV}$.

---

## 2. THUẬT TOÁN QUY HOẠCH CÂY KHUNG NHỎ NHẤT CÓ RÀNG BUỘC (`Kruskal.cpp` / `DSU.cpp`)

- **Mục tiêu:** Tìm tập hợp $V - 1$ cạnh kết nối toàn bộ $V$ nút mạng thành một cây khung liên thông không chu trình với tổng chi phí lắp đặt nhỏ nhất:
  $$\min \sum_{e \in T} \text{TotalCost}(e)$$
- **Tiêu chuẩn sắp xếp cạnh (`compareEdges`):**
  - Ưu tiên $\text{TotalCost}(e)$ tăng dần; nếu chi phí bằng nhau thì ưu tiên `edge.id` nhỏ hơn (đảm bảo tính tất định - deterministic).
- **4 điều kiện ràng buộc khi xét chọn cạnh (`checkEdge`):**
  1. `e.isUp == true` (nếu đứt $\to$ loại với lý do `LINK_DOWN`).
  2. `e.length <= e.maxSegmentLength` (nếu vượt chiều dài tối đa của môi trường truyền $\to$ loại với lý do `TOO_LONG`).
  3. `dsu.connected(e.u, e.v) == false` (nếu đã cùng thành phần liên thông $\to$ loại với lý do `CYCLE`).
  4. `node(u).usedPorts < node(u).totalPorts` và `node(v).usedPorts < node(v).totalPorts` (nếu hết cổng vật lý $\to$ loại với lý do `NO_PORT`).
- **Độ phức tạp:** $\mathcal{O}(E \log E)$ sử dụng cấu trúc `DSU` có nén đường đi (*Path Compression*) và gộp theo hạng (*Union by Rank*).

---

## 3. THUẬT TOÁN CHỌN CÁP DỰ PHÒNG (`selectBackupLinks` & `Reliability.cpp`)

Sau khi dựng cây khung MST, hệ thống chọn tối đa $K$ cạnh ngoài cây (`!e.isBuilt && e.isUp && isStaticFeasible(e)`) theo thứ tự chi phí tăng dần:
- Sử dụng cấu trúc `TreeIndex` để xác định đường đi trên cây MST giữa hai đầu mút $(u, v)$ của cạnh ứng viên.
- Cạnh ứng viên được chọn làm **Cáp dự phòng (`isBackup = true, isBuilt = true`)** nếu cả hai đầu nút còn cổng trống và cạnh đó bảo vệ thêm ít nhất 1 cạnh trên cây MST chưa được bảo vệ (`!covered[t]`).
- Trong module `Reliability::findBackupForTreeEdge`: khi giả định một cạnh cây $e_f = (u, v)$ bị đứt, thuật toán BFS tách cây thành 2 nhánh $V_A, V_B$ và tìm tất cả các cạnh ngoài cây nối giữa $V_A$ và $V_B$ (ưu tiên cạnh đã xây `isBuilt = true`, sau đó đến chi phí nhỏ nhất).

---

## 4. MÔ HÌNH ĐỘ TRỄ VIỄN THÔNG 4 THÀNH PHẦN (`DelayModel.cpp`)

Độ trễ cơ sở (`calcBaseDelay`) trên một chặng (hop) từ nút $u$ qua liên kết $e$ đến nút kế tiếp $v$ bao gồm 4 thành phần (đơn vị: **giây**):

$$D_{\text{base}}(p, e, v) = D_{\text{trans}} + D_{\text{prop}} + D_{\text{proc}} + D_{\text{queue}}$$

### A. Độ trễ truyền dẫn (`calcTransmissionDelay`):
Thời gian đẩy toàn bộ các bit của gói tin kích thước $\text{sizeBytes}$ lên đường truyền có băng thông $\text{bandwidthMbps}$:
$$D_{\text{trans}} = \frac{\text{sizeBytes} \times 8}{\text{bandwidthMbps} \times 10^6}\text{ (giây)}$$

### B. Độ trễ lan truyền vật lý (`calcPropagationDelay`):
Thời gian tín hiệu truyền dọc theo chiều dài cáp $\text{length}$ (m):
$$D_{\text{prop}} = \frac{\text{length}}{v_{\text{prop}}}\text{ (giây)}$$
Trong đó (`Config.h`):
- Cáp quang (`FIBER`) và Cáp đồng (`COPPER`): $v_{\text{prop}} = \text{SPEED\_OF\_LIGHT\_FIBER} = 2.0 \times 10^8\text{ m/s}$.
- Sóng không dây (`WIRELESS`): $v_{\text{prop}} = \text{SPEED\_OF\_LIGHT\_AIR} = 3.0 \times 10^8\text{ m/s}$.

### C. Độ trễ xử lý tại nút nhận (`calcProcessingDelay`):
$$D_{\text{proc}} = \text{nextNode.processingTime}\text{ (giây)}$$

### D. Độ trễ hàng đợi có xét tắc nghẽn (`calcQueueDelay`):
$$D_{\text{queue}} = \text{edge.queueDelay} \times \text{congestionPenalty}(\text{edge.currentLoad})$$
Với hàm phạt tắc nghẽn bậc thang `RoutingCost::congestionPenalty(load)`:
$$\text{congestionPenalty}(\text{load}) = \begin{cases} 1.00 & \text{nếu } \text{load} \le 0.80 \\ 1.56 & \text{nếu } 0.80 < \text{load} \le 0.85 \\ 3.25 & \text{nếu } 0.85 < \text{load} \le 0.90 \\ 6.06 & \text{nếu } 0.90 < \text{load} \le 0.95 \\ 10.00 & \text{nếu } \text{load} > 0.95 \end{cases}$$

---

## 5. MÔ HÌNH PHÂN MẢNH GÓI TIN (`calcFragmentCount` & `calculateHopDelay`)

Gói tin kích thước $\text{sizeBytes}$ đi qua liên kết có giới hạn $\text{MTU}$:
- Nếu $\text{sizeBytes} \le \text{MTU}$: Không cần phân mảnh $\to n = 1$ mảnh.
- Nếu $\text{sizeBytes} > \text{MTU}$:
  - Nếu không cho phép phân mảnh (`canFragment == false`, cờ DF bật): Trả về $n = 0 \implies D_{\text{hop}} = \infty$, gói tin bị hủy với lỗi `DF_BLOCKED_BY_MTU`.
  - Nếu cho phép phân mảnh (`canFragment == true`): Trừ $20\text{ bytes}$ IP Header, kích thước dữ liệu tối đa mỗi mảnh là $\text{payload} = \text{MTU} - 20$. Số mảnh sinh ra:
    $$n = \left\lceil \frac{\text{sizeBytes}}{\text{MTU} - 20} \right\rceil$$
    *(Ví dụ: $\text{sizeBytes} = 3000\text{ B}, \text{MTU} = 1500 \implies \text{payload} = 1480\text{ B} \implies n = \lceil 3000 / 1480 \rceil = 3\text{ mảnh}$)*.
- **Độ trễ một chặng khi có phân mảnh (`calculateHopDelay`):**
  $$D_{\text{hop}} = D_{\text{base}} + (n - 1) \times D_{\text{trans}}$$

---

## 6. ĐỘ TRỄ GIAO THỨC TCP / UDP (`calcProtocolOverhead` & `Simulator::sendPacket`)

Sau khi tính tổng độ trễ một chiều qua toàn bộ các chặng trên đường đi $D_{\text{one-way}} = \sum_{e \in \text{Path}} D_{\text{hop}}(e)$:
- **Giao thức UDP (`Protocol::UDP`):** Không thiết lập kết nối $\to \text{Overhead} = 0$.
- **Giao thức TCP (`Protocol::TCP`):**
  - Nếu là kết nối mới (`packet.newConnection == true`): Quá trình bắt tay 3 bước (*3-way handshake*) tốn thêm $1\text{ RTT} = 2.0 \times D_{\text{one-way}}$:
    $$\text{Overhead}_{\text{TCP}} = 2.0 \times D_{\text{one-way}} \implies D_{\text{total}} = 3.0 \times D_{\text{one-way}}$$
  - Nếu kết nối đã thiết lập (`packet.newConnection == false`): $\text{Overhead}_{\text{TCP}} = 0$.

---

## 7. HÀM TRỌNG SỐ ĐỊNH TUYẾN ĐỘNG DIJKSTRA (`RoutingCost.cpp` & `Config.h`)

Thuật toán `Dijkstra` sử dụng hàng đợi ưu tiên Min-Heap $\mathcal{O}((V + E) \log V)$ trên các liên kết đang hoạt động (`e.isUp == true`) và đã được lắp đặt (`onlyBuilt = true` $\implies$ `e.isBuilt == true`).

Chi phí định tuyến động của một cạnh $e$ đối với gói tin $p$ được tính bởi `RoutingCost::calculateRoutingCost(p, e)`:

$$w(p, e) = w_{\text{Trans}} \cdot D_{\text{trans}} + w_{\text{Prop}} \cdot D_{\text{prop}} + w_{\text{Queue}} \cdot \underbrace{\left(\text{queueDelay} \cdot \text{congestionPenalty}(\text{load})\right)}_{D_{\text{queue}}} + \text{fragmentPenalty}$$

Trong đó:
- **Phạt phân mảnh (`fragmentPenalty`):**
  $$\text{fragmentPenalty} = (n - 1) \times \text{FRAGMENT\_PENALTY\_PER\_EXTRA} \quad (\text{với } \text{FRAGMENT\_PENALTY\_PER\_EXTRA} = 0.0003\text{s})$$
- **Bộ trọng số ưu tiên dịch vụ QoS (`Config::QoSWeight`):**

| Loại gói tin (`PacketType`) | $w_{\text{Trans}}$ (Băng thông) | $w_{\text{Prop}}$ (Khoảng cách) | $w_{\text{Queue}}$ (Hàng đợi / Nghẽn) | Ý nghĩa kỹ thuật |
| :--- | :---: | :---: | :---: | :--- |
| **`REAL_TIME`** (Thoại, Video, Game) | $1.0$ | $1.0$ | **$4.0$** | Rất nhạy với trễ hàng đợi/jitter $\to$ Sẵn sàng đi đường vòng để né nghẽn. |
| **`BULK_DATA`** (Tải file lớn, Backup) | **$2.0$** | $1.0$ | $2.0$ | Chú trọng băng thông lớn ($10\text{Gbps}$) để giảm $D_{\text{trans}}$. |
| **`CONTROL`** (Bản tin điều khiển) | $1.0$ | $1.0$ | $1.0$ | Cân bằng trung tính giữa các thành phần độ trễ. |

---

## 8. CẬP NHẬT TẢI TRỌNG ĐỘNG SAU KHI TRUYỀN (`Simulator::updateNetworkState`)

Sau khi một gói tin truyền qua cạnh $e$, tải trọng `currentLoad` của cạnh được cập nhật và suy giảm theo cửa sổ thời gian (`Config.h`: `LOAD_WINDOW_SECONDS = 1.0`, `LOAD_DECAY_RATE = 0.99`):

$$\Delta_{\text{load}} = \frac{D_{\text{trans}}}{\text{LOAD\_WINDOW\_SECONDS}}$$
$$\text{currentLoad}_{\text{new}} = \min\left(1.0,\; \text{currentLoad}_{\text{old}} + \Delta_{\text{load}}\right) \times \text{LOAD\_DECAY\_RATE}$$
