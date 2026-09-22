# TỔNG HỢP VÀ GIẢI THÍCH CHI TIẾT TOÀN BỘ MÃ NGUỒN PBL2 (TUẦN 1 - TUẦN 3)

Tài liệu này là cẩm nang bách khoa toàn thư của dự án, tổng hợp toàn bộ các thư mục, các file, mã nguồn và giải thích chi tiết ý nghĩa từng dòng lệnh cho toàn bộ các thành viên (Ý & Quyến) tính đến thời điểm hoàn thành Tuần 3.

---

## PHẦN I. TỔNG QUAN KIẾN TRÚC THƯ MỤC

Dự án áp dụng chuẩn cấu trúc mã nguồn C/C++ hiện đại:
1. **`data/`**: Chứa các file text đầu vào (`nodes.txt`, `edges.txt`, `packets.txt`). Mô phỏng dữ liệu thô chưa được kiểm duyệt.
2. **`include/`**: Chứa toàn bộ các file Khai báo Header (`.h`). Đây là "Bộ xương" của chương trình, định nghĩa các cấu trúc dữ liệu và khuôn mẫu hàm.
3. **`src/`**: Chứa toàn bộ các file Triển khai (`.cpp`). Đây là "Phần thịt", chứa logic xử lý thực sự của các hàm đã khai báo ở `include/`.
4. **`tests/`**: Chứa các chương trình test nhỏ lẻ, độc lập để kiểm chứng từng module (Unit Test) mà không cần chạy toàn bộ dự án.
5. **`docs/`**: Chứa các tài liệu giải thích, báo cáo và quy trình làm việc (như file bạn đang đọc).

---

## PHẦN II. GIẢI THÍCH CHI TIẾT TỪNG FILE TRONG HỆ THỐNG

### 1. CÁC TỆP CƠ SỞ DỮ LIỆU (Data Structures)
*Nằm trong thư mục `include/`*

#### 📄 `Config.h` & `Types.h`
- **Mục đích:** `Config.h` gom tất cả các "Con số ma thuật" (Magic numbers) vào một chỗ để dễ thay đổi (ví dụ: tốc độ ánh sáng `SPEED_OF_LIGHT_FIBER`, tải tối đa `MAX_LOAD`, vô cực `INF`). `Types.h` chứa các danh sách liệt kê (Enum) phân loại Nút, Cáp, Giao thức để code đọc dễ hiểu hơn (ví dụ thay vì viết `type == 1`, ta viết `type == NodeType::ROUTER`).
- **Đóng góp:** Giúp dự án đạt chuẩn Clean Code (Không hard-code).

#### 📄 `Node.h` & `Edge.h` & `Packet.h`
- **Mục đích:** Đây là 3 Struct thần thánh đại diện cho 3 thực thể vật lý của mạng.
  - `Node` chứa thông tin về Router/Switch: số cổng tối đa (`totalPorts`), số cổng đã cắm (`usedPorts`), tốc độ xử lý CPU (`processingTime`).
  - `Edge` chứa thông tin dây cáp: độ dài, giá tiền, băng thông, tải trọng (`currentLoad`), cờ đứt cáp (`isUp`), và 2 cờ quy hoạch của Quyến (`isBuilt`, `isBackup`).
  - `Packet` chứa gói tin: Dung lượng (`sizeBytes`), giao thức (`protocol`), cờ cấm phân mảnh (`canFragment`).

---

### 2. LÕI ĐỒ THỊ VÀ ĐỌC FILE (Core Graph & IO)

#### 📄 `src/Graph.cpp`
**Mục đích:** Xây dựng một không gian toán học (Đồ thị vô hướng) để chứa các Node và Edge, đồng thời biết được Nút nào đang cắm vào Nút nào (Tính liền kề - Adjacency).

```cpp
bool Graph::addEdge(const Edge& edge) {
    if (edges.find(edge.id) != edges.end()) return false; // Trùng ID cáp
    if (!hasNode(edge.u) || !hasNode(edge.v)) return false; // Cắm vào Nút không tồn tại
    if (edge.u == edge.v) return false; // Cáp vòng tròn tự cắm vào chính nó

    edges[edge.id] = edge;
    // Cập nhật ma trận kề: Nút u có nối với dây cáp edge.id, và Nút v cũng vậy.
    adjacency[edge.u].push_back(edge.id);
    adjacency[edge.v].push_back(edge.id);
    return true;
}
```
*Giải thích:* Mỗi lần nối 1 dây cáp, Đồ thị phải học được rằng cả 2 đầu Nút (u và v) đều đang ngậm chung 1 ID dây cáp. Nhờ vậy sau này đứng ở u ta biết được có bao nhiêu dây đang cắm vào nó.

#### 📄 `src/IOManager.cpp`
**Mục đích:** Đọc file text (`data/edges.txt`...) rồi đúc thành đối tượng `Edge`, sau đó đẩy vào `Graph`. Nó kiêm luôn việc gọi Validation để từ chối dữ liệu bẩn.

---

### 3. CÁC MODULE CỦA Ý (Tuần 1, 2, 3)

#### 📄 `src/Validation.cpp` (Tuần 1 & 2)
**Mục đích:** "Bảo vệ hệ thống". Không cho phép nhập dữ liệu sai logic vật lý.

```cpp
bool isStaticFeasible(const Edge& edge) {
    // Cáp không được dài hơn sức chịu đựng của nó
    return edge.length <= edge.maxSegmentLength; 
}
bool checkPortCapacity(const Node& node) {
    // Không thể cắm cáp nếu Nút đã hết lỗ cắm
    return node.usedPorts < node.totalPorts;
}
```
*Giải thích:* `isStaticFeasible` gọi một lần vì chiều dài cáp không tự dài ra. `checkPortCapacity` phải gọi liên tục mỗi khi cắm cáp vì lỗ cắm sẽ vơi dần.

#### 📄 `src/CostModel.cpp` (Tuần 2)
**Mục đích:** Tính toán chính xác số tiền để kéo một sợi cáp, bóc tách ra 4 loại chi phí (Nhân công, Vật tư, Thiết bị, Bảo trì).

```cpp
CostBreakdown CostModel::calculateCablingCost(const Edge& edge) {
    CostBreakdown breakdown;
    if (!edge.isUp) {
        // Cáp đứt -> Giá vô cực (INF) -> Thuật toán né luôn.
        breakdown.total = Config::INF; 
        return breakdown;
    }
    breakdown.install = Config::INSTALLATION_COST;
    breakdown.material = edge.length * edge.unitPrice * edge.terrainFactor; // Dài x Giá x Khó
    breakdown.equipment = edge.equipmentCost;
    breakdown.maintenance = edge.maintenanceCost;
    breakdown.total = breakdown.install + breakdown.material + breakdown.equipment + breakdown.maintenance;
    return breakdown;
}
```
*Giải thích:* Hàm này tuyệt đối tuân thủ thực tế. Nếu `!isUp` (đứt) thì tổng tiền là `INF`. Nếu không, tiền sẽ bằng Độ dài nhân Đơn giá và cộng các chi phí cố định.

#### 📄 `src/DelayModel.cpp` (Tuần 3)
**Mục đích:** Mô phỏng lại các định luật vật lý (ánh sáng, điện từ) và giao thức (TCP, MTU) để ra được số Giây (s) mà một gói tin phải trải qua.

```cpp
    double calcTransmissionDelay(int sizeBytes, double bandwidthMbps) {
        // (Dung lượng * 8) / (Băng thông * 1,000,000)
        return (sizeBytes * 8.0) / (bandwidthMbps * 1000000.0);
    }
```
*Giải thích:* Đổi Byte ra bit, đổi Megabit ra bit. Sau đó chia cho nhau để ra thời gian đẩy bit lên dây.

#### 📄 `src/Reporter.cpp` (Tuần 2)
**Mục đích:** Xuất hóa đơn tổng kết chi phí sau khi chạy thuật toán tìm đường.

```cpp
        // Ưu tiên kiểm tra isBackup trước
        if (edge.isBackup) {
            totalMSTBackup += cost;
        } else if (edge.isBuilt) {
            totalMST += cost;
            totalMSTBackup += cost;
        }
```
*Giải thích:* Phân loại xem cáp nào là cáp chính (MST), cáp nào là Dự phòng (Backup) để tính tổng tiền cho 3 phương án quy hoạch. Buộc phải check cờ Backup trước để tránh bị nuốt mất dữ liệu.

---

### 4. CÁC MODULE CỦA QUYẾN (Tuần 2)

#### 📄 `src/DSU.cpp` (Disjoint Set Union)
**Mục đích:** Thuật toán chống tạo chu trình (chống vòng lặp).

```cpp
bool DSU::unite(int i, int j) {
    int root_i = find(i);
    int root_j = find(j);
    if (root_i == root_j) return false; // Nút i và j đã có chung "Tổ tiên" -> Cắm vào sẽ thành Vòng Tròn (Chu trình) -> Từ chối.
    // ... logic gộp 2 nhánh ...
    return true; // Cho phép nối
}
```
*Giải thích:* DSU giống như việc xét gia phả, nếu 2 người đã chung 1 cụ tổ (Chung 1 nhánh mạng lưới) thì không được kết hôn (Không được cắm cáp) để tránh tạo thành một vòng lặp chết chóc (Loop) trong mạng.

#### 📄 `src/Kruskal.cpp`
**Mục đích:** Tìm ra Cây khung nhỏ nhất (Minimal Spanning Tree - MST). Nhặt các sợi cáp RẺ NHẤT để nối tất cả các Nút lại với nhau thành một mạng lưới xuyên suốt mà không bị thừa dây (Không tạo chu trình). Đây là bộ não Quy hoạch mạng của Tuần 2.

```cpp
// Lấy tất cả cạnh và sắp xếp theo giá tiền tăng dần
std::sort(validEdges.begin(), validEdges.end(), [](const Edge& a, const Edge& b) {
    return CostModel::calculateCablingCost(a).total < CostModel::calculateCablingCost(b).total;
});

// Duyệt từng cạnh (từ rẻ nhất đến đắt nhất)
for (Edge& e : validEdges) {
    // 1. Nếu cắm vào mà vi phạm Ràng Buộc Động của Ý (Hết lỗ cắm) -> Bỏ qua
    if (!Validation::checkPortCapacity(u) || !Validation::checkPortCapacity(v)) continue;
    
    // 2. Nếu cắm vào mà tạo vòng tròn (Hàm unite của Quyến báo false) -> Bỏ qua
    if (!dsu.unite(e.u, e.v)) continue;

    // 3. Nếu an toàn, quyết định xây cáp này! Bật cờ isBuilt = true và trừ đi 1 lỗ cắm ở 2 đầu Nút.
    e.isBuilt = true;
    u.usedPorts++;
    v.usedPorts++;
}
```
*Giải thích:* Đây là "Sự kết hợp hoàn hảo" (Integration) giữa Ý và Quyến. Quyến phụ trách phần toán học (Sắp xếp, DSU), nhưng khi ra quyết định cắm dây thì phải gọi hàm `CostModel` và `Validation` của Ý ra để phán xử.

---

## PHẦN III. ĐÁNH GIÁ MỨC ĐỘ HOÀN THÀNH (STATUS REPORT)

Tính đến thời điểm hiện tại:

1. **Giai đoạn Đọc dữ liệu (Tuần 1):** HOÀN THÀNH 100%. (IOManager, Validation tĩnh).
2. **Giai đoạn Xây dựng mạng lưới Vật lý (Tuần 2):** HOÀN THÀNH 100%. (CostModel, Validation động, DSU, Kruskal, Tìm MST, Tìm Backup, Reporter).
3. **Giai đoạn Tính toán Độ trễ (Tuần 3 - Phần của Ý):** HOÀN THÀNH 100%. (Base Delay, Transmission, Propagation, Processing, Queue, Phân mảnh dữ liệu MTU, Overhead giao thức TCP/UDP).

**Những gì còn thiếu và sắp làm:**
- **Tuần 3 (Phần của Quyến):** Cần triển khai thuật toán **Dijkstra** để vạch ra đường đi ngắn nhất (chạy trên mạng lưới đã được Kruskal xây). Dijkstra của Quyến sẽ phải gọi `DelayModel::calculateTotalDelay` của Ý để biết đường nào chạy nhanh hơn.
- **Tuần 4 (Cả hai):** Đưa toàn bộ vào mô phỏng thực tế động (Dynamic Simulation). Bổ sung Penalty khi cáp bị nghẽn (Congestion), mô phỏng đứt cáp giữa chừng, và thay đổi độ ưu tiên của gói tin (QoS).

**Kết luận:** Đồ án đã được xây dựng cực kỳ chắc chắn với kiến trúc Clean Code và SOLID. Mọi logic đều được chia cắt rạch ròi, module này phục vụ module kia không bị chồng chéo. Hệ thống đã đủ vững vàng để gánh vác phần định tuyến cực khó (Dijkstra) ở giai đoạn tiếp theo!
