# SỔ TAY KỸ THUẬT TOÀN DIỆN & GIẢI THÍCH CHI TIẾT TỪNG DÒNG CODE
## ĐỒ ÁN PBL2: THIẾT KẾ VÀ MÔ PHỎNG HỆ THỐNG MẠNG VIỄN THÔNG
**Dành cho bảo vệ đồ án & vấn đáp chuyên sâu**

---

# TỔNG QUAN HỆ THỐNG & LUỒNG THỰC THI 11 BƯỚC

Hệ thống được thiết kế theo mô hình phân tầng chặt chẽ, tách bạch giữa **Tầng Hạ tầng Vật lý** (Đi dây, Cây khung MST, Chi phí lắp đặt) và **Tầng Dịch vụ Vận hành** (Định tuyến gói tin, Xử lý nghẽn mạng, Phân mảnh MTU, và Tự động phục hồi khi đứt cáp).

```text
[data/nodes.txt]    [data/edges.txt]    [data/packets.txt]    [data/pricing.txt]
       |                   |                   |                     |
       +-------------------+-------------------+---------------------+
                                   |
                                   v
                         [Bước 1: Load Core Data] (IOManager)
                                   |
                                   v
                         [Bước 2: Data Validation] (Validation)
                                   |
                                   v
                    [Khởi tạo Đồ thị Mạng gốc (baseGraph)]
                                   |
       +---------------------------+---------------------------+
       |                                                       |
       v                                                       v
[Bước 3: Quy hoạch MST] (Kruskal)             [Bước 4: Cáp dự phòng Backup] (selectBackupLinks)
(Tìm V-1 cạnh rẻ nhất nối mạng)                 (Chọn K cáp cứu mạng ngoài cây)
       |                                                       |
       +---------------------------+---------------------------+
                                   |
                                   v
             [Đánh dấu hạ tầng hoàn thành: isBuilt = true]
                                   |
                                   v
             [Bước 5: Bảng định tuyến tĩnh ban đầu] (Dijkstra)
                                   |
                                   v
             [Bước 6: Menu điều hướng tương tác] (main.cpp: [0-8])
                                   |
                 +-----------------+-----------------+
                 |                                   |
                 v                                   v
    [Xem báo cáo (Menu 7, 8)]           [Chạy Kịch bản Mô phỏng (Menu 1-6)]
    - printMSTReport()                  (data/scenarios/<scenario>.txt)
    - printFullRoutingTable()                        |
                                                     v
                                        [Bước 7: Reset đồ thị sạch]
                                        (testGraph = baseGraph)
                                                     |
                                                     v
                                        [Bước 8: Gom sự kiện theo mốc t]
                                                     |
                                                     v
                                        [Bước 9: Áp sự kiện & Snapshot]
                                        (LINK_DOWN, LOAD_UPDATE, ...)
                                                     |
                                                     v
                                        [Bước 10: Định tuyến động & Bắn gói]
                                        - Dijkstra động (QoS + Trễ + Tắc nghẽn)
                                        - sendPacket() (MTU, TCP Handshake)
                                                     |
                                                     v
                                        [Bước 11: Báo cáo & Xuất Log]
                                        - In màn hình (Console)
                                        - Logger::exportSimulationLog() (.txt)
```

---

# CHƯƠNG 1: CÁC CẤU TRÚC DỮ LIỆU NỀN TẢNG & CẤU HÌNH HỆ THỐNG

Các file trong chương này định nghĩa toàn bộ "ngôn ngữ dữ liệu" của đồ án: các bảng mã Enum, các thông số vật lý và toán học, cùng cấu trúc của Thiết bị (Node), Đường dây (Edge), Gói tin (Packet) và Kết quả tìm đường (PathResult).

---

## 1.1. File `include/Types.h`

### Toàn bộ mã nguồn:
```cpp
  1: #ifndef TYPES_H
  2: #define TYPES_H
  3: 
  4: #include <string>
  5: 
  6: enum class NodeType {
  7:     ROUTER,
  8:     HOST,
  9:     SWITCH
 10: };
 11: 
 12: enum class MediaType {
 13:     FIBER,
 14:     COPPER,
 15:     WIRELESS
 16: };
 17: 
 18: enum class Protocol {
 19:     TCP,
 20:     UDP
 21: };
 22: 
 23: enum class PacketType {
 24:     REAL_TIME,
 25:     BULK_DATA,
 26:     CONTROL
 27: };
 28: 
 29: enum class EventType {
 30:     LINK_DOWN,
 31:     LINK_UP,
 32:     LOAD_UPDATE,
 33:     DELAY_UPDATE
 34: };
 35: 
 36: #endif // TYPES_H

```

### Giải thích chi tiết từng dòng code:
* **Dòng 1-2, 36:** `#ifndef TYPES_H`, `#define TYPES_H`, `#endif`
  - Đây là kỹ thuật **Header Guard**. Nó ngăn chặn lỗi *redefinition* (định nghĩa lại kiểu dữ liệu) khi file header này được `#include` nhiều lần từ các file khác nhau trong dự án.
* **Dòng 6-10:** `enum class NodeType { ROUTER, HOST, SWITCH };`
  - Định nghĩa kiểu liệt kê phạm vi (`scoped enumeration` hay `enum class`). Việc dùng `enum class` thay vì `enum` thường giúp tránh việc xung đột tên định danh trong phạm vi toàn cục.
  - `ROUTER`: Thiết bị định tuyến mạng lớp 3 (L3), có thời gian trễ xử lý gói tin CPU (`processingTime > 0`).
  - `HOST`: Máy trạm đầu cuối (nguồn phát hoặc đích nhận của gói tin), không tham gia định tuyến trung chuyển.
  - `SWITCH`: Thiết bị chuyển mạch lớp 2 (L2).
* **Dòng 12-16:** `enum class MediaType { FIBER, COPPER, WIRELESS };`
  - Đại diện cho 3 môi trường truyền dẫn vật lý:
  - `FIBER` (Cáp quang): Vận tốc ánh sáng trong thủy tinh $v = 2.0 \times 10^8$ m/s, cự ly truyền tối đa lớn (lên tới hàng chục km), chi phí vật tư và thiết bị cổng quang cao.
  - `COPPER` (Cáp đồng / Cáp mạng xoắn đôi): Vận tốc truyền $2.0 \times 10^8$ m/s, cự ly truyền giới hạn nghiêm ngặt (chuẩn 100m, nếu vượt quá sẽ bị suy hao tín hiệu), chi phí rẻ.
  - `WIRELESS` (Sóng vô tuyến không dây): Vận tốc truyền trong không khí $v = 3.0 \times 10^8$ m/s, đơn giá cáp bằng 0 (không cần dây nối), nhưng phí thiết bị thu phát vô tuyến cao.
* **Dòng 18-21:** `enum class Protocol { TCP, UDP };`
  - `TCP`: Giao thức hướng kết nối (Connection-oriented), yêu cầu bắt tay 3 bước (3-way handshake) gây trễ $1 \times RTT$ khi thiết lập kết nối mới (`newConnection = true`).
  - `UDP`: Giao thức không hướng kết nối (Connectionless), không có độ trễ bắt tay, truyền tin lập tức.
* **Dòng 23-27:** `enum class PacketType { REAL_TIME, BULK_DATA, CONTROL };`
  - Phân loại chất lượng dịch vụ QoS của lưu lượng mạng:
  - `REAL_TIME`: Gói tin truyền thông thời gian thực (VoIP, Video Call) cực kỳ nhạy cảm với độ trễ hàng đợi và hiện tượng giật jitter $\to$ Trọng số phạt hàng đợi rất cao để Dijkstra chủ động né đường nghẽn.
  - `BULK_DATA`: Gói tin tải tập tin lớn (Download, Backup file), cần tối ưu băng thông lớn $\to$ Ưu tiên độ trễ truyền dẫn thấp.
  - `CONTROL`: Gói tin điều khiển giao thức mạng (OSPF, BGP, Ping), trọng số cân bằng.
* **Dòng 29-34:** `enum class EventType { LINK_DOWN, LINK_UP, LOAD_UPDATE, DELAY_UPDATE };`
  - `LINK_DOWN`: Sự cố đứt cáp vật lý $\to$ Cạnh bị tắt (`isUp = false`).
  - `LINK_UP`: Cáp quang nối xong $\to$ Cạnh phục hồi (`isUp = true`).
  - `LOAD_UPDATE`: Lưu lượng trên đường truyền thay đổi $\to$ Cập nhật `currentLoad`.
  - `DELAY_UPDATE`: Hàng đợi nghẽn cục bộ $\to$ Cập nhật `queueDelay`.

---

## 1.2. File `include/Config.h`

### Toàn bộ mã nguồn:
```cpp
  1: #ifndef CONFIG_H
  2: #define CONFIG_H
  3: 
  4: #include <limits> 
  5: 
  6: namespace Config {
  7:     // 1. Hằng số Mạng cơ bản
  8:     const double MAX_LOAD = 1.0;               // Tải tối đa
  9:     const double DEFAULT_TERRAIN_FACTOR = 1.0; // Hệ số địa hình mặc định
 10: 
 11:     // 2. Hằng số Thuật toán 
 12:     const double INF = std::numeric_limits<double>::infinity(); // Đại diện cho đứt cáp/không liên thông
 13: 
 14:     // 3. Hằng số Vật lý mạng 
 18:     const double SPEED_OF_LIGHT_FIBER = 2.0e8; // m/s (Trong cáp quang/cáp đồng)
 19:     const double SPEED_OF_LIGHT_AIR = 3.0e8;   // m/s (Sóng Wireless)
 20: 
 21:     // 5. Trọng số QoS cho Dynamic Routing Cost (Tuần 4, Task A4.2)
 22:     // Dùng trong RoutingCost::calculateRoutingCost() để Dijkstra chọn đường
 23:     // phù hợp với từng loại gói tin. KHÔNG hard-code các số này trong .cpp.
 24:     struct QoSWeight {
 25:         double wTrans;   // trọng số độ trễ truyền tải (liên quan băng thông)
 26:         double wProp;    // trọng số độ trễ lan truyền (liên quan khoảng cách)
 27:         double wQueue;   // trọng số độ trễ hàng đợi (liên quan tắc nghẽn)
 28:     };
 29: 
 30:     // REAL_TIME: rất nhạy với độ trễ hàng đợi/tắc nghẽn (jitter khó chịu),
 31:     // nên wQueue cao nhất -> sẵn sàng đi đường xa hơn để né nghẽn.
 32:     const QoSWeight QOS_WEIGHT_REAL_TIME = {1.0, 1.0, 4.0};
 33: 
 34:     // BULK_DATA: chú trọng băng thông (wTrans cao hơn) và vẫn né tắc nghẽn
 35:     // nặng, nhưng không nhạy cảm bằng REAL_TIME.
 36:     const QoSWeight QOS_WEIGHT_BULK_DATA = {2.0, 1.0, 2.0};
 37: 
 38:     // CONTROL: cân bằng, không nghiêng hẳn về phía nào.
 39:     const QoSWeight QOS_WEIGHT_CONTROL = {1.0, 1.0, 1.0};
 40: 
 41:     // Phạt thêm (giây) cho MỖI mảnh vượt quá 1 khi gói tin phải phân mảnh,
 42:     // để Dijkstra ưu tiên đường có MTU đủ lớn, tránh phải băm nhỏ gói tin.
 43:     const double FRAGMENT_PENALTY_PER_EXTRA = 0.0003;
 44: }
 45: 
 46: #endif // CONFIG_H

```

### Giải thích chi tiết từng dòng code:
* **Dòng 1-2, 46:** Header guard bảo vệ cấu hình hệ thống.
* **Dòng 4:** `#include <limits>` để lấy giá trị vô cực toán học `std::numeric_limits<double>::infinity()`.
* **Dòng 6:** `namespace Config { ... }`: Gom toàn bộ các hằng số cấu hình vào một không gian tên riêng biệt, tuân thủ nguyên tắc không làm ô nhiễm không gian tên toàn cục (Global Namespace Pollution).
* **Dòng 8:** `const double MAX_LOAD = 1.0;`: Tải tối đa của đường truyền là $100\%$ ($1.0$).
* **Dòng 9:** `const double DEFAULT_TERRAIN_FACTOR = 1.0;`: Hệ số địa hình mặc định (đồng bằng bằng phẳng). Nếu địa hình đồi núi hiểm trở, hệ số này lớn hơn $1.0$ làm đội chi phí thi công cáp.
* **Dòng 12:** `const double INF = std::numeric_limits<double>::infinity();`: Giá trị vô cực biểu diễn chi phí không thể đi qua (cáp đứt, gói quá MTU mà cấm phân mảnh, đồ thị không liên thông).
* **Dòng 18-19:**
  - `const double SPEED_OF_LIGHT_FIBER = 2.0e8;`: Vận tốc truyền tín hiệu trong lõi thủy tinh cáp quang / lõi đồng ($200,000$ km/s).
  - `const double SPEED_OF_LIGHT_AIR = 3.0e8;`: Vận tốc ánh sáng trong không khí ($300,000$ km/s).
* **Dòng 24-28:** `struct QoSWeight { double wTrans; double wProp; double wQueue; };`
  - Cấu trúc chứa bộ 3 trọng số đánh giá chất lượng dịch vụ: Trọng số trễ truyền dẫn (`wTrans`), trễ lan truyền (`wProp`), và trễ hàng đợi (`wQueue`).
* **Dòng 32:** `const QoSWeight QOS_WEIGHT_REAL_TIME = {1.0, 1.0, 4.0};`
  - Đặt `wQueue = 4.0` (rất cao): Buộc Dijkstra coi trễ hàng đợi nặng gấp 4 lần bình thường, giúp gói tin REAL_TIME tự động chọn đường vòng vắng vẻ hơn là chui vào đường thẳng đang bị kẹt xe.
* **Dòng 36:** `const QoSWeight QOS_WEIGHT_BULK_DATA = {2.0, 1.0, 2.0};`
  - Đặt `wTrans = 2.0`: Ưu tiên chọn đường truyền có băng thông lớn ($d_{trans} = \frac{\text{size}}{\text{bandwidth}}$ nhỏ).
* **Dòng 39:** `const QoSWeight QOS_WEIGHT_CONTROL = {1.0, 1.0, 1.0};`: Bộ trọng số trung tính.
* **Dòng 43:** `const double FRAGMENT_PENALTY_PER_EXTRA = 0.0003;`
  - Hệ số phạt bổ sung (0.3 ms) cho mỗi mảnh dữ liệu vượt quá 1 khi gói tin phải phân mảnh. Giúp Dijkstra ưu tiên các tuyến cáp có MTU lớn để tránh phân mảnh.

---

## 1.3. File `include/Node.h`

### Toàn bộ mã nguồn:
```cpp
  1: #ifndef NODE_H
  2: #define NODE_H
  3: 
  4: #include <string>
  5: #include "Types.h"
  6: 
  7: struct Node {
  8:     int id;
  9:     std::string name;
 10:     NodeType type;
 11:     int totalPorts;
 12:     int usedPorts;
 13:     double processingTime;
 14: 
 15:     Node() : id(-1), type(NodeType::ROUTER), totalPorts(0), usedPorts(0), processingTime(0.0) {}
 16: };
 17: 
 18: #endif 

```

### Giải thích chi tiết từng dòng code:
* **Dòng 7-16:** `struct Node { ... }`:
  - `int id`: Mã định danh duy nhất của thiết bị trong đồ thị mạng (0, 1, 2, ...).
  - `std::string name`: Tên gợi nhớ của thiết bị (ví dụ: "R1", "R2", "H1", "H2").
  - `NodeType type`: Loại thiết bị (`ROUTER` hoặc `HOST`).
  - `int totalPorts`: Tổng số cổng vật lý mà thiết bị sở hữu (ví dụ Router có 4 cổng, Host có 1 cổng).
  - `int usedPorts`: Số cổng đã được cắm dây cáp. Ràng buộc quan trọng: `usedPorts <= totalPorts`. Nếu hết cổng thì không thể nối thêm cáp mới (bị Kruskal loại).
  - `double processingTime`: Thời gian CPU của router xử lý và tra bảng chuyển tiếp gói tin (đơn vị: giây, ví dụ 0.0001s = 0.1ms).
  - `Node()` (Constructor mặc định): Khởi tạo node với `id = -1`, `type = ROUTER`, các cổng bằng 0 và thời gian trễ bằng 0.

---

## 1.4. File `include/Edge.h`

### Toàn bộ mã nguồn:
```cpp
  1: #ifndef EDGE_H
  2: #define EDGE_H
  3: 
  4: #include "Types.h"
  5: 
  6: struct Edge {
  7:     int id;
  8:     int u;
  9:     int v;
 10:     MediaType mediaType;
 11:     double length;            // m
 12:     double bandwidthMbps;     // Mbps
 13:     double maxSegmentLength;  // m
 14:     double terrainFactor;
 15:     int mtu;                  // bytes
 16:     
 17:     double currentLoad;       // 0.0 to 1.0
 18:     double queueDelay;        // seconds
 19:     bool isUp;
 20:     bool isBackup;
 21:     bool isBuilt;
 22: 
 23:     Edge() : id(-1), u(-1), v(-1), mediaType(MediaType::COPPER), length(0.0), 
 24:              bandwidthMbps(0.0), maxSegmentLength(0.0), 
 25:              terrainFactor(1.0), 
 26:              mtu(1500), currentLoad(0.0), queueDelay(0.0), 
 27:              isUp(true), isBackup(false), isBuilt(false) {}
 28: };
 29: 
 30: #endif // EDGE_H

```

### Giải thích chi tiết từng dòng code:
* **Dòng 6-28:** `struct Edge { ... }`: Đại diện cho một đoạn cáp kết nối vật lý nối giữa 2 nút:
  - `int id`: Định danh cạnh. Trong mạng thực tế có thể có nhiều sợi cáp song song giữa 2 node (ví dụ vừa có cáp quang vừa có cáp đồng), nên việc định danh cạnh qua `id` độc lập là bắt buộc.
  - `int u, v`: ID của 2 nút mạng ở 2 đầu sợi cáp.
  - `MediaType mediaType`: Loại cáp (`FIBER`, `COPPER`, `WIRELESS`).
  - `double length`: Chiều dài vật lý của sợi cáp (đơn vị: mét).
  - `double bandwidthMbps`: Băng thông tối đa của đường cáp (đơn vị: Megabit/giây - Mbps).
  - `double maxSegmentLength`: Chiều dài đoạn truyền tối đa không cần trạm lặp (Repeater). Nếu `length > maxSegmentLength`, cáp bị loại do suy hao tín hiệu (`TOO_LONG`).
  - `double terrainFactor`: Hệ số địa hình thi công cáp ($1.0$ là bình thường, $>1.0$ là đồi núi/hiểm trở).
  *(Lưu ý: Toàn bộ đơn giá vật tư, thiết bị, bảo trì được quản lý tập trung và nạp độc lập từ `data/pricing.txt` qua `CostModel`, không lưu phân tán trong `Edge`)*.
  - `int mtu`: Giới hạn kích thước gói tối đa (Maximum Transmission Unit), chuẩn Ethernet là 1500 byte.
  - `double currentLoad`: Mức tải lưu lượng hiện tại ($0.0 \le \text{load} \le 1.0$).
  - `double queueDelay`: Trễ hàng đợi cơ sở của cáp khi không có tắc nghẽn (đơn vị: giây).
  - `bool isBuilt`: Cờ đánh dấu cáp đã được phê duyệt bỏ tiền xây dựng trong thực tế (thuộc MST hoặc Backup) hay chỉ là phương án khảo sát trên giấy.
  - `bool isBackup`: Cờ đánh dấu sợi cáp này đóng vai trò là cáp dự phòng.
  - `bool isUp`: Cờ trạng thái vật lý của dây cáp (`true` = thông suốt, `false` = đứt cáp / hỏng thiết bị).

---

## 1.5. File `include/Packet.h`

### Toàn bộ mã nguồn:
```cpp
  1: #ifndef PACKET_H
  2: #define PACKET_H
  3: 
  4: #include "Types.h"
  5: 
  6: struct Packet {
  7:     int id;
  8:     int source;
  9:     int destination;
 10:     int sizeBytes;
 11:     PacketType type;
 12:     Protocol protocol;
 13:     bool canFragment;
 14:     bool newConnection;
 15: 
 16:     Packet() : id(-1), source(-1), destination(-1), sizeBytes(0), 
 17:                type(PacketType::BULK_DATA), protocol(Protocol::UDP), 
 18:                canFragment(true), newConnection(false) {}
 19: };
 20: 
 21: #endif // PACKET_H

```

### Giải thích chi tiết từng dòng code:
* **Dòng 7-20:** `struct Packet { ... }`: Biểu diễn một đơn vị dữ liệu cần vận chuyển qua mạng:
  - `int id`: Mã số gói tin (0, 1, 2, ...).
  - `int source`: ID nút nguồn phát sinh gói tin.
  - `int destination`: ID nút đích cần chuyển gói tin đến.
  - `int sizeBytes`: Kích thước toàn bộ gói tin (đơn vị: Byte).
  - `PacketType type`: Loại gói (`REAL_TIME`, `BULK_DATA`, `CONTROL`).
  - `Protocol protocol`: Giao thức tầng vận chuyển (`TCP` hoặc `UDP`).
  - `bool canFragment`: Cờ cấm/cho phép phân mảnh (tương ứng với bit Don't Fragment - DF trong header IPv4). Nếu `canFragment = false` mà kích thước gói lớn hơn MTU của cáp $\to$ Gói bị DROP ngay lập tức.
  - `bool newConnection`: Cờ báo hiệu đây là luồng kết nối TCP mới mở. Nếu `true`, gói tin phải gánh thêm thời gian bắt tay 3 bước (1 RTT handshake).

---

## 1.6. File `include/PathResult.h`

### Toàn bộ mã nguồn:
```cpp
  1: #ifndef PATH_RESULT_H
  2: #define PATH_RESULT_H
  3: 
  4: #include <vector>
  5: 
  6: // ============================================================
  7: // PathResult - kết quả của một lần tìm đường (Dijkstra) từ node
  8: // nguồn (source) đến node đích (destination).
  9: // ============================================================
 10: struct PathResult {
 11:     bool reachable = false;     // true nếu tới được destination
 12: 
 13:     // Đường đi, theo thứ tự source -> destination (đã đảo chiều sau khi dựng lại)
 14:     std::vector<int> nodes;     // danh sách nodeId trên đường đi
 15:     std::vector<int> edges;     // danh sách edgeId trên đường đi (edges.size() == nodes.size() - 1)
 16: 
 17:     double totalCost = 0.0;     // tổng trọng số (weight) của đường đi
 18:     int    hopCount   = 0;      // số cạnh đã đi qua, luôn bằng edges.size()
 19: };
 20: 
 21: #endif // PATH_RESULT_H

```

### Giải thích chi tiết từng dòng code:
* **Dòng 8-28:** `struct PathResult { ... }`: Kết quả trả về sau khi thuật toán Dijkstra tìm đường:
  - `bool reachable`: Cờ báo đích đến có thể tới được hay không (`true` = tìm thấy đường, `false` = bị chia cắt mạng hoàn toàn).
  - `std::vector<int> nodes`: Danh sách các nút theo đúng thứ tự hành trình (ví dụ: `0 -> 2 -> 3`).
  - `std::vector<int> edges`: Danh sách các ID sợi cáp đi qua trên lộ trình (ví dụ: Cạnh #2, Cạnh #3).
  - `double totalCost`: Tổng chi phí tối ưu tích lũy dọc theo đường đi theo hàm trọng số `RoutingCost`.
  - `int hopCount`: Số chặng (số cạnh đi qua), chính bằng `edges.size()`.

---

## 1.7. File `include/Scenario.h`

### Toàn bộ mã nguồn:
```cpp
  1: #ifndef SCENARIO_H
  2: #define SCENARIO_H
  3: 
  4: #include <string>
  5: #include <vector>
  6: #include "Types.h"
  7: 
  8: // Task B4.3: ScenarioEvent
  9: struct ScenarioEvent {
 10:     EventType eventType;
 11:     int targetId;
 12:     double value;
 13:     double time;
 14: };
 15: 
 16: // Cấu trúc gom nhóm toàn bộ các sự kiện của một kịch bản cụ thể
 17: struct Scenario {
 18:     std::string name;
 19:     std::vector<ScenarioEvent> events;
 20: };
 21: 
 22: #endif // SCENARIO_H

```

### Giải thích chi tiết từng dòng code:
* **Dòng 8-15:** `struct ScenarioEvent { ... }`: Biểu diễn một biến cố mạng xảy ra tại một thời điểm:
  - `double time`: Mốc thời gian xảy ra sự kiện (tính bằng giây, ví dụ 1.0s, 2.5s).
  - `EventType eventType`: Loại sự kiện (`LINK_DOWN`, `LINK_UP`, `LOAD_UPDATE`, `DELAY_UPDATE`).
  - `int targetId`: ID của cạnh chịu tác động của sự kiện.
  - `double value`: Giá trị thiết lập mới (ví dụ mức tải mới $0.85$ hoặc trễ hàng đợi mới $0.005$s).
* **Dòng 17-20:** `struct Scenario { ... }`:
  - `std::string name`: Đường dẫn hoặc tên file kịch bản (ví dụ: `"data/scenarios/failure.txt"`).
  - `std::vector<ScenarioEvent> events`: Danh sách các sự kiện được sắp xếp theo trình tự thời gian tăng dần.

---

## 1.8. File `include/EnumUtils.h` & `src/EnumUtils.cpp`

### Toàn bộ mã nguồn Header (`include/EnumUtils.h`):
```cpp
  1: #ifndef ENUMUTILS_H
  2: #define ENUMUTILS_H
  3: 
  4: #include "Types.h"
  5: #include <string>
  6: 
  7: namespace EnumUtils {
  8:     // Chuyển đổi Enum sang chuỗi văn bản (dùng cho in ấn báo cáo / log)
  9:     const char* toString(NodeType type);
 10:     const char* toString(MediaType media);
 11:     const char* toString(Protocol proto);
 12:     const char* toString(PacketType type);
 13:     const char* toString(EventType event);
 14: 
 15:     // Chuyển đổi chuỗi văn bản sang Enum (trả về true nếu hợp lệ, false nếu chuỗi lạ)
 16:     bool parseNodeType(const std::string& str, NodeType& out);
 17:     bool parseMediaType(const std::string& str, MediaType& out);
 18:     bool parseProtocol(const std::string& str, Protocol& out);
 19:     bool parsePacketType(const std::string& str, PacketType& out);
 20:     bool parseEventType(const std::string& str, EventType& out);
 21: }
 22: 
 23: #endif // ENUMUTILS_H

```

### Toàn bộ mã nguồn Cài đặt (`src/EnumUtils.cpp`):
```cpp
  1: #include "EnumUtils.h"
  2: 
  3: namespace EnumUtils {
  4: 
  5: const char* toString(NodeType type) {
  6:     switch (type) {
  7:         case NodeType::ROUTER: return "ROUTER";
  8:         case NodeType::HOST:   return "HOST";
  9:         case NodeType::SWITCH: return "SWITCH";
 10:     }
 11:     return "UNKNOWN";
 12: }
 13: 
 14: const char* toString(MediaType media) {
 15:     switch (media) {
 16:         case MediaType::FIBER:    return "FIBER";
 17:         case MediaType::COPPER:   return "COPPER";
 18:         case MediaType::WIRELESS: return "WIRELESS";
 19:     }
 20:     return "UNKNOWN";
 21: }
 22: 
 23: const char* toString(Protocol proto) {
 24:     switch (proto) {
 25:         case Protocol::TCP: return "TCP";
 26:         case Protocol::UDP: return "UDP";
 27:     }
 28:     return "UNKNOWN";
 29: }
 30: 
 31: const char* toString(PacketType type) {
 32:     switch (type) {
 33:         case PacketType::REAL_TIME: return "REAL_TIME";
 34:         case PacketType::BULK_DATA: return "BULK_DATA";
 35:         case PacketType::CONTROL:   return "CONTROL";
 36:     }
 37:     return "UNKNOWN";
 38: }
 39: 
 40: const char* toString(EventType event) {
 41:     switch (event) {
 42:         case EventType::LINK_DOWN:    return "LINK_DOWN";
 43:         case EventType::LINK_UP:      return "LINK_UP";
 44:         case EventType::LOAD_UPDATE:  return "LOAD_UPDATE";
 45:         case EventType::DELAY_UPDATE: return "DELAY_UPDATE";
 46:     }
 47:     return "UNKNOWN";
 48: }
 49: 
 50: bool parseNodeType(const std::string& str, NodeType& out) {
 51:     if (str == "ROUTER") { out = NodeType::ROUTER; return true; }
 52:     if (str == "HOST")   { out = NodeType::HOST;   return true; }
 53:     if (str == "SWITCH") { out = NodeType::SWITCH; return true; }
 54:     return false;
 55: }
 56: 
 57: bool parseMediaType(const std::string& str, MediaType& out) {
 58:     if (str == "FIBER")    { out = MediaType::FIBER;    return true; }
 59:     if (str == "COPPER")   { out = MediaType::COPPER;   return true; }
 60:     if (str == "WIRELESS") { out = MediaType::WIRELESS; return true; }
 61:     return false;
 62: }
 63: 
 64: bool parseProtocol(const std::string& str, Protocol& out) {
 65:     if (str == "TCP") { out = Protocol::TCP; return true; }
 66:     if (str == "UDP") { out = Protocol::UDP; return true; }
 67:     return false;
 68: }
 69: 
 70: bool parsePacketType(const std::string& str, PacketType& out) {
 71:     if (str == "REAL_TIME") { out = PacketType::REAL_TIME; return true; }
 72:     if (str == "BULK_DATA") { out = PacketType::BULK_DATA; return true; }
 73:     if (str == "CONTROL")   { out = PacketType::CONTROL;   return true; }
 74:     return false;
 75: }
 76: 
 77: bool parseEventType(const std::string& str, EventType& out) {
 78:     if (str == "LINK_DOWN")    { out = EventType::LINK_DOWN;    return true; }
 79:     if (str == "LINK_UP")      { out = EventType::LINK_UP;      return true; }
 80:     if (str == "LOAD_UPDATE")  { out = EventType::LOAD_UPDATE;  return true; }
 81:     if (str == "DELAY_UPDATE") { out = EventType::DELAY_UPDATE; return true; }
 82:     return false;
 83: }
 84: 
 85: } // namespace EnumUtils

```

### Giải thích chi tiết từng dòng code:
* **Mục đích kiến trúc:** Module này giải quyết triệt để lỗi vi phạm Clean Code (Hardcoded strings và duplicate converters). Nó tập trung hóa toàn bộ việc chuyển đổi hai chiều giữa chuỗi văn bản và các kiểu Enum.
* **Các hàm `EnumUtils::toString(...)` (Dòng 5-41 trong .cpp):**
  - Nhận vào một giá trị Enum và trả về chuỗi hằng số tương ứng (`const char*`). Sử dụng cấu trúc `switch-case` tối ưu về bộ nhớ, không sinh bản sao heap allocation. Nếu gặp mã không hợp lệ, trả về `"UNKNOWN"`.
* **Các hàm `EnumUtils::parseXxx(...)` (Dòng 43-78 trong .cpp):**
  - Nhận chuỗi `str` từ file dữ liệu đầu vào và tham chiếu `out`. Trả về `bool`: `true` nếu chuyển đổi thành công, `false` nếu chuỗi lạ/sai cú pháp.
  - Điều này cho phép `CostModel` và `IOManager` phát hiện ngay lập tức các lỗi gõ sai chính tả (như gõ nhầm `"FIBRE"` thay vì `"FIBER"`) và in cảnh báo ra màn hình thay vì gán nhầm giá trị.

---

# CHƯƠNG 2: CẤU TRÚC ĐỒ THỊ & BẢO VỆ DỮ LIỆU ĐẦU VÀO (DEFENSIVE IO & VALIDATION)

Chương này trình bày cách hệ thống xây dựng mô hình đồ thị mạng trong bộ nhớ RAM, cách nạp dữ liệu từ các file cấu hình và cơ chế kiểm duyệt dữ liệu đa tầng (Defensive Input Validation) để đảm bảo mạng luôn hoạt động ổn định, không bị crash bởi dữ liệu bẩn.

---

## 2.1. File `include/Graph.h` & `src/Graph.cpp`

### Toàn bộ mã nguồn Header (`include/Graph.h`):
```cpp
  1: #ifndef GRAPH_H
  2: #define GRAPH_H
  3: 
  4: #include <vector>
  5: #include <unordered_map>
  6: #include <stdexcept>
  7: #include "Node.h"
  8: #include "Edge.h"
  9: 
 10: // ============================================================
 11: // Graph - đồ thị vô hướng lưu mạng.
 12: //
 13: // Lưu 3 thứ:
 14: //   nodes     : id -> Node
 15: //   edges     : id -> Edge
 16: //   adjacency : nodeId -> danh sách EDGE ID chạm vào node đó
 17: //
 18: // Vì sao adjacency lưu edgeId thay vì nodeId?
 19: //   Hai node có thể có NHIỀU dây ứng viên (vd cáp đồng + cáp quang).
 20: //   Có edgeId thì biết chính xác dây nào, lấy được cost/bandwidth của nó.
 21: //   Muốn biết node bên kia thì gọi otherEndpoint(nodeId, edgeId).
 22: //
 23: // Vì sao getNode/getEdge trả THAM CHIẾU (không copy)?
 24: //   Kruskal phải sửa trực tiếp usedPorts, isBuilt, isBackup trong đồ thị.
 25: //   Nếu trả bản copy thì sửa xong đồ thị không đổi gì.
 26: // ============================================================
 27: class Graph {
 28: private:
 29:     std::unordered_map<int, Node> nodes;
 30:     std::unordered_map<int, Edge> edges;
 31: 
 32:     // adjacency[nodeId] = danh sách các edgeId kề
 33:     std::unordered_map<int, std::vector<int>> adjacency;
 34: 
 35: public:
 36:     Graph();
 37:     ~Graph();
 38: 
 39:     // Thêm Node. Trả false (không thêm gì) nếu id đã tồn tại.
 40:     bool addNode(const Node& node);
 41: 
 42:     // Thêm Edge. Trả false (không thêm gì) nếu: id đã tồn tại,
 43:     // node u hoặc v chưa tồn tại, hoặc u == v (dây nối node với chính nó).
 44:     // Adjacency được cập nhật ngay: edge xuất hiện ở CẢ HAI đầu.
 45:     bool addEdge(const Edge& edge);
 46: 
 47:     bool hasNode(int nodeId) const;
 48:     bool hasEdge(int edgeId) const;
 49: 
 50:     // Lấy Node / Edge theo id. Ném std::runtime_error nếu không tồn tại
 51:     // (Validation dùng cách bắt exception này để kiểm tra node có tồn tại).
 52:     // Bản non-const trả tham chiếu để sửa được usedPorts, isBuilt, ...
 53:     Node&       getNode(int nodeId);
 54:     const Node& getNode(int nodeId) const;
 55:     Edge&       getEdge(int edgeId);
 56:     const Edge& getEdge(int edgeId) const;
 57: 
 58:     // Danh sách edgeId kề với một node (rỗng nếu node không tồn tại)
 59:     const std::vector<int>& getNeighbors(int nodeId) const;
 60: 
 61:     // Trả nodeId ở đầu kia của cạnh khi biết một nodeId và edgeId.
 62:     // Trả -1 nếu edge không tồn tại hoặc node không thuộc edge đó.
 63:     int otherEndpoint(int nodeId, int edgeId) const;
 64: 
 65:     // Dựng lại adjacency từ đầu theo danh sách edges (dùng khi sửa trực tiếp edges).
 66:     // Thứ tự edgeId trong mỗi danh sách tăng dần -> kết quả luôn giống nhau giữa các lần chạy.
 67:     void buildAdjacency();
 68: 
 69:     // Số lượng node / edge
 70:     int V() const;
 71:     int E() const;
 72: 
 73:     // Danh sách id đã sắp tăng dần (unordered_map duyệt không theo thứ tự cố định,
 74:     // nên thuật toán cần thứ tự ổn định thì lấy từ đây)
 75:     std::vector<int> nodeIds() const;
 76:     std::vector<int> edgeIds() const;
 77: 
 78:     // Id node lớn nhất (-1 nếu chưa có node). DSU dùng để biết cần bao nhiêu phần tử.
 79:     int maxNodeId() const;
 80: 
 81:     // Đưa về trạng thái "chưa xây gì": usedPorts = 0, isBuilt = isBackup = false
 82:     void resetBuildState();
 83: };
 84: 
 85: #endif // GRAPH_H

```

### Toàn bộ mã nguồn Cài đặt (`src/Graph.cpp`):
```cpp
  1: #include "Graph.h"
  2: #include <algorithm>
  3: 
  4: Graph::Graph() {}
  5: 
  6: Graph::~Graph() {}
  7: 
  8: bool Graph::addNode(const Node& node) {
  9:     if (nodes.count(node.id)) return false;   // id trùng: không ghi đè
 10:     nodes[node.id] = node;
 11:     adjacency[node.id];                       // tạo sẵn danh sách kề rỗng cho node này
 12:     return true;
 13: }
 14: 
 15: bool Graph::addEdge(const Edge& edge) {
 16:     if (edges.count(edge.id)) return false;                   // id trùng
 17:     if (!hasNode(edge.u) || !hasNode(edge.v)) return false;   // đầu mút phải tồn tại
 18:     if (edge.u == edge.v) return false;                       // không cho dây khuyên
 19:     edges[edge.id] = edge;
 20:     // Mỗi edge xuất hiện ở adjacency của CẢ HAI đầu => tổng kích thước = 2E
 21:     adjacency[edge.u].push_back(edge.id);
 22:     adjacency[edge.v].push_back(edge.id);
 23:     return true;
 24: }
 25: 
 26: bool Graph::hasNode(int nodeId) const { return nodes.count(nodeId) > 0; }
 27: bool Graph::hasEdge(int edgeId) const { return edges.count(edgeId) > 0; }
 28: 
 29: Node& Graph::getNode(int nodeId) {
 30:     auto it = nodes.find(nodeId);
 31:     if (it == nodes.end()) throw std::runtime_error("Node not found!");
 32:     return it->second;
 33: }
 34: 
 35: const Node& Graph::getNode(int nodeId) const {
 36:     auto it = nodes.find(nodeId);
 37:     if (it == nodes.end()) throw std::runtime_error("Node not found!");
 38:     return it->second;
 39: }
 40: 
 41: Edge& Graph::getEdge(int edgeId) {
 42:     auto it = edges.find(edgeId);
 43:     if (it == edges.end()) throw std::runtime_error("Edge not found!");
 44:     return it->second;
 45: }
 46: 
 47: const Edge& Graph::getEdge(int edgeId) const {
 48:     auto it = edges.find(edgeId);
 49:     if (it == edges.end()) throw std::runtime_error("Edge not found!");
 50:     return it->second;
 51: }
 52: 
 53: const std::vector<int>& Graph::getNeighbors(int nodeId) const {
 54:     static const std::vector<int> empty;
 55:     auto it = adjacency.find(nodeId);
 56:     return it != adjacency.end() ? it->second : empty;
 57: }
 58: 
 59: int Graph::otherEndpoint(int nodeId, int edgeId) const {
 60:     auto it = edges.find(edgeId);
 61:     if (it == edges.end()) return -1;
 62:     const Edge& e = it->second;
 63:     if (nodeId == e.u) return e.v;
 64:     if (nodeId == e.v) return e.u;
 65:     return -1;   // node không thuộc edge này
 66: }
 67: 
 68: void Graph::buildAdjacency() {
 69:     adjacency.clear();
 70:     for (const auto& kv : nodes) adjacency[kv.first];   // node cô lập vẫn có danh sách rỗng
 71:     for (int id : edgeIds()) {                           // duyệt theo id tăng dần cho ổn định
 72:         const Edge& e = edges.at(id);
 73:         if (!hasNode(e.u) || !hasNode(e.v) || e.u == e.v) continue;
 74:         adjacency[e.u].push_back(id);
 75:         adjacency[e.v].push_back(id);
 76:     }
 77: }
 78: 
 79: int Graph::V() const { return static_cast<int>(nodes.size()); }
 80: int Graph::E() const { return static_cast<int>(edges.size()); }
 81: 
 82: std::vector<int> Graph::nodeIds() const {
 83:     std::vector<int> ids;
 84:     ids.reserve(nodes.size());
 85:     for (const auto& kv : nodes) ids.push_back(kv.first);
 86:     std::sort(ids.begin(), ids.end());
 87:     return ids;
 88: }
 89: 
 90: std::vector<int> Graph::edgeIds() const {
 91:     std::vector<int> ids;
 92:     ids.reserve(edges.size());
 93:     for (const auto& kv : edges) ids.push_back(kv.first);
 94:     std::sort(ids.begin(), ids.end());
 95:     return ids;
 96: }
 97: 
 98: int Graph::maxNodeId() const {
 99:     int best = -1;
100:     for (const auto& kv : nodes) best = std::max(best, kv.first);
101:     return best;
102: }
103: 
104: void Graph::resetBuildState() {
105:     for (auto& kv : nodes) kv.second.usedPorts = 0;
106:     for (auto& kv : edges) {
107:         kv.second.isBuilt = false;
108:         kv.second.isBackup = false;
109:     }
110: }

```

### Giải thích chi tiết từng dòng code:

#### 1. Cấu trúc dữ liệu bên trong lớp `Graph` (`include/Graph.h` dòng 28-34):
* `std::unordered_map<int, Node> nodes;`: Bảng băm lưu trữ các đỉnh của đồ thị, ánh xạ từ `nodeId -> Node`. Sử dụng bảng băm giúp tra cứu đỉnh trong thời gian $\mathcal{O}(1)$. Đặc biệt, nó hỗ trợ ID đỉnh thưa thớt (sparse ID), không bắt buộc ID phải liên tục từ 0 đến $N-1$.
* `std::unordered_map<int, Edge> edges;`: Bảng băm ánh xạ từ `edgeId -> Edge`.
* `std::unordered_map<int, std::vector<int>> adjacency;`: **Danh sách kề (Adjacency List)**. 
  - **Câu hỏi vấn đáp quan trọng:** *Tại sao danh sách kề lại lưu danh sách `edgeId` thay vì lưu `nodeId` kề?*
  - **Trả lời:** Trong mạng viễn thông thực tế, giữa 2 router có thể tồn tại **nhiều đường cáp song song (Parallel Links/Multi-edges)**, ví dụ một sợi cáp quang chính và một sợi cáp đồng dự phòng. Nếu chỉ lưu `nodeId`, ta không thể biết gói tin đang đi qua sợi cáp nào để tính băng thông và độ trễ. Lưu `edgeId` cho phép truy xuất chính xác $100\%$ thuộc tính của từng sợi cáp. Khi cần biết đỉnh đầu kia, ta chỉ việc gọi hàm `otherEndpoint(nodeId, edgeId)`.

#### 2. Các phương thức trong `src/Graph.cpp`:
* **Dòng 8-10 (`addNode`):**
  - `if (nodes.find(node.id) != nodes.end()) return false;`: Kiểm tra trùng lặp ID. Nếu đã tồn tại node với ID này thì từ chối thêm và trả về `false`.
  - `nodes[node.id] = node;`: Thêm node vào map và trả về `true`.
* **Dòng 12-21 (`addEdge`):**
  - Kiểm tra 3 điều kiện biên:
    1. `edges.find(edge.id) != edges.end()`: Trùng ID cạnh.
    2. `!hasNode(edge.u) || !hasNode(edge.v)`: Một trong hai đầu nút chưa tồn tại trong đồ thị.
    3. `edge.u == edge.v`: Cạnh tự khép vòng (Self-loop) nối một nút với chính nó.
  - Nếu vi phạm bất kỳ điều nào $\to$ Trả về `false`.
  - Nếu hợp lệ $\to$ Thêm vào map `edges` và cập nhật danh sách kề ở CẢ HAI ĐẦU:
    `adjacency[edge.u].push_back(edge.id);` và `adjacency[edge.v].push_back(edge.id);`.
* **Dòng 23-28 (`hasNode`, `hasEdge`):**
  - Dùng phương thức `.count(id)` của `unordered_map` để kiểm tra sự tồn tại trong $\mathcal{O}(1)$.
* **Dòng 30-48 (`getNode`, `getEdge`):**
  - Cung cấp cả 2 phiên bản `const` (chỉ đọc) và `non-const` (cho phép chỉnh sửa).
  - Bản `non-const` trả về tham chiếu trực tiếp (`Node&`, `Edge&`) để các thuật toán như Kruskal cập nhật số cổng `usedPorts`, hoặc Simulator cập nhật trạng thái `isUp`, `currentLoad`. Nếu không tìm thấy, hàm ném `std::runtime_error`.
* **Dòng 50-53 (`getNeighbors`):**
  - Trả về tham chiếu hằng (`const std::vector<int>&`) đến danh sách các `edgeId` nối với node. Trả về vector rỗng nếu node không tồn tại.
* **Dòng 55-61 (`otherEndpoint`):**
  - Nhận vào `nodeId` và `edgeId`. Lấy cạnh `e` tương ứng: nếu `nodeId == e.u` thì trả về `e.v`, ngược lại nếu `nodeId == e.v` thì trả về `e.u`. Nếu node không thuộc cạnh, trả về `-1`.
* **Dòng 63-71 (`buildAdjacency`):**
  - Xóa sạch danh sách kề và dựng lại từ đầu duyệt qua toàn bộ `edges`. Đồng thời sắp xếp các `edgeId` tăng dần để đảm bảo kết quả duyệt đồ thị luôn nhất quán tuyệt đối giữa các lần chạy.
* **Dòng 73-74 (`V`, `E`):**
  - Trả về số lượng đỉnh (`nodes.size()`) và số lượng cạnh (`edges.size()`).
* **Dòng 76-88 (`nodeIds`, `edgeIds`):**
  - Thu thập tất cả ID và sắp xếp tăng dần bằng `std::sort`.
* **Dòng 90-95 (`maxNodeId`):**
  - Tìm ID đỉnh lớn nhất trong đồ thị. Giá trị này được chuyển cho cấu trúc DSU khởi tạo mảng quản lý tập hợp rời nhau.
* **Dòng 97-104 (`resetBuildState`):**
  - Reset trạng thái xây dựng của đồ thị về ban đầu: đặt `usedPorts = 0` cho mọi node, `isBuilt = false` và `isBackup = false` cho mọi edge.

---

## 2.2. File `include/Validation.h` & `src/Validation.cpp`

### Toàn bộ mã nguồn Header (`include/Validation.h`):
```cpp
  1: #ifndef VALIDATION_H
  2: #define VALIDATION_H
  3: 
  4: #include <string>
  5: #include "Node.h"
  6: #include "Edge.h"
  7: #include "Packet.h"
  8: #include "Graph.h"
  9: 
 10: namespace Validation {
 11:     // Kiểm tra hợp lệ cho Node
 12:     bool isValidNode(const Node& node, std::string& reason);
 13:     
 14:     // Kiểm tra hợp lệ cho Edge.
 15:     bool isValidEdge(const Edge& edge, const Graph& graph, std::string& reason);
 16:     
 17:     // Kiểm tra hợp lệ cho Packet (sizeBytes > 0)
 18:     bool isValidPacket(const Packet& packet, std::string& reason);
 19: 
 20:     // Kiểm tra hợp lệ cho Packet kèm kiểm tra node nguồn và đích có tồn tại trong Graph
 21:     bool isValidPacket(const Packet& packet, const Graph& graph, std::string& reason);
 22: 
 23:     bool checkMaxLength(const Edge& edge);
 24:     bool checkPortCapacity(const Node& node);
 25:     bool isStaticFeasible(const Edge& edge);
 26: }
 27: 
 28: #endif // VALIDATION_H

```

### Toàn bộ mã nguồn Cài đặt (`src/Validation.cpp`):
```cpp
  1: #include "Validation.h"
  2: 
  3: namespace Validation {
  4: 
  5: bool isValidNode(const Node& node, std::string& reason) {
  6:     if (node.totalPorts <= 0) {
  7:         reason = "totalPorts phải lớn hơn 0";
  8:         return false;
  9:     }
 10:     if (node.processingTime < 0) {
 11:         reason = "processingTime không được âm";
 12:         return false;
 13:     }
 14:     return true;
 15: }
 16: 
 17: bool isValidEdge(const Edge& edge, const Graph& graph, std::string& reason) {
 18:     if (!graph.hasNode(edge.u)) {
 19:         reason = "Node u (" + std::to_string(edge.u) + ") không tồn tại";
 20:         return false;
 21:     }
 22:     
 23:     if (!graph.hasNode(edge.v)) {
 24:         reason = "Node v (" + std::to_string(edge.v) + ") không tồn tại";
 25:         return false;
 26:     }
 27: 
 28:     if (edge.length <= 0) {
 29:         reason = "length phải lớn hơn 0";
 30:         return false;
 31:     }
 32:     if (edge.bandwidthMbps <= 0) {
 33:         reason = "bandwidthMbps phải lớn hơn 0";
 34:         return false;
 35:     }
 36:     if (edge.mtu <= 0) {
 37:         reason = "mtu phải lớn hơn 0";
 38:         return false;
 39:     }
 40:     if (edge.currentLoad < 0 || edge.currentLoad > 1) {
 41:         reason = "currentLoad phải nằm trong khoảng [0, 1]";
 42:         return false;
 43:     }
 44:     return true;
 45: }
 46: 
 47: bool isValidPacket(const Packet& packet, std::string& reason) {
 48:     if (packet.sizeBytes <= 0) {
 49:         reason = "sizeBytes phải lớn hơn 0";
 50:         return false;
 51:     }
 52:     return true;
 53: }
 54: 
 55: bool isValidPacket(const Packet& packet, const Graph& graph, std::string& reason) {
 56:     if (!isValidPacket(packet, reason)) return false;
 57:     if (!graph.hasNode(packet.source)) {
 58:         reason = "Node nguồn (" + std::to_string(packet.source) + ") không tồn tại trong đồ thị";
 59:         return false;
 60:     }
 61:     if (!graph.hasNode(packet.destination)) {
 62:         reason = "Node đích (" + std::to_string(packet.destination) + ") không tồn tại trong đồ thị";
 63:         return false;
 64:     }
 65:     return true;
 66: }
 67: 
 68: bool checkMaxLength(const Edge& edge) {
 69:     return edge.length <= edge.maxSegmentLength;
 70: }
 71: 
 72: bool checkPortCapacity(const Node& node) {
 73:     return node.usedPorts < node.totalPorts;
 74: }
 75: 
 76: bool isStaticFeasible(const Edge& edge) {
 77:     return checkMaxLength(edge);
 78: }
 79: 
 80: }

```

### Giải thích chi tiết từng dòng code:
* **Mục đích:** Đảm bảo toàn vẹn dữ liệu trước khi thuật toán tiếp nhận. Ngăn ngừa mọi dữ liệu rác hoặc giá trị phi lý.
* **Hàm `isValidNode(const Node& node, std::string& reason)` (Dòng 5-14 trong .cpp):**
  - `node.totalPorts <= 0`: Router hoặc Host bắt buộc phải có ít nhất 1 cổng mạng. Nếu $\le 0$, loại bỏ với lý do `"totalPorts phải lớn hơn 0"`.
  - `node.processingTime < 0`: Thời gian CPU xử lý không thể âm.
* **Hàm `isValidEdge(const Edge& edge, const Graph& graph, std::string& reason)` (Dòng 16-47 trong .cpp):**
  - `!graph.hasNode(edge.u)` và `!graph.hasNode(edge.v)`: Hai đầu nút của sợi cáp bắt buộc phải tồn tại trong đồ thị. Kiểm tra bằng `hasNode()` trong $\mathcal{O}(1)$, không dùng `try-catch` nuốt lỗi.
  - `edge.length <= 0`: Chiều dài cáp vật lý phải $> 0$.
  - `edge.bandwidthMbps <= 0`: Băng thông phải $> 0$ (tránh lỗi chia cho 0 khi tính trễ truyền dẫn).
  - `edge.mtu <= 0`: Kích thước MTU phải dương.
  - `edge.currentLoad < 0 || edge.currentLoad > 1`: Tải trọng cáp phải nằm trong đoạn $[0, 1]$ ($0\% - 100\%$).
* **Hàm `isValidPacket(const Packet& packet, std::string& reason)` (Dòng 49-55):**
  - `packet.sizeBytes <= 0`: Gói tin phải có dung lượng $> 0$.
* **Hàm `isValidPacket(const Packet& packet, const Graph& graph, std::string& reason)` (Dòng 57-68):**
  - Kiểm tra thêm tính hợp lệ trong không gian đồ thị: Node nguồn `packet.source` và node đích `packet.destination` đều phải tồn tại trong đồ thị.
* **Hàm `checkMaxLength(const Edge& edge)` và `isStaticFeasible(edge)` (Dòng 70-72, 78-80):**
  - `edge.length <= edge.maxSegmentLength`: Kiểm tra tính khả thi vật lý tĩnh. Nếu chiều dài vượt quá cự ly truyền tối đa của loại cáp đó (ví dụ cáp đồng vượt quá 100m) $\to$ Trả về `false` để Kruskal loại bỏ cạnh này ngay từ đầu.
* **Hàm `checkPortCapacity(const Node& node)` (Dòng 74-76):**
  - `node.usedPorts < node.totalPorts`: Kiểm tra thiết bị còn cổng trống để cắm dây hay không.

---

## 2.3. File `include/IOManager.h` & `src/IOManager.cpp`

### Toàn bộ mã nguồn Header (`include/IOManager.h`):
```cpp
  1: #ifndef IOMANAGER_H
  2: #define IOMANAGER_H
  3: 
  4: #include <string>
  5: #include <vector>
  6: #include "Graph.h"
  7: #include "Packet.h"
  8: #include "Scenario.h"
  9: #include "Types.h"
 10: 
 11: class IOManager {
 12: public:
 13:     // Đọc danh sách Node từ file
 14:     static bool loadNodes(const std::string& filepath, Graph& graph);
 15:     
 16:     // Đọc danh sách Edge từ file
 17:     static bool loadEdges(const std::string& filepath, Graph& graph);
 18:     
 19:     // Đọc danh sách Packet từ file (có thể truyền Graph để kiểm tra node nguồn/đích)
 20:     static bool loadPackets(const std::string& filepath, std::vector<Packet>& packets, const Graph* graph = nullptr);
 21:     
 22:     // Đọc kịch bản sự kiện (Scenario) từ file (Task B4.3, B4.7)
 23:     static bool loadScenario(const std::string& filepath, Scenario& scenario);
 24:     
 25:     // Kiểm tra xem dòng có nên bỏ qua hay không (dòng trống hoặc bắt đầu bằng '#')
 26:     static bool isSkippableLine(const std::string& line);
 27:     
 34: };
 35: 
 36: #endif 

```

### Toàn bộ mã nguồn Cài đặt (`src/IOManager.cpp`):
```cpp
  1: #include "IOManager.h"
  2: #include "Validation.h"
  3: #include "EnumUtils.h"
  4: #include <fstream>
  5: #include <sstream>
  6: #include <iostream>
  7: #include <stdexcept>
  8: #include <cctype>
  9: 
 10: bool IOManager::isSkippableLine(const std::string& line) {
 11:     if (line.empty()) return true;
 12:     for (char c : line) {
 13:         if (!std::isspace(c)) {
 14:             if (c == '#') return true;
 15:             return false;
 16:         }
 17:     }
 18:     return true;
 19: }
 20: 
 21: NodeType IOManager::stringToNodeType(const std::string& str) {
 22:     NodeType type;
 23:     if (EnumUtils::parseNodeType(str, type)) return type;
 24:     throw std::invalid_argument("Unknown NodeType: " + str);
 25: }
 26: 
 27: MediaType IOManager::stringToMediaType(const std::string& str) {
 28:     MediaType media;
 29:     if (EnumUtils::parseMediaType(str, media)) return media;
 30:     throw std::invalid_argument("Unknown MediaType: " + str);
 31: }
 32: 
 33: Protocol IOManager::stringToProtocol(const std::string& str) {
 34:     Protocol proto;
 35:     if (EnumUtils::parseProtocol(str, proto)) return proto;
 36:     throw std::invalid_argument("Unknown Protocol: " + str);
 37: }
 38: 
 39: PacketType IOManager::stringToPacketType(const std::string& str) {
 40:     PacketType pktType;
 41:     if (EnumUtils::parsePacketType(str, pktType)) return pktType;
 42:     throw std::invalid_argument("Unknown PacketType: " + str);
 43: }
 44: 
 45: EventType IOManager::stringToEventType(const std::string& str) {
 46:     EventType evType;
 47:     if (EnumUtils::parseEventType(str, evType)) return evType;
 48:     throw std::invalid_argument("Unknown EventType: " + str);
 49: }
 50: 
 51: bool IOManager::loadNodes(const std::string& filepath, Graph& graph) {
 52:     std::ifstream file(filepath);
 53:     if (!file.is_open()) {
 54:         std::cerr << "Lỗi: Không thể mở file " << filepath << "\n";
 55:         return false;
 56:     }
 57: 
 58:     std::string line;
 59:     int lineCount = 0;
 60:     while (std::getline(file, line)) {
 61:         lineCount++;
 62:         if (isSkippableLine(line)) continue;
 63: 
 64:         std::stringstream ss(line);
 65:         Node node;
 66:         std::string typeStr;
 67: 
 68:         // Định dạng file: id name type totalPorts processingTime
 69:         if (!(ss >> node.id >> node.name >> typeStr >> node.totalPorts >> node.processingTime)) {
 70:             std::cerr << "Lỗi sai format dòng " << lineCount << " trong " << filepath << "\n";
 71:             continue;
 72:         }
 73: 
 74:         try {
 75:             node.type = stringToNodeType(typeStr);
 76:         } catch (const std::exception& e) {
 77:             std::cerr << "Lỗi dữ liệu Enum dòng " << lineCount << ": " << e.what() << "\n";
 78:             continue;
 79:         }
 80: 
 81:         std::string reason;
 82:         if (!Validation::isValidNode(node, reason)) {
 83:             std::cerr << "REJECT Node ID " << node.id << ": " << reason << "\n";
 84:             continue;
 85:         }
 86: 
 87:         if (!graph.addNode(node)) {
 88:             std::cerr << "REJECT Node ID " << node.id << ": ID đã tồn tại trong đồ thị\n";
 89:             continue;
 90:         }
 91:     }
 92:     return true;
 93: }
 94: 
 95: bool IOManager::loadEdges(const std::string& filepath, Graph& graph) {
 96:     std::ifstream file(filepath);
 97:     if (!file.is_open()) {
 98:         std::cerr << "Lỗi: Không thể mở file " << filepath << "\n";
 99:         return false;
100:     }
101: 
102:     std::string line;
103:     int lineCount = 0;
104:     while (std::getline(file, line)) {
105:         lineCount++;
106:         if (isSkippableLine(line)) continue;
107: 
108:         std::stringstream ss(line);
109:         Edge edge;
110:         std::string mediaStr;
111: 
112:         // Định dạng mới (11 cột): id u v media length bw maxSeg terrain mtu load delay
113:         if (!(ss >> edge.id >> edge.u >> edge.v >> mediaStr >> edge.length 
114:                  >> edge.bandwidthMbps >> edge.maxSegmentLength 
115:                  >> edge.terrainFactor 
116:                  >> edge.mtu >> edge.currentLoad >> edge.queueDelay)) {
117:             std::cerr << "Lỗi sai format dòng " << lineCount << " trong " << filepath << "\n";
118:             continue;
119:         }
120: 
121:         try {
122:             edge.mediaType = stringToMediaType(mediaStr);
123:         } catch (const std::exception& e) {
124:             std::cerr << "Lỗi dữ liệu Enum dòng " << lineCount << ": " << e.what() << "\n";
125:             continue;
126:         }
127: 
128:         std::string reason;
129:         if (!Validation::isValidEdge(edge, graph, reason)) {
130:             std::cerr << "REJECT Edge ID " << edge.id << ": " << reason << "\n";
131:             continue;
132:         }
133: 
134:         if (!graph.addEdge(edge)) {
135:             std::cerr << "REJECT Edge ID " << edge.id << ": Cạnh trùng ID hoặc tự nối chính nó\n";
136:             continue;
137:         }
138:     }
139:     return true;
140: }
141: 
142: bool IOManager::loadPackets(const std::string& filepath, std::vector<Packet>& packets, const Graph* graph) {
143:     std::ifstream file(filepath);
144:     if (!file.is_open()) {
145:         std::cerr << "Lỗi: Không thể mở file " << filepath << "\n";
146:         return false;
147:     }
148: 
149:     std::string line;
150:     int lineCount = 0;
151:     while (std::getline(file, line)) {
152:         lineCount++;
153:         if (isSkippableLine(line)) continue;
154: 
155:         std::stringstream ss(line);
156:         Packet pkt;
157:         std::string typeStr, protoStr;
158:         int canFrag, newConn;
159: 
160:         // Định dạng: id src dest size type protocol canFragment newConnection
161:         if (!(ss >> pkt.id >> pkt.source >> pkt.destination >> pkt.sizeBytes 
162:                  >> typeStr >> protoStr >> canFrag >> newConn)) {
163:             std::cerr << "Lỗi sai format dòng " << lineCount << " trong " << filepath << "\n";
164:             continue;
165:         }
166: 
167:         try {
168:             pkt.type = stringToPacketType(typeStr);
169:             pkt.protocol = stringToProtocol(protoStr);
170:         } catch (const std::exception& e) {
171:             std::cerr << "Lỗi dữ liệu Enum dòng " << lineCount << ": " << e.what() << "\n";
172:             continue;
173:         }
174:         
175:         pkt.canFragment = (canFrag != 0);
176:         pkt.newConnection = (newConn != 0);
177: 
178:         std::string reason;
179:         bool valid = graph ? Validation::isValidPacket(pkt, *graph, reason)
180:                            : Validation::isValidPacket(pkt, reason);
181:         if (!valid) {
182:             std::cerr << "REJECT Packet ID " << pkt.id << ": " << reason << "\n";
183:             continue;
184:         }
185: 
186:         packets.push_back(pkt);
187:     }
188:     return true;
189: }
190: 
191: bool IOManager::loadScenario(const std::string& filepath, Scenario& scenario) {
192:     std::ifstream file(filepath);
193:     if (!file.is_open()) {
194:         std::cerr << "Lỗi: Không thể mở file " << filepath << "\n";
195:         return false;
196:     }
197: 
198:     // Gán tên kịch bản từ tên file
199:     scenario.name = filepath;
200:     scenario.events.clear();
201: 
202:     std::string line;
203:     int lineCount = 0;
204:     while (std::getline(file, line)) {
205:         lineCount++;
206:         if (isSkippableLine(line)) continue;
207: 
208:         std::stringstream ss(line);
209:         ScenarioEvent ev;
210:         std::string evTypeStr;
211: 
212:         // Định dạng file: [Time] [EventType] [TargetEdgeId] [Value]
213:         if (!(ss >> ev.time >> evTypeStr >> ev.targetId >> ev.value)) {
214:             std::cerr << "Lỗi sai format dòng " << lineCount << " trong " << filepath << "\n";
215:             continue;
216:         }
217: 
218:         try {
219:             ev.eventType = stringToEventType(evTypeStr);
220:         } catch (const std::exception& e) {
221:             std::cerr << "Lỗi dữ liệu Enum dòng " << lineCount << ": " << e.what() << "\n";
222:             continue;
223:         }
224: 
225:         scenario.events.push_back(ev);
226:     }
227:     return true;
228: }

```

### Giải thích chi tiết từng dòng code:
* **Hàm `isSkippableLine(const std::string& line)` (Dòng 10-20 trong .cpp):**
  - Bỏ qua các dòng trống hoặc dòng bắt đầu bằng ký tự comment `#`. Giúp file dữ liệu có thể ghi chú giải thích rõ ràng mà không gây lỗi parser.
* **Các hàm `stringToNodeType`, `stringToMediaType`, `stringToProtocol`, `stringToPacketType`, `stringToEventType` (Dòng 22-53):**
  - Chuyển tiếp (delegate) sang bộ phân giải chuẩn `EnumUtils::parseXxx()`. Nếu chuỗi không nhận dạng được, ném ngoại lệ `std::invalid_argument` kèm thông báo chi tiết.
* **Hàm `loadNodes(const std::string& filepath, Graph& graph)` (Dòng 55-94):**
  - Mở file bằng `std::ifstream`. Đọc từng dòng qua `std::getline`.
  - Tách các trường bằng `std::stringstream`: `id`, `name`, `typeStr`, `totalPorts`, `processingTime`.
  - Bắt lỗi sai định dạng số/chuỗi hoặc sai Enum qua `try-catch`.
  - Gọi `Validation::isValidNode()`. Nếu sai $\to$ In `REJECT Node ID ...` ra `std::cerr` và bỏ qua.
  - Gọi `graph.addNode(node)`. Nếu trả về `false` (trùng ID) $\to$ In cảnh báo `REJECT` rõ ràng.
* **Hàm `loadEdges(const std::string& filepath, Graph& graph)` (Dòng 96-141):**
  - Đọc 11 trường thông số kỹ thuật vật lý của cạnh: `id`, `u`, `v`, `mediaStr`, `length`, `bw`, `maxSeg`, `terrain`, `mtu`, `load`, `delay`. (Toàn bộ đơn giá được tách riêng sang `data/pricing.txt` để bảo đảm nguyên tắc Single Source of Truth).
  - Gọi `Validation::isValidEdge(edge, graph)`. Nếu không hợp lệ $\to$ Bỏ qua.
  - Gọi `graph.addEdge(edge)`.
* **Hàm `loadPackets(const std::string& filepath, std::vector<Packet>& packets, const Graph* graph)` (Dòng 143-191):**
  - Đọc thông số gói tin: `id`, `source`, `destination`, `sizeBytes`, `typeStr`, `protoStr`, `canFrag`, `newConn`.
  - Nếu có truyền con trỏ `graph`, kiểm tra node nguồn và đích có tồn tại trong mạng hay không. Nếu hợp lệ mới `packets.push_back(pkt)`.
* **Hàm `loadScenario(const std::string& filepath, Scenario& scenario)` (Dòng 193-231):**
  - Đọc kịch bản biến cố động theo thời gian: `[Time] [EventType] [TargetEdgeId] [Value]`.
  - Lưu vào `scenario.events` để bộ mô phỏng `Simulator` thực thi.

---

# CHƯƠNG 3: QUY HOẠCH HẠ TẦNG CÁP MẠNG (BÀI TOÁN VẬT LÝ – KRUSKAL & MST & CÁP DỰ PHÒNG)

Chương này giải quyết bài toán kinh tế - kỹ thuật trong thiết kế mạng viễn thông: Làm thế nào để nối thông suốt toàn bộ $V$ tòa nhà/router với **tổng chi phí lắp đặt thấp nhất** (Cây khung nhỏ nhất - Minimum Spanning Tree), đồng thời thỏa mãn các ràng buộc cự ly truyền dẫn, giới hạn số cổng thiết bị, và quy hoạch thêm các tuyến cáp dự phòng (Backup Links) chống sập mạng.

---

## 3.1. File `include/DSU.h` & `src/DSU.cpp`

### Toàn bộ mã nguồn Header (`include/DSU.h`):
```cpp
  1: #ifndef DSU_H
  2: #define DSU_H
  3: 
  4: #include <vector>
  5: 
  6: // ============================================================
  7: // DSU (Disjoint Set Union / Union-Find).
  8: // Kruskal dùng để trả lời nhanh: "hai node đã cùng một thành phần chưa?"
  9: // Nếu rồi (cùng root) thì nối thêm một cạnh sẽ tạo CHU TRÌNH => bỏ qua.
 10: //
 11: // Hai kỹ thuật làm nó gần như O(1):
 12: //   - Path compression : khi find(x), trỏ mọi node trên đường đi thẳng lên root.
 13: //   - Union by rank    : gắn cây thấp vào cây cao, giữ cây không bị cao.
 14: //
 15: // Phần tử là số nguyên 0..n-1. Với đồ thị có id node lớn nhất là M thì tạo DSU(M + 1).
 16: // ============================================================
 17: class DSU {
 18: public:
 19:     explicit DSU(int n = 0) { reset(n); }
 20: 
 21:     // Khởi tạo lại n phần tử rời rạc: mỗi node là root của chính nó
 22:     void reset(int n);
 23: 
 24:     // Root của tập chứa x (có path compression nên không const)
 25:     int find(int x);
 26: 
 27:     // Gộp tập chứa a và b.
 28:     // Trả true  nếu a, b trước đó KHÁC tập (đã gộp),
 29:     //     false nếu đã cùng tập (không làm gì - gộp nữa sẽ tạo chu trình).
 30:     bool unite(int a, int b);
 31: 
 32:     // Hai phần tử có cùng tập không
 33:     bool connected(int a, int b) { return find(a) == find(b); }
 34: 
 35:     // Số tập (thành phần liên thông) hiện tại
 36:     int components() const { return components_; }
 37: 
 38:     int size() const { return static_cast<int>(parent_.size()); }
 39: 
 40: private:
 41:     std::vector<int> parent_;
 42:     std::vector<int> rank_;
 43:     int              components_ = 0;
 44: };
 45: 
 46: #endif // DSU_H

```

### Toàn bộ mã nguồn Cài đặt (`src/DSU.cpp`):
```cpp
  1: #include "DSU.h"
  2: #include <utility>
  3: 
  4: void DSU::reset(int n) {
  5:     parent_.assign(n, 0);
  6:     rank_.assign(n, 0);
  7:     for (int i = 0; i < n; ++i) parent_[i] = i;   // mỗi phần tử là root của chính nó
  8:     components_ = n;
  9: }
 10: 
 11: int DSU::find(int x) {
 12:     // Bước 1: leo lên tìm root
 13:     int root = x;
 14:     while (parent_[root] != root) root = parent_[root];
 15:     // Bước 2 (path compression): trỏ mọi node trên đường đi thẳng vào root
 16:     while (parent_[x] != root) {
 17:         int next = parent_[x];
 18:         parent_[x] = root;
 19:         x = next;
 20:     }
 21:     return root;
 22: }
 23: 
 24: bool DSU::unite(int a, int b) {
 25:     int ra = find(a);
 26:     int rb = find(b);
 27:     if (ra == rb) return false;                  // đã cùng tập: không gộp
 28:     if (rank_[ra] < rank_[rb]) std::swap(ra, rb); // ra là cây cao hơn (hoặc bằng)
 29:     parent_[rb] = ra;                            // gắn cây thấp vào cây cao
 30:     if (rank_[ra] == rank_[rb]) ++rank_[ra];     // chỉ khi bằng nhau chiều cao mới tăng
 31:     --components_;
 32:     return true;
 33: }

```

### Giải thích chi tiết từng dòng code:

#### 1. Nguyên lý cấu trúc dữ liệu Disjoint Set Union (DSU / Tập hợp rời rạc):
DSU được sử dụng trong thuật toán Kruskal để kiểm tra xem việc thêm một cạnh $(u, v)$ có tạo thành **chu trình (Cycle)** hay không trong thời gian gần như hằng số $\mathcal{O}(\alpha(V))$ (hàm Ackermann nghịch đảo, với mọi đồ thị thực tế $\alpha(V) \le 4$).

#### 2. Các phương thức trong `src/DSU.cpp`:
* **Dòng 4-9 (`reset`):**
  - Khởi tạo $N$ phần tử riêng biệt.
  - `parent_.assign(n, 0);`: Mảng lưu cha của mỗi phần tử. Ban đầu mỗi nút là cha của chính nó: `parent_[i] = i`.
  - `rank_.assign(n, 0);`: Mảng lưu chiều cao ước tính của cây (rank). Ban đầu đều bằng 0.
  - `components_ = n;`: Ban đầu có đúng $N$ thành phần liên thông rời rạc.
* **Dòng 11-22 (`find` - Tìm gốc đại diện kèm Nén đường đi / Path Compression):**
  - **Bước 1 (Dòng 13-14):** Leo ngược lên theo các nút cha `while (parent_[root] != root) root = parent_[root];` cho đến khi tìm được nút gốc `root` cao nhất.
  - **Bước 2 (Dòng 16-20 - Nén đường đi / Path Compression):** Duyệt lại đường đi từ $x$ lên gốc và trỏ trực tiếp nút cha của mọi node trên đường đi vào thẳng `root`.
  - **Ý nghĩa kỹ thuật:** Nhờ kỹ thuật nén đường đi, cây bị làm phẳng hoàn toàn (chiều cao xấp xỉ 1). Lần gọi `find` tiếp theo trên các nút này sẽ chỉ tốn đúng 1 bước nhảy.
* **Dòng 24-33 (`unite` - Hợp nhất hai tập hợp theo Rank / Union by Rank):**
  - `int ra = find(a); int rb = find(b);`: Tìm gốc đại diện của hai đỉnh $a$ và $b$.
  - `if (ra == rb) return false;`: Nếu cùng gốc đại diện, nghĩa là $a$ và $b$ đã thông nhau từ trước. Thêm cạnh nối $a-b$ nữa sẽ **tạo thành chu trình** $\to$ Trả về `false`.
  - **Gộp theo Rank (Dòng 28-30):**
    - `if (rank_[ra] < rank_[rb]) std::swap(ra, rb);`: Đảm bảo $ra$ luôn là gốc của cây có chiều cao lớn hơn hoặc bằng.
    - `parent_[rb] = ra;`: Luôn gắn gốc cây thấp hơn ($rb$) vào làm con của cây cao hơn ($ra$). Điều này ngăn chặn hiện tượng cây bị suy biến thành danh sách liên kết.
    - `if (rank_[ra] == rank_[rb]) ++rank_[ra];`: Chỉ khi hai cây có chiều cao bằng nhau thì sau khi gộp chiều cao mới tăng thêm 1 đơn vị.
  - `--components_;`: Số thành phần liên thông giảm đi 1. Trả về `true` (gộp thành công).

---

## 3.2. File include/CostModel.h & src/CostModel.cpp

### Toàn bộ mã nguồn Header (include/CostModel.h):
`cpp
  1: #ifndef COSTMODEL_H
  2: #define COSTMODEL_H
  3: 
  4: #include "Edge.h"
  5: 
  6: #include <map>
  7: #include <string>
  8: 
  9: struct CostBreakdown {
 10:     double install;     
 11:     double material;      // Chi phí vật tư (chiều dài * đơn giá * hệ số địa hình)
 12:     double equipment;     
 13:     double maintenance;   
 14:     double total;         
 15: };
 16: 
 17: // Cấu trúc lưu trạng thái bảng giá nạp từ pricing.txt (khởi tạo rỗng đồng bộ)
 18: struct PricingConfig {
 19:     double installationCost = 0.0;
 20:     std::map<MediaType, double> unitPrice;
 21:     std::map<MediaType, double> equipmentCost;
 22:     std::map<MediaType, double> maintenanceCost;
 23: };
 24: 
 25: class CostModel {
 26: private:
 27:     PricingConfig config; // Thuộc tính private: Đảm bảo đóng gói tuyệt đối
 28: public:
 29:     // Constructor mặc định (sẽ dùng cấu hình mặc định trong PricingConfig)
 30:     CostModel() = default;
 31: 
 32:     // Nạp cấu hình từ file text
 33:     bool loadPricing(const std::string& filepath);
 34: 
 35:     // Tính toán chi phí - Hàm const vì nó không làm thay đổi trạng thái của model
 36:     CostBreakdown calculateCablingCost(const Edge& edge) const;
 37: };
 38: 
 39: #endif 
`

### Toàn bộ mã nguồn Cài đặt (src/CostModel.cpp):
`cpp
  1: #include "CostModel.h"
  2: #include "Config.h"
  3: #include "EnumUtils.h"
  4: #include <fstream>
  5: #include <sstream>
  6: #include <iostream>
  7: #include <stdexcept>
  8: 
  9: bool CostModel::loadPricing(const std::string& filepath) {
 10:     std::ifstream file(filepath);
 11:     if (!file.is_open()) {
 12:         std::cerr << "[CostModel] Canh bao: Khong the mo " << filepath << ". Su dung thong so mac dinh tu danh sach canh.\n";
 13:         return false;
 14:     }
 15: 
 16:     std::string line;
 17:     int lineCount = 0;
 18:     while (std::getline(file, line)) {
 19:         lineCount++;
 20:         if (line.empty() || line[0] == '#') {
 21:             continue;
 22:         }
 23: 
 24:         std::stringstream ss(line);
 25:         std::string key;
 26:         ss >> key;
 27: 
 28:         if (key == "INSTALLATION_COST") {
 29:             if (!(ss >> config.installationCost)) {
 30:                 std::cerr << "[CostModel] Canh bao dong " << lineCount << ": Sai format INSTALLATION_COST trong " << filepath << ".\n";
 31:             }
 32:         } 
 33:         else if (key == "UNIT_PRICE" || key == "EQUIPMENT_COST" || key == "MAINTENANCE_COST") {
 34:             std::string typeStr;
 35:             double val;
 36:             if (ss >> typeStr >> val) {
 37:                 MediaType media;
 38:                 if (!EnumUtils::parseMediaType(typeStr, media)) {
 39:                     std::cerr << "[CostModel] Canh bao dong " << lineCount 
 40:                               << ": Loai cap khong hop le '" << typeStr 
 41:                               << "' trong " << filepath << ". Bo qua dong nay.\n";
 42:                     continue;
 43:                 }
 44:                 if (key == "UNIT_PRICE") {
 45:                     config.unitPrice[media] = val;
 46:                 } else if (key == "EQUIPMENT_COST") {
 47:                     config.equipmentCost[media] = val;
 48:                 } else {
 49:                     config.maintenanceCost[media] = val;
 50:                 }
 51:             } else {
 52:                 std::cerr << "[CostModel] Canh bao dong " << lineCount 
 53:                           << ": Sai format " << key << " trong " << filepath << ". Bo qua dong nay.\n";
 54:             }
 55:         }
 56:     }
 57:     std::cout << "[CostModel] Nhan cau hinh gia thanh cong tu: " << filepath << "\n";
 58:     return true;
 59: }
 60: 
 61: CostBreakdown CostModel::calculateCablingCost(const Edge& edge) const {
 62:     CostBreakdown breakdown;
 63:     
 64:     // Ràng buộc: "dùng INF cho cạnh không đi được"
 65:     if (!edge.isUp) {
 66:         breakdown.install = breakdown.material = breakdown.equipment = breakdown.maintenance = 0.0;
 67:         breakdown.total = Config::INF;
 68:         return breakdown;
 69:     }
 70: 
 71:     // 1. Kiểm tra loại cáp đã được định giá trong pricing.txt chưa
 72:     if (config.unitPrice.count(edge.mediaType) == 0 ||
 73:         config.equipmentCost.count(edge.mediaType) == 0 ||
 74:         config.maintenanceCost.count(edge.mediaType) == 0) {
 75:         breakdown.install = breakdown.material = breakdown.equipment = breakdown.maintenance = 0.0;
 76:         breakdown.total = Config::INF;
 77:         return breakdown;
 78:     }
 79: 
 80:     // 2. Chi phí lắp đặt cơ bản
 81:     breakdown.install = config.installationCost;
 82:     
 83:     // 3. Chi phí vật tư cáp = Chiều dài * Đơn giá từ pricing.txt * Hệ số địa hình
 84:     double currentUnitPrice = config.unitPrice.at(edge.mediaType);
 85:     breakdown.material = edge.length * currentUnitPrice * edge.terrainFactor;
 86:     
 87:     // 4. Phí thiết bị cổng từ pricing.txt
 88:     breakdown.equipment = config.equipmentCost.at(edge.mediaType);
 89:     
 90:     // 5. Phí bảo trì định kỳ từ pricing.txt
 91:     breakdown.maintenance = config.maintenanceCost.at(edge.mediaType);
 92:     
 93:     // 6. Tổng tất cả các chi phí
 94:     breakdown.total = breakdown.install + breakdown.material + breakdown.equipment + breakdown.maintenance;
 95:     
 96:     return breakdown;
 97: }
`

### Giải thích chi tiết từng dòng code:

#### 1. Cấu trúc bảng giá PricingConfig & CostBreakdown:
* CostBreakdown (include/CostModel.h dòng 9-15): Cung cấp chi tiết 4 thành phần chi phí của một đường cáp: install, material, equipment, maintenance và 	otal.
* PricingConfig (include/CostModel.h dòng 18-23): Khởi tạo rỗng đồng bộ. Toàn bộ đơn giá được nạp trực tiếp và duy nhất từ file cấu hình thị trường data/pricing.txt (Single Source of Truth), không gán ngầm giá trị trong mã nguồn.

#### 2. Các phương thức trong src/CostModel.cpp:
* **Dòng 9-57 (loadPricing):**
  - Mở file data/pricing.txt. Nếu không mở được, in cảnh báo ra std::cerr và trả về alse.
  - Bóc tách từng dòng qua std::stringstream: nhận diện từ khóa INSTALLATION_COST, UNIT_PRICE, EQUIPMENT_COST, MAINTENANCE_COST.
  - Kiểm tra tính hợp lệ của chuỗi loại cáp qua EnumUtils::parseMediaType. Nếu gõ sai chính tả, cảnh báo rõ số dòng và bỏ qua.
* **Dòng 59-99 (calculateCablingCost - Nguyên tắc Nguồn giá duy nhất):**
  - Kiểm tra trạng thái vật lý: if (!edge.isUp) return breakdown với total = Config::INF; (cáp hỏng thì chi phí là vô cực để thuật toán né tránh).
  - **Kiểm tra định giá:** Nếu loại cáp này chưa từng được định giá trong pricing.txt (chưa có trong unitPrice, equipmentCost hoặc maintenanceCost), lập tức trả về 	otal = Config::INF để từ chối thi công, đảm bảo tính minh bạch tài chính tuyệt đối, không lấy lôm côm từ edges.txt.
  - Áp dụng công thức tính giá theo đúng bảng giá thị trường pricing.txt:
    \text{material} = \text{edge.length} \times \text{config.unitPrice.at(edge.mediaType)} \times \text{edge.terrainFactor}
    \text{total} = \text{install} + \text{material} + \text{equipment} + \text{maintenance}

---

## 3.3. File `include/Kruskal.h` & `src/Kruskal.cpp`

### Toàn bộ mã nguồn Header (`include/Kruskal.h`):
```cpp
  1: #ifndef KRUSKAL_H
  2: #define KRUSKAL_H
  3: 
  4: #include <functional>
  5: #include <vector>
  6: #include "Graph.h"
  7: #include "DSU.h"
  8: 
  9: // ============================================================
 10: // Kruskal.h - Giai đoạn XÂY HẠ TẦNG (bước 1-4 của quy trình).
 11: //
 12: //   1. Sắp xếp các dây ứng viên theo chi phí lắp đặt tăng dần
 13: //   2. Duyệt từng dây, chỉ chọn nếu:
 14: //        - dây đang hoạt động (isUp) và không vượt độ dài tối đa của loại cáp
 15: //        - không tạo chu trình (DSU)
 16: //        - hai đầu còn cổng trống (usedPorts < totalPorts)
 17: //   3. Chọn xong: usedPorts hai đầu +1, isBuilt = true
 18: //   4. Thêm k dây DỰ PHÒNG để mạng có đường thay thế khi dây đứt
 19: //
 20: // LƯU Ý KHI BẢO VỆ: vì có ràng buộc cổng, kết quả là THAM LAM CÓ RÀNG BUỘC
 21: // (degree-constrained greedy), không còn là MST thuần túy. Nếu không có ràng
 22: // buộc cổng thì đúng là MST tối ưu của Kruskal.
 23: // ============================================================
 24: 
 25: // Hàm tính chi phí lắp đặt của một dây (đơn vị ĐV).
 26: // Kruskal không tự biết công thức, để nhóm đổi được công thức mà không sửa thuật toán.
 27: // Khi CostModel (bạn B) xong thì truyền hàm calculateCablingCost vào đây.
 28: using CostFn = std::function<double(const Edge&)>;
 29: 
 30: // Công thức tạm dùng khi chưa có CostModel:
 31: //   length * unitPrice * terrainFactor + equipmentCost + maintenanceCost
 32: // (chưa có installationCost vì struct Edge hiện tại không có field này)
 33: double defaultCablingCost(const Edge& e);
 34: 
 35: // Số dây dự phòng mặc định
 36: constexpr int DEFAULT_BACKUP_LINKS = 2;
 37: 
 38: // Kết quả xây cây khung (khớp task board mục 3.6)
 39: struct MSTResult {
 40:     std::vector<int> edgeIds;          // các edge được chọn, theo thứ tự chọn
 41:     double           totalCost = 0.0;  // tổng chi phí lắp đặt các edge đó (không tính thiết bị)
 42:     bool             connected = false; // true nếu nối được TẤT CẢ node (đủ V-1 cạnh)
 43: };
 44: 
 45: // Lý do một edge bị loại (dùng để giải thích trong báo cáo)
 46: enum class RejectReason { NONE, LINK_DOWN, TOO_LONG, CYCLE, NO_PORT };
 47: const char* toString(RejectReason r);
 48: 
 49: struct Rejection {
 50:     int          edgeId;
 51:     RejectReason reason;
 52: };
 53: 
 54: // So sánh hai edge để sắp xếp: chi phí tăng dần, bằng nhau thì edgeId nhỏ hơn trước.
 55: // Tie-breaker theo id giúp hai lần chạy cùng dữ liệu cho CÙNG kết quả.
 56: bool compareEdges(const Edge& a, const Edge& b, const CostFn& cost = defaultCablingCost);
 57: 
 58: // Kiểm tra một edge có chọn được lúc này không. Trả NONE nếu được, ngược lại là lý do.
 59: // Thứ tự kiểm tra: down -> quá dài -> chu trình -> hết cổng.
 60: // (dsu không const vì find() có path compression)
 61: RejectReason checkEdge(const Graph& g, const Edge& e, DSU& dsu);
 62: inline bool canSelectEdge(const Graph& g, const Edge& e, DSU& dsu) {
 63:     return checkEdge(g, e, dsu) == RejectReason::NONE;
 64: }
 65: 
 66: // Xây cây khung. Reset trạng thái xây dựng của g trước, nên gọi lại nhiều lần vẫn đúng.
 67: // Cập nhật g: usedPorts của node, isBuilt của edge.
 68: // rejections (tùy chọn): nếu khác nullptr sẽ nhận danh sách edge bị loại + lý do.
 69: MSTResult buildMST(Graph& g, const CostFn& cost = defaultCablingCost,
 70:                    std::vector<Rejection>* rejections = nullptr);
 71: 
 72: // Kết quả chọn dây dự phòng
 73: struct BackupResult {
 74:     std::vector<int> edgeIds;              // các edge dự phòng đã chọn
 75:     double           totalCost = 0.0;      // tổng chi phí lắp đặt các dây dự phòng
 76:     int              treeEdges = 0;        // số cạnh trong cây
 77:     int              protectedTreeEdges = 0; // số cạnh cây CÓ đường thay thế sau khi thêm backup
 78: };
 79: 
 80: // Chọn tối đa k dây dự phòng. Chiến lược (tham lam theo độ bảo vệ):
 81: //   - Xét các edge chưa xây, hợp lệ, còn cổng, theo thứ tự compareEdges (rẻ trước)
 82: //   - Một edge nối u-v "bảo vệ" các cạnh cây nằm trên đường u->v trong cây:
 83: //     nếu một cạnh đó đứt, edge này nối lại hai phía
 84: //   - Chỉ chọn edge bảo vệ THÊM ít nhất một cạnh cây chưa được bảo vệ
 85: //     (không chọn dây tốn tiền mà không thêm lợi gì)
 86: // Có thể trả về ít hơn k edge nếu không còn edge nào thêm được sự bảo vệ.
 87: // Cạnh cây không thể bảo vệ (vd host chỉ có 1 cổng) không tính vào protectedTreeEdges.
 88: // Cập nhật g: isBuilt = true, isBackup = true, usedPorts +1 hai đầu.
 89: BackupResult selectBackupLinks(Graph& g, const MSTResult& mst,
 90:                                int k = DEFAULT_BACKUP_LINKS,
 91:                                const CostFn& cost = defaultCablingCost);
 92: 
 93: // Tổng chi phí lắp đặt của một tập edge
 94: double sumCablingCost(const Graph& g, const std::vector<int>& edgeIds,
 95:                       const CostFn& cost = defaultCablingCost);
 96: 
 97: #endif // KRUSKAL_H

```

### Toàn bộ mã nguồn Cài đặt (`src/Kruskal.cpp`):
```cpp
  1: #include "Kruskal.h"
  2: #include <algorithm>
  3: #include <queue>
  4: #include <unordered_map>
  5: #include "Validation.h"
  6: 
  7: double defaultCablingCost(const Edge& e) {
  8:     return e.length * e.unitPrice * e.terrainFactor + e.equipmentCost + e.maintenanceCost;
  9: }
 10: 
 11: const char* toString(RejectReason r) {
 12:     switch (r) {
 13:         case RejectReason::NONE:      return "OK";
 14:         case RejectReason::LINK_DOWN: return "LINK_DOWN";
 15:         case RejectReason::TOO_LONG:  return "TOO_LONG";
 16:         case RejectReason::CYCLE:     return "CYCLE";
 17:         case RejectReason::NO_PORT:   return "NO_PORT";
 18:     }
 19:     return "?";
 20: }
 21: 
 22: // ------------------------------------------------------------
 23: // Sắp xếp: rẻ trước; bằng nhau thì id nhỏ trước
 24: // (hai edge cùng dữ liệu tính cùng công thức nên so sánh bằng chính xác)
 25: // ------------------------------------------------------------
 26: bool compareEdges(const Edge& a, const Edge& b, const CostFn& cost) {
 27:     const double ca = cost(a);
 28:     const double cb = cost(b);
 29:     if (ca != cb) return ca < cb;
 30:     return a.id < b.id;
 31: }
 32: 
 33: RejectReason checkEdge(const Graph& g, const Edge& e, DSU& dsu) {
 34:     if (!e.isUp) return RejectReason::LINK_DOWN;                    // dây đang hỏng, không thể lắp
 35:     if (!Validation::isStaticFeasible(e)) return RejectReason::TOO_LONG; // vượt độ dài tối đa của loại cáp
 36:     if (dsu.connected(e.u, e.v)) return RejectReason::CYCLE;         // đã có đường u~v, thêm nữa thành chu trình
 37:     if (!Validation::checkPortCapacity(g.getNode(e.u)) || !Validation::checkPortCapacity(g.getNode(e.v)))
 38:         return RejectReason::NO_PORT;                                // một đầu hết cổng
 39:     return RejectReason::NONE;
 40: }
 41: 
 42: // ------------------------------------------------------------
 43: // Kruskal có ràng buộc
 44: // ------------------------------------------------------------
 45: MSTResult buildMST(Graph& g, const CostFn& cost, std::vector<Rejection>* rejections) {
 46:     g.resetBuildState();
 47:     if (rejections) rejections->clear();
 48: 
 49:     MSTResult result;
 50:     DSU dsu(g.maxNodeId() + 1);
 51: 
 52:     // Bước 1: sắp xếp id edge theo compareEdges
 53:     std::vector<int> order = g.edgeIds();
 54:     std::sort(order.begin(), order.end(), [&](int x, int y) {
 55:         return compareEdges(g.getEdge(x), g.getEdge(y), cost);
 56:     });
 57: 
 58:     // Bước 2-3: duyệt, chọn nếu hợp lệ
 59:     for (int id : order) {
 60:         Edge& e = g.getEdge(id);
 61:         RejectReason why = checkEdge(g, e, dsu);
 62:         if (why != RejectReason::NONE) {
 63:             if (rejections) rejections->push_back({id, why});
 64:             continue;
 65:         }
 66:         dsu.unite(e.u, e.v);
 67:         g.getNode(e.u).usedPorts++;
 68:         g.getNode(e.v).usedPorts++;
 69:         e.isBuilt = true;
 70:         result.edgeIds.push_back(id);
 71:         result.totalCost += cost(e);
 72:     }
 73: 
 74:     // Nối hết mọi node <=> đủ V-1 cạnh (V > 0)
 75:     result.connected = (g.V() > 0 && static_cast<int>(result.edgeIds.size()) == g.V() - 1);
 76:     return result;
 77: }
 78: 
 79: // ------------------------------------------------------------
 80: // Dây dự phòng
 81: // ------------------------------------------------------------
 82: namespace {
 83: 
 84: // Rừng tạo từ các cạnh cây: parent/depth/comp để tìm đường u -> v trong cây
 85: struct TreeIndex {
 86:     std::unordered_map<int, int> comp;        // thành phần chứa node
 87:     std::unordered_map<int, int> depth;
 88:     std::unordered_map<int, int> parentNode;
 89:     std::unordered_map<int, int> parentEdge;  // edge nối node lên cha
 90: 
 91:     TreeIndex(const Graph& g, const std::vector<int>& treeEdges) {
 92:         std::unordered_map<int, std::vector<int>> adj;
 93:         for (int id : treeEdges) {
 94:             const Edge& e = g.getEdge(id);
 95:             adj[e.u].push_back(id);
 96:             adj[e.v].push_back(id);
 97:         }
 98:         int c = 0;
 99:         for (int s : g.nodeIds()) {
100:             if (comp.count(s)) continue;
101:             std::queue<int> q;
102:             q.push(s);
103:             comp[s] = c;
104:             depth[s] = 0;
105:             while (!q.empty()) {
106:                 int x = q.front();
107:                 q.pop();
108:                 for (int id : adj[x]) {
109:                     int y = g.otherEndpoint(x, id);
110:                     if (comp.count(y)) continue;
111:                     comp[y] = c;
112:                     depth[y] = depth[x] + 1;
113:                     parentNode[y] = x;
114:                     parentEdge[y] = id;
115:                     q.push(y);
116:                 }
117:             }
118:             ++c;
119:         }
120:     }
121: 
122:     // Các cạnh cây trên đường u -> v (leo lên từ node sâu hơn cho đến khi gặp nhau)
123:     std::vector<int> pathEdges(int u, int v) const {
124:         std::vector<int> out;
125:         while (u != v) {
126:             if (depth.at(u) >= depth.at(v)) {
127:                 out.push_back(parentEdge.at(u));
128:                 u = parentNode.at(u);
129:             } else {
130:                 out.push_back(parentEdge.at(v));
131:                 v = parentNode.at(v);
132:             }
133:         }
134:         return out;
135:     }
136: };
137: 
138: } // namespace
139: 
140: BackupResult selectBackupLinks(Graph& g, const MSTResult& mst, int k, const CostFn& cost) {
141:     BackupResult result;
142:     result.treeEdges = static_cast<int>(mst.edgeIds.size());
143:     if (k <= 0 || mst.edgeIds.empty()) return result;
144: 
145:     TreeIndex tree(g, mst.edgeIds);
146:     std::unordered_map<int, bool> covered;   // cạnh cây nào đã có đường thay thế
147: 
148:     // Ứng viên: chưa xây, đang up, hợp lệ về độ dài; sắp rẻ trước
149:     std::vector<int> candidates;
150:     for (int id : g.edgeIds()) {
151:         const Edge& e = g.getEdge(id);
152:         if (!e.isBuilt && e.isUp && e.length <= e.maxSegmentLength) candidates.push_back(id);
153:     }
154:     std::sort(candidates.begin(), candidates.end(), [&](int x, int y) {
155:         return compareEdges(g.getEdge(x), g.getEdge(y), cost);
156:     });
157: 
158:     for (int id : candidates) {
159:         if (static_cast<int>(result.edgeIds.size()) >= k) break;
160:         Edge& e = g.getEdge(id);
161:         if (!Validation::checkPortCapacity(g.getNode(e.u)) || !Validation::checkPortCapacity(g.getNode(e.v))) continue;
162:         if (tree.comp.at(e.u) != tree.comp.at(e.v)) continue;   // không có đường trong cây để bảo vệ
163: 
164:         // Chỉ chọn nếu bảo vệ thêm ít nhất một cạnh cây chưa được bảo vệ
165:         std::vector<int> path = tree.pathEdges(e.u, e.v);
166:         bool addsCoverage = false;
167:         for (int t : path) if (!covered[t]) { addsCoverage = true; break; }
168:         if (!addsCoverage) continue;
169: 
170:         for (int t : path) covered[t] = true;
171:         g.getNode(e.u).usedPorts++;
172:         g.getNode(e.v).usedPorts++;
173:         e.isBuilt = true;
174:         e.isBackup = true;
175:         result.edgeIds.push_back(id);
176:         result.totalCost += cost(e);
177:     }
178: 
179:     for (int t : mst.edgeIds) if (covered[t]) ++result.protectedTreeEdges;
180:     return result;
181: }
182: 
183: double sumCablingCost(const Graph& g, const std::vector<int>& edgeIds, const CostFn& cost) {
184:     double sum = 0.0;
185:     for (int id : edgeIds) sum += cost(g.getEdge(id));
186:     return sum;
187: }

```

### Giải thích chi tiết từng dòng code:

#### 1. Định nghĩa kiểu dữ liệu & Lý do từ chối cạnh (`include/Kruskal.h` dòng 16-36):
* `enum class RejectReason`: Ghi nhận nguyên nhân cụ thể khi một cạnh bị Kruskal từ chối:
  - `LINK_DOWN`: Dây đang đứt/hỏng hóc vật lý.
  - `TOO_LONG`: Vượt quá chiều dài tối đa của loại cáp (`length > maxSegmentLength`).
  - `CYCLE`: Cạnh tạo thành chu trình khép kín (hai đầu đã được nối bởi các cạnh rẻ hơn trước đó).
  - `NO_PORT`: Một trong hai router ở đầu mút đã cắm hết số cổng cho phép (`usedPorts >= totalPorts`).

#### 2. Các hàm bổ trợ trong `src/Kruskal.cpp`:
* **Dòng 26-31 (`compareEdges` - Tiêu chí sắp xếp cạnh):**
  - So sánh chi phí: `if (ca != cb) return ca < cb;` (Cạnh có chi phí thi công rẻ hơn được ưu tiên xét trước).
  - **Tie-breaker:** `return a.id < b.id;`: Nếu hai cạnh có chi phí bằng nhau, ưu tiên cạnh có ID nhỏ hơn.
  - **Ý nghĩa kỹ thuật:** Đảm bảo tính tất định (Deterministic). Kết quả cây khung MST sẽ hoàn toàn giống nhau $100\%$ giữa các lần chạy, không bị phụ thuộc vào thứ tự ngẫu nhiên trong bộ nhớ.
* **Dòng 33-40 (`checkEdge` - Kiểm tra 4 tầng ràng buộc):**
  - Lần lượt kiểm tra: `!e.isUp` $\to$ `LINK_DOWN`, `!isStaticFeasible(e)` $\to$ `TOO_LONG`, `dsu.connected(e.u, e.v)` $\to$ `CYCLE`, `!checkPortCapacity` $\to$ `NO_PORT`. Chỉ khi vượt qua cả 4 chốt chặn mới trả về `NONE` (hợp lệ).

#### 3. Thuật toán chính `buildMST` (Dòng 45-77):
* **Bước 1 (Dòng 53-56):** Lấy danh sách ID của tất cả các cạnh trong đồ thị và sắp xếp theo `compareEdges`. Độ phức tạp: $\mathcal{O}(E \log E)$.
* **Bước 2 (Dòng 59-72):** Duyệt qua từng cạnh trong danh sách đã sắp xếp:
  - Gọi `checkEdge(g, e, dsu)`. Nếu vi phạm thì ghi vào danh sách `rejections` và bỏ qua.
  - Nếu hợp lệ:
    - `dsu.unite(e.u, e.v);`: Gộp 2 nút vào cùng một tập hợp cây khung.
    - `g.getNode(e.u).usedPorts++; g.getNode(e.v).usedPorts++;`: Tăng số cổng đã sử dụng của 2 router.
    - `e.isBuilt = true;`: Đóng dấu phê duyệt xây dựng cáp này trên thực tế.
    - `result.edgeIds.push_back(id);`: Thêm cạnh vào danh sách MST.
    - `result.totalCost += cost(e);`: Cộng dồn chi phí.
* **Bước 3 (Dòng 75):** Kiểm tra tính liên thông toàn mạng:
  - `result.connected = (g.V() > 0 && result.edgeIds.size() == g.V() - 1);`
  - Theo định lý đồ thị học: Một đồ thị liên thông gồm $V$ đỉnh có cây khung khi và chỉ khi cây chứa đúng $V - 1$ cạnh và không có chu trình.

#### 4. Thuật toán quy hoạch cáp dự phòng `selectBackupLinks` (Dòng 140-181):
* **Mục đích:** Cây MST chỉ có $V-1$ cạnh độc đạo, nếu 1 sợi đứt thì mạng gãy làm đôi ngay. Do đó, hàm này chọn thêm tối đa $k$ cạnh ngoài cây (Non-tree edges) có chi phí rẻ nhất để làm cáp dự phòng.
* **Cấu trúc `TreeIndex` (Dòng 85-136):**
  - Dùng thuật toán BFS trên các cạnh MST để tính `depth` (độ sâu) và `parentNode` (nút cha) của từng đỉnh trên cây.
  - Hàm `pathEdges(u, v)`: Tìm đường đi duy nhất giữa 2 nút $u$ và $v$ trên cây MST bằng cách leo dần từ nút sâu hơn lên cho đến khi gặp nhau (tương tự Lowest Common Ancestor - LCA).
* **Tiêu chí chọn cáp dự phòng (Dòng 165-177):**
  - Lọc các ứng viên: Chưa xây (`!isBuilt`), còn hoạt động (`isUp`), cự ly truyền đạt chuẩn (`length <= maxSegmentLength`), còn cổng trống ở 2 đầu router.
  - Sợi cáp dự phòng nối giữa $u$ và $v$ sẽ tạo thành một chu trình dự phòng bảo vệ cho toàn bộ các cạnh trên đường đi $u \sim v$ trong cây MST.
  - **Nguyên lý Greedy tối ưu độ phủ:** Chỉ chọn cạnh dự phòng nếu nó bảo vệ thêm ít nhất một cạnh cây MST chưa được bảo vệ (`addsCoverage == true`).
  - Khi được chọn: Đánh dấu `e.isBuilt = true` và `e.isBackup = true`.

---

# CHƯƠNG 4: TÍNH TOÁN ĐỘ TRỄ & ĐỊNH TUYẾN ĐỘNG THÍCH ỨNG (BÀI TOÁN LOGIC – DIJKSTRA, DELAY & QOS)

Sau khi hạ tầng cáp vật lý đã được phê duyệt ở Chương 3 (`isBuilt = true`), hệ thống bước vào bài toán vận hành dữ liệu: Chọn đường đi nào cho gói tin sao cho nhanh nhất, né tắc nghẽn và đảm bảo chất lượng dịch vụ (QoS).

---

## 4.1. File `include/DelayModel.h` & `src/DelayModel.cpp`

### Toàn bộ mã nguồn Header (`include/DelayModel.h`):
```cpp
  1: #ifndef DELAYMODEL_H
  2: #define DELAYMODEL_H
  3: 
  4: #include "Edge.h"
  5: #include "Node.h"
  6: #include "Packet.h"
  7: 
  8: namespace DelayModel {
  9: 
 10:     //Độ trễ truyền tải 
 11:     double calcTransmissionDelay(int sizeBytes, double bandwidthMbps);
 12: 
 13:     //Độ trễ lan truyền
 14:     double calcPropagationDelay(double length, double propagationSpeed);
 15: 
 16:     //Độ trễ xử lý tại Node đích
 17:     double calcProcessingDelay(const Node& nextNode);
 18: 
 19:     //Độ trễ hàng đợi
 20:     double calcQueueDelay(double baseQueueDelay, double currentLoad);
 21: 
 22:     //Độ trễ cơ sở tổng hợp
 23:     double calcBaseDelay(const Packet& packet, const Edge& edge, const Node& nextNode);
 24: 
 25:     //Tính số mảnh dữ liệu
 26:     // Trả về số mảnh. Nếu không cho phép phân mảnh và gói tin quá lớn, trả về 0.
 27:     int calcFragmentCount(int sizeBytes, int mtu, bool canFragment);
 28: 
 29:     //TCP / UDP Protocol Overhead
 30:     double calcProtocolOverhead(const Packet& packet, double oneWayDelay);
 31: 
 32:     // Tính độ trễ truyền một chặng/cạnh (chưa bao gồm Protocol Overhead như TCP handshake)
 33:     double calculateHopDelay(const Packet& packet, const Edge& edge, const Node& nextNode);
 34: 
 35:     // Tính tổng thể khi gói tin đi qua 1 cạnh (đã bao gồm Protocol Overhead nếu xét đơn lẻ 1 chặng)
 36:     double calculateTotalDelay(const Packet& packet, const Edge& edge, const Node& nextNode);
 37: }
 38: 
 39: #endif // DELAYMODEL_H

```

### Toàn bộ mã nguồn Cài đặt (`src/DelayModel.cpp`):
```cpp
  1: #include "DelayModel.h"
  2: #include "Config.h"
  3: #include "RoutingCost.h"
  4: 
  5: namespace DelayModel {
  6: 
  7:     double calcTransmissionDelay(int sizeBytes, double bandwidthMbps) {
  8:         if (bandwidthMbps <= 0) return Config::INF;
  9:         // Công thức: (sizeBytes * 8) / (bandwidthMbps * 1e6)
 10:         // Đổi byte ra bit và Mbps ra bps
 11:         return (sizeBytes * 8.0) / (bandwidthMbps * 1000000.0);
 12:     }
 13: 
 14:     double calcPropagationDelay(double length, double propagationSpeed) {
 15:         if (propagationSpeed <= 0) return 0.0;
 16:         return length / propagationSpeed;
 17:     }
 18: 
 19:     double calcProcessingDelay(const Node& nextNode) {
 20:         // Trễ do CPU của Router đích xử lý
 21:         return nextNode.processingTime;
 22:     }
 23: 
 24:     double calcQueueDelay(double baseQueueDelay, double currentLoad) {
 25:         // Trễ hàng đợi bị khuếch đại nếu cáp đang tắc nghẽn (Dùng hàm của RoutingCost, tuân thủ DRY)
 26:         return baseQueueDelay * RoutingCost::congestionPenalty(currentLoad);
 27:     }
 28: 
 29:     double calcBaseDelay(const Packet& packet, const Edge& edge, const Node& nextNode) {
 30:         if (!edge.isUp) return Config::INF; // Cáp đứt thì không truyền được
 31: 
 32:         double dTrans = calcTransmissionDelay(packet.sizeBytes, edge.bandwidthMbps);
 33:         
 34:         // Cáp quang/đồng thì truyền theo tốc độ ánh sáng trong thủy tinh/đồng (~2e8)
 35:         // Wireless truyền theo tốc độ ánh sáng trong không khí (~3e8)
 36:         double propSpeed = (edge.mediaType == MediaType::WIRELESS) ? 
 37:                            Config::SPEED_OF_LIGHT_AIR : Config::SPEED_OF_LIGHT_FIBER;
 38:                            
 39:         double dProp = calcPropagationDelay(edge.length, propSpeed);
 40:         
 41:         double dProc = calcProcessingDelay(nextNode);
 42:         double dQueue = calcQueueDelay(edge.queueDelay, edge.currentLoad);
 43: 
 44:         // Cộng dồn 4 loại trễ
 45:         return dTrans + dProp + dProc + dQueue;
 46:     }
 47: 
 48:     int calcFragmentCount(int sizeBytes, int mtu, bool canFragment) {
 49:         if (sizeBytes <= mtu) return 1;
 50:         if (!canFragment) return 0; // DF bit is set, packet drops
 51:         
 52:         // Payload mỗi mảnh = mtu - 20 byte header
 53:         int payload = mtu - 20;
 54:         if (payload <= 0) return 0; // Tránh lỗi chia 0 nếu MTU quá nhỏ
 55:         
 56:         // Tính số mảnh: ceil(size / payload)
 57:         int n = sizeBytes / payload;
 58:         if (sizeBytes % payload != 0) n++; // Làm tròn lên
 59:         return n;
 60:     }
 61: 
 62:     double calcProtocolOverhead(const Packet& packet, double oneWayDelay) {
 63:         if (packet.protocol == Protocol::TCP && packet.newConnection) {
 64:             // TCP 3-way handshake tốn 1 RTT (Round Trip Time = 2 * One-way Delay)
 65:             return 2.0 * oneWayDelay;
 66:         }
 67:         return 0.0;
 68:     }
 69: 
 70:     double calculateHopDelay(const Packet& packet, const Edge& edge, const Node& nextNode) {
 71:         if (!edge.isUp) return Config::INF;
 72: 
 73:         // 1. Kiểm tra phân mảnh
 74:         int fragments = calcFragmentCount(packet.sizeBytes, edge.mtu, packet.canFragment);
 75:         if (fragments == 0) return Config::INF; // Bị rớt gói tin do quá to mà không cho băm
 76: 
 77:         // 2. Tính Base Delay cơ bản (trường hợp 1 mảnh không phân mảnh)
 78:         double baseDelay = calcBaseDelay(packet, edge, nextNode);
 79:         if (baseDelay == Config::INF) return Config::INF;
 80: 
 81:         // 3. Nếu phải phân mảnh (fragments > 1), trễ phát dTrans sẽ tăng thêm cho (fragments - 1) mảnh phụ
 82:         double dTrans = calcTransmissionDelay(packet.sizeBytes, edge.bandwidthMbps);
 83:         return baseDelay + dTrans * (fragments - 1);
 84:     }
 88: 
 89:     double calculateTotalDelay(const Packet& packet, const Edge& edge, const Node& nextNode) {
 90:         double oneWayDelay = calculateHopDelay(packet, edge, nextNode);
 91:         if (oneWayDelay == Config::INF) return Config::INF;
 92: 
 93:         // 3. Tính Overhead của Giao thức (TCP/UDP) nếu xét đơn lẻ trên 1 cạnh
 94:         double overhead = calcProtocolOverhead(packet, oneWayDelay);
 95: 
 96:         return oneWayDelay + overhead;
 97:     }
 98: 
 99: }

```

### Giải thích chi tiết từng dòng code:

#### 1. Mô hình 4 loại trễ mạng viễn thông:
Mỗi khi gói tin đi qua một liên kết vật lý kết nối với một router kế tiếp, nó phải chịu đúng 4 loại trễ:
1. **Trễ truyền dẫn (Transmission Delay - $d_{trans}$):** Thời gian cần thiết để bộ điều khiển mạng đẩy tất cả các bit của gói tin lên đường dây:
   $$d_{trans} = \frac{\text{Kích thước gói (bits)}}{\text{Băng thông (bps)}} = \frac{\text{sizeBytes} \times 8}{\text{bandwidthMbps} \times 10^6}$$
2. **Trễ lan truyền (Propagation Delay - $d_{prop}$):** Thời gian sóng điện từ / hạt photon ánh sáng di chuyển từ đầu này sang đầu kia sợi cáp với vận tốc ánh sáng $v$:
   $$d_{prop} = \frac{\text{Chiều dài cáp (m)}}{\text{Vận tốc ánh sáng trong môi trường (m/s)}}$$
   Trong cáp quang/cáp đồng: $v = 2.0 \times 10^8$ m/s. Trong không khí (Wireless): $v = 3.0 \times 10^8$ m/s.
3. **Trễ xử lý tại Router (Processing Delay - $d_{proc}$):** Thời gian CPU của Router đích kiểm tra checksum header, tra bảng định tuyến và quyết định cổng ra tiếp theo (`nextNode.processingTime`).
4. **Trễ hàng đợi (Queue Delay - $d_{queue}$):** Thời gian gói tin phải xếp hàng chờ trong bộ đệm RAM của card mạng trước khi đến lượt truyền. Khi đường truyền bị quá tải ($currentLoad > 80\%$), hàng đợi bị dồn ứ nghiêm trọng, trễ hàng đợi tăng vọt theo hệ số phạt nghẽn `congestionPenalty`.

#### 2. Chi tiết các hàm trong `src/DelayModel.cpp`:
* **Dòng 7-12 (`calcTransmissionDelay`):**
  - Đổi byte ra bit (`sizeBytes * 8.0`) và đổi Mbps ra bps (`bandwidthMbps * 1e6`).
  - Trả về $INF$ nếu băng thông $\le 0$ để bảo vệ chia cho 0.
* **Dòng 14-17 (`calcPropagationDelay`):**
  - Trả về `length / propagationSpeed`.
* **Dòng 19-22 (`calcProcessingDelay`):**
  - Trả về `nextNode.processingTime`.
* **Dòng 24-27 (`calcQueueDelay`):**
  - Trả về `baseQueueDelay * RoutingCost::congestionPenalty(currentLoad)`. Tuân thủ nguyên tắc DRY (Don't Repeat Yourself), tái sử dụng hệ số phạt nghẽn của `RoutingCost`.
* **Dòng 29-46 (`calcBaseDelay`):**
  - Tổng của 4 thành phần cơ bản: $d_{trans} + d_{prop} + d_{proc} + d_{queue}$.
* **Dòng 48-60 (`calcFragmentCount` - Phân mảnh gói tin theo MTU):**
  - Nếu `sizeBytes <= mtu`: Không cần phân mảnh $\to$ Trả về `1`.
  - Nếu `!canFragment`: Cờ Don't Fragment (DF) được bật $\to$ Cấm băm nhỏ $\to$ Trả về `0` (Gói tin bị DROP do quá cỡ).
  - Ngược lại: Payload hữu ích của mỗi mảnh = `mtu - 20` byte (dành 20 byte cho IP header của từng mảnh).
  - Số mảnh: $\lceil \frac{\text{sizeBytes}}{\text{payload}} \rceil$.
* **Dòng 62-68 (`calcProtocolOverhead` - Chi phí giao thức TCP Handshake):**
  - Nếu là giao thức `TCP` và là kết nối mới mở (`newConnection == true`):
    - Cần bắt tay 3 bước SYN $\to$ SYN-ACK $\to$ ACK, tốn đúng $1 \times RTT$ (Round Trip Time).
    - Vì $RTT = 2 \times \text{One-Way Delay}$, hàm trả về `2.0 * oneWayDelay`.
  - Ngược lại (UDP hoặc kết nối TCP đã mở sẵn): Trả về `0.0`.
* **Dòng 70-87 (`calculateHopDelay` - Trễ một chặng):**
  - Tính tổng trễ trên 1 cạnh cụ thể: Trễ truyền dẫn nhân theo số mảnh ($d_{trans} \times fragments$) cộng với trễ lan truyền, trễ xử lý router và trễ hàng đợi. **Tuyệt đối không cộng TCP handshake ở đây**.
* **Dòng 89-98 (`calculateTotalDelay`):**
  - Dùng cho kiểm thử đơn lẻ 1 chặng: Lấy kết quả của `calculateHopDelay` cộng thêm TCP handshake nếu có.

---

## 4.2. File `include/RoutingCost.h` & `src/RoutingCost.cpp`

### Toàn bộ mã nguồn Header (`include/RoutingCost.h`):
```cpp
  1: #ifndef ROUTING_COST_H
  2: #define ROUTING_COST_H
  3: 
  4: #include "Edge.h"
  5: #include "Packet.h"
  6: #include "Dijkstra.h"   // for WeightFn
  7: 
  8: // ============================================================
  9: // RoutingCost.h 
 10: // Trước đây Dijkstra định tuyến theo weight "tĩnh" (unitWeight
 11: // hoặc lengthWeight) - không quan tâm tải hiện tại của dây hay loại gói
 12: // tin. Bây giờ cần Dijkstra định tuyến "động": cùng 1 mạng nhưng gói tin
 13: // REAL_TIME và BULK_DATA có thể được định tuyến theo 2 đường khác nhau,
 14: // tùy theo cạnh nào đang bị nghẽn.
 15: // ============================================================
 16: namespace RoutingCost {
 17: 
 18: // Hệ số phạt tăng theo tải hiện tại của cạnh (load trong [0,1]).
 19: 
 20: double congestionPenalty(double load);
 21: 
 22: // Chi phí định tuyến "động" của 1 cạnh, phụ thuộc loại
 23: // gói tin (QoS) và trạng thái tức thời của cạnh (currentLoad, queueDelay).
 24: //
 25: //   cost = wTrans*dTrans + wProp*dProp + wQueue*dQueue*congestionPenalty
 26: //          + fragmentPenalty
 27: //
 28: // - Nếu cạnh đang đứt (isUp = false) -> trả Config::INF.
 29: // - Nếu gói tin DF (canFragment = false) mà vượt MTU -> trả Config::INF.
 30: // - Không bao giờ trả NaN (có chặn phòng thủ ở cuối hàm).
 31: //
 32: // LƯU Ý: không cộng dProc (độ trễ xử lý tại node kế tiếp) vào đây, vì
 33: // WeightFn chỉ nhận được Edge (không có thông tin đang đi theo hướng
 34: // u->v hay v->u, nên không xác định được "node kế tiếp" là node nào).
 35: // dProc vẫn được cộng đầy đủ khi tính TỔNG độ trễ thực tế của gói tin
 36: // (DelayModel::calculateTotalDelay, dùng trong Simulator::sendPacket) -
 37: // chỉ không tham gia vào việc Dijkstra CHỌN đường, vì nó gần như cố định
 38: // theo node đích, không giúp so sánh giữa các lựa chọn đường khác nhau.
 39: double calculateRoutingCost(const Packet& packet, const Edge& edge);
 40: 
 41: // Đóng gói calculateRoutingCost cho 1 Packet cụ thể thành 1
 42: // WeightFn, để Dijkstra chỉ cần gọi weight(e) như các weight tĩnh trước
 43: // đây (unitWeight, lengthWeight) - Dijkstra.cpp KHÔNG cần biết gì về
 44: // QoS/tắc nghẽn/MTU, giữ đúng nguyên tắc tách biệt thuật toán và nghiệp vụ.
 45: WeightFn makeWeightFn(const Packet& packet);
 46: 
 47: } // namespace RoutingCost
 48: 
 49: #endif // ROUTING_COST_H

```

### Toàn bộ mã nguồn Cài đặt (`src/RoutingCost.cpp`):
```cpp
  1: #include "RoutingCost.h"
  2: #include "Config.h"
  3: #include "DelayModel.h"
  4: #include <cmath>
  5: 
  6: namespace RoutingCost {
  7: 
  8: double congestionPenalty(double load) {
  9:     if (load <= 0.80) return 1.0;
 10:     if (load <= 0.85) return 1.56;
 11:     if (load <= 0.90) return 3.25;
 12:     if (load <= 0.95) return 6.06;
 13:     return 10.00;
 14: }
 15: 
 16: double calculateRoutingCost(const Packet& packet, const Edge& edge) {
 17:     // Day dang dut -> khong the di qua
 18:     if (!edge.isUp) return Config::INF;
 19: 
 20:     // Kiem tra phan manh truoc: neu la goi DF (khong cho phan manh) ma
 21:     // vuot qua MTU thi khong the di qua day nay duoc (giong DelayModel).
 22:     int fragments = DelayModel::calcFragmentCount(packet.sizeBytes, edge.mtu, packet.canFragment);
 23:     if (fragments == 0) return Config::INF;
 24: 
 25:     // Chon bo trong so QoS theo loai goi tin
 26:     const Config::QoSWeight* w = nullptr;
 27:     switch (packet.type) {
 28:         case PacketType::REAL_TIME: w = &Config::QOS_WEIGHT_REAL_TIME; break;
 29:         case PacketType::BULK_DATA: w = &Config::QOS_WEIGHT_BULK_DATA; break;
 30:         case PacketType::CONTROL:   w = &Config::QOS_WEIGHT_CONTROL;   break;
 31:         default:                    w = &Config::QOS_WEIGHT_CONTROL;   break;
 32:     }
 33: 
 34:     // Tai su dung dung cac cong thuc thanh phan da co san o DelayModel
 35:     // de khong viet trung logic - chi khac o cho nhan them
 36:     // trong so QoS va he so tac nghen khi cong lai thanh 1 "chi phi".
 37:     double dTrans = DelayModel::calcTransmissionDelay(packet.sizeBytes, edge.bandwidthMbps);
 38:     double propSpeed = (edge.mediaType == MediaType::WIRELESS)
 39:                             ? Config::SPEED_OF_LIGHT_AIR
 40:                             : Config::SPEED_OF_LIGHT_FIBER;
 41:     double dProp = DelayModel::calcPropagationDelay(edge.length, propSpeed);
 42:     double dQueue = edge.queueDelay * congestionPenalty(edge.currentLoad);
 43: 
 44:     // Duoc dung ngay o day (nhan vao dQueue) - cang tai nhieu,
 45:     // dQueue cang bi khuech dai manh, Dijkstra se tu dong tranh duong nay.
 46:     double fragmentPenalty = (fragments > 1)
 47:                                   ? (fragments - 1) * Config::FRAGMENT_PENALTY_PER_EXTRA
 48:                                   : 0.0;
 49: 
 50:     double cost = w->wTrans * dTrans + w->wProp * dProp + w->wQueue * dQueue + fragmentPenalty;
 51: 
 52:     // Phong thu: khong de NaN lot qua (vd bandwidthMbps = 0 -> dTrans = INF,
 53:     // INF * trong so van la INF, khong phai NaN, nhung van kiem tra cho chac).
 54:     if (std::isnan(cost)) return Config::INF;
 55: 
 56:     return cost;
 57: }
 58: 
 59: WeightFn makeWeightFn(const Packet& packet) {
 60:     return [packet](const Edge& e) {
 61:         return calculateRoutingCost(packet, e);
 62:     };
 63: }
 64: 
 65: } // namespace RoutingCost

```

### Giải thích chi tiết từng dòng code:

#### 1. Hệ số phạt tắc nghẽn `congestionPenalty(load)` (Dòng 8-14 trong .cpp):
* Mạng viễn thông tuân theo lý thuyết hàng đợi $M/M/1$: Khi mức tải tiến dần về $100\%$, độ dài hàng đợi không tăng tuyến tính mà bùng nổ theo hàm tiệm cận $\frac{1}{1 - \rho}$.
* Bảng tra cứu bậc thang:
  - $\text{load} \le 80\%$: Bình thường, hệ số phạt $= 1.0$.
  - $80\% < \text{load} \le 85\%$: Hệ số phạt $= 1.56$.
  - $85\% < \text{load} \le 90\%$: Hệ số phạt $= 3.25$.
  - $90\% < \text{load} \le 95\%$: Hệ số phạt $= 6.06$.
  - $\text{load} > 95\%$: Báo động đỏ, hệ số phạt $= 10.00$.

#### 2. Hàm chi phí định tuyến `calculateRoutingCost(packet, edge)` (Dòng 16-57):
* **Chặn cáp đứt (Dòng 18):** `if (!edge.isUp) return Config::INF;`
* **Chặn vi phạm MTU (Dòng 22-23):** Nếu gói tin bật cờ DF mà kích thước vượt quá MTU của sợi cáp $\to$ Trả về `Config::INF` (Dijkstra coi cạnh này có chi phí vô cực, không bao giờ chọn đi qua).
* **Phân bổ trọng số QoS (Dòng 26-32):**
  - Chọn bộ trọng số tương ứng: `QOS_WEIGHT_REAL_TIME`, `QOS_WEIGHT_BULK_DATA`, hoặc `QOS_WEIGHT_CONTROL`.
* **Tính toán chi phí tổng hợp (Dòng 50):**
  $$\text{cost} = w_{trans} \times d_{trans} + w_{prop} \times d_{prop} + w_{queue} \times d_{queue} + \text{fragmentPenalty}$$
* **Chống lỗi NaN (Dòng 54):** `if (std::isnan(cost)) return Config::INF;`: Cơ chế phòng thủ bảo vệ thuật toán Dijkstra không bị treo nếu gặp số thực không xác định.

#### 3. Hàm đóng gói `makeWeightFn(packet)` (Dòng 59-63):
* Trả về một hàm Lambda closure nhận `const Edge& e` và trả về `double`. Đóng gói toàn bộ nghiệp vụ QoS và tắc nghẽn bên trong, giúp thuật toán Dijkstra giữ nguyên chữ ký kinh điển độc lập hoàn toàn với nghiệp vụ.

---

## 4.3. File `include/Dijkstra.h` & `src/Dijkstra.cpp`

### Toàn bộ mã nguồn Header (`include/Dijkstra.h`):
```cpp
  1: #ifndef DIJKSTRA_H
  2: #define DIJKSTRA_H
  3: 
  4: #include <functional>
  5: #include "Graph.h"
  6: #include "PathResult.h"
  7: 
  8: // ============================================================
  9: // Dijkstra.h - Tuần 3, Task A3.1 -> A3.5.
 10: //
 11: // Tìm đường đi "rẻ nhất" (theo weight) từ 1 node nguồn đến 1 node đích,
 12: // trên đồ thị vô hướng Graph đã có (Node/Edge/adjacency của Tuần 1-2).
 13: //
 14: // weight KHÔNG cố định là gì: A3.2 yêu cầu test lần lượt với
 15: //   1. weight = 1        (giống BFS, đếm số hop)
 16: //   2. weight = length   (khoảng cách vật lý)
 17: // Sau này khi ghép với DelayModel (module của Ý) chỉ cần đổi WeightFn
 18: // truyền vào, không phải sửa lại thuật toán - giống cách Kruskal nhận CostFn.
 19: // ============================================================
 20: using WeightFn = std::function<double(const Edge&)>;
 21: 
 22: // weight = 1 cho mọi cạnh -> Dijkstra chạy giống BFS, dist = số hop
 23: double unitWeight(const Edge& e);
 24: 
 25: // weight = độ dài vật lý của cáp
 26: double lengthWeight(const Edge& e);
 27: 
 28: // Tìm đường đi rẻ nhất từ source đến destination.
 29: // Trả PathResult.reachable = false nếu không tới được (đồ thị rời rạc,
 30: // source/destination không tồn tại, hoặc không có đường).
 31: //
 32: // Luôn bỏ qua cạnh đang isUp = false (dây đứt) - không thể đi qua dây hỏng.
 33: //
 34: // onlyBuilt: khi true, CHỈ đi trên các cạnh đã isBuilt = true (đã được Kruskal
 35: // chọn xây, tức MST + Backup). Đây là chế độ đúng cho việc định tuyến gói tin
 36: // thật, vì gói tin chỉ có thể đi trên dây cáp ĐÃ LẮP, không phải trên toàn bộ
 37: // dây ứng viên. Để false khi muốn khảo sát trên đồ thị ứng viên đầy đủ
 38: // (ví dụ so sánh trước khi quyết định xây gì).
 39: PathResult dijkstra(const Graph& g, int source, int destination,
 40:                      const WeightFn& weight = lengthWeight,
 41:                      bool onlyBuilt = false);
 42: 
 43: #endif // DIJKSTRA_H

```

### Toàn bộ mã nguồn Cài đặt (`src/Dijkstra.cpp`):
```cpp
  1: #include "Dijkstra.h"
  2: #include <queue>
  3: #include <unordered_map>
  4: #include <limits>
  5: 
  6: double unitWeight(const Edge&) { return 1.0; }
  7: double lengthWeight(const Edge& e) { return e.length; }
  8: 
  9: namespace {
 10: 
 11: // ------------------------------------------------------------
 12: // Task A3.3 - Min heap
 13: // State giữ (khoảng cách hiện tại tới node, node đó).
 14: // priority_queue mặc định là MAX-heap, nên phải dùng greater<State>
 15: // để lấy ra được State có dist NHỎ NHẤT trước - đúng ý tưởng của Dijkstra
 16: // ("luôn xử lý node gần nguồn nhất còn lại trước").
 17: // ------------------------------------------------------------
 18: struct State {
 19:     double dist;
 20:     int    node;
 21: };
 22: 
 23: bool operator>(const State& a, const State& b) {
 24:     return a.dist > b.dist;
 25: }
 26: 
 27: } // namespace
 28: 
 29: PathResult dijkstra(const Graph& g, int source, int destination, const WeightFn& weight, bool onlyBuilt) {
 30:     PathResult result;
 31: 
 32:     if (!g.hasNode(source) || !g.hasNode(destination)) return result;  // reachable = false
 33: 
 34:     // dist/parent/parentEdge dùng unordered_map vì id node có thể không liên tục (sparse)
 35:     std::unordered_map<int, double> dist;
 36:     std::unordered_map<int, int>    parent;      // parent[v] = node trước v trên đường đi ngắn nhất
 37:     std::unordered_map<int, int>    parentEdge;  // parentEdge[v] = edgeId nối parent[v] -> v
 38:     std::unordered_map<int, bool>   visited;     // node đã "chốt" khoảng cách ngắn nhất chưa
 39: 
 40:     for (int id : g.nodeIds()) dist[id] = std::numeric_limits<double>::infinity();
 41:     dist[source] = 0.0;
 42: 
 43:     // Task A3.3: Min heap - luôn pop ra state có dist nhỏ nhất
 44:     std::priority_queue<State, std::vector<State>, std::greater<State>> pq;
 45:     pq.push({0.0, source});
 46: 
 47:     while (!pq.empty()) {
 48:         State cur = pq.top();
 49:         pq.pop();
 50: 
 51:         // Bỏ record cũ: node đã chốt rồi, hoặc dist trong state đã lỗi thời
 52:         // (đã bị một lần relax sau ghi đè bằng giá trị nhỏ hơn) thì bỏ qua.
 53:         if (visited[cur.node]) continue;
 54:         if (cur.dist > dist[cur.node]) continue;
 55: 
 56:         visited[cur.node] = true;
 57:         if (cur.node == destination) break;   // đã chốt xong đích, không cần đi tiếp
 58: 
 59:         // Task A3.4: Relaxation
 60:         for (int edgeId : g.getNeighbors(cur.node)) {
 61:             const Edge& e = g.getEdge(edgeId);
 62:             if (!e.isUp) continue;             // day dang dut -> khong the di qua
 63:             if (onlyBuilt && !e.isBuilt) continue;  // chi di tren day DA LAP (MST + Backup)
 64:             int v = g.otherEndpoint(cur.node, edgeId);
 65:             if (v == -1 || visited[v]) continue;
 66: 
 67:             double w = weight(e);
 68:             double newDist = dist[cur.node] + w;
 69: 
 70:             if (newDist < dist[v]) {           // if dist[v] > dist[u] + w
 71:                 dist[v] = newDist;
 72:                 parent[v] = cur.node;
 73:                 parentEdge[v] = edgeId;        // lưu ĐÚNG edgeId - quan trọng khi có cạnh song song
 74:                 pq.push({newDist, v});
 75:             }
 76:         }
 77:     }
 78: 
 79:     if (!visited[destination]) {
 80:         return result;   // reachable = false, không tới được
 81:     }
 82: 
 83:     // Task A3.5: Reconstruct path - đi ngược từ destination về source rồi đảo chiều
 84:     std::vector<int> nodesRev;
 85:     std::vector<int> edgesRev;
 86:     int cur = destination;
 87:     nodesRev.push_back(cur);
 88:     while (cur != source) {
 89:         edgesRev.push_back(parentEdge[cur]);
 90:         cur = parent[cur];
 91:         nodesRev.push_back(cur);
 92:     }
 93: 
 94:     result.reachable = true;
 95:     result.nodes.assign(nodesRev.rbegin(), nodesRev.rend());
 96:     result.edges.assign(edgesRev.rbegin(), edgesRev.rend());
 97:     result.totalCost = dist[destination];
 98:     result.hopCount  = static_cast<int>(result.edges.size());
 99: 
100:     return result;
101: }

```

### Giải thích chi tiết từng dòng code:

#### 1. Cấu trúc hàng đợi ưu tiên Min-Heap (`State` dòng 18-25 trong .cpp):
* `struct State { double dist; int node; };`: Lưu cặp `(khoảng cách nhỏ nhất hiện tại, ID nút)`.
* `bool operator>(const State& a, const State& b) { return a.dist > b.dist; }`:
  - **Câu hỏi vấn đáp kinh điển:** *Tại sao lại nạp chồng toán tử `>` thay vì `<`?*
  - **Trả lời:** `std::priority_queue` trong thư viện chuẩn C++ mặc định là **Max-Heap** (phần tử lớn nhất nằm ở đỉnh). Để chuyển thành **Min-Heap** (phần tử có khoảng cách nhỏ nhất luôn được lấy ra xử lý trước - đúng tinh thần Greedy của Dijkstra), ta phải truyền `std::greater<State>`, yêu cầu toán tử `operator>`.

#### 2. Thuật toán Dijkstra tìm đường ngắn nhất (Dòng 29-101):
* **Khởi tạo (Dòng 35-45):**
  - Bảng băm `dist[nodeId]`: Khởi tạo bằng $\infty$ cho mọi nút, riêng nút nguồn `dist[source] = 0.0`.
  - Bảng băm `parent`: Lưu nút đi trước trên đường đi ngắn nhất.
  - Bảng băm `parentEdge`: Lưu ID cạnh nối từ `parent[v] -> v`. Cực kỳ quan trọng vì giữa 2 node có thể có nhiều sợi cáp song song, phải lưu chính xác `edgeId`.
  - Đẩy `{0.0, source}` vào Min-Heap.
* **Vòng lặp nới lỏng cạnh (Relaxation Loop - Dòng 47-77):**
  - Lấy đỉnh đầu hàng đợi: `State cur = pq.top(); pq.pop();`.
  - **Loại bỏ trạng thái lỗi thời (Dòng 53-54):**
    `if (visited[cur.node] || cur.dist > dist[cur.node]) continue;`
    Khi một nút được cập nhật khoảng cách ngắn hơn, một bản ghi mới được đẩy vào heap trong khi bản ghi cũ vẫn nằm trong heap. Lệnh này loại bỏ ngay các bản ghi cũ khi chúng được pop ra.
  - Đánh dấu đã chốt đường ngắn nhất: `visited[cur.node] = true;`.
  - Nếu đã đến nút đích `cur.node == destination`: Lập tức thoát vòng lặp (`break`), không cần duyệt phần còn lại của đồ thị.
  - **Duyệt các cạnh kề (Dòng 60-76):**
    - `if (!e.isUp) continue;`: Dây cáp đứt $	o$ Bỏ qua.
    - `if (onlyBuilt && !e.isBuilt) continue;`: **Ràng buộc hạ tầng cốt lõi**: Gói tin chỉ được phép đi trên những đường cáp đã bỏ tiền xây dựng ở Bước 3 & 4 (MST + Backup). Cáp khảo sát trên giấy chưa xây sẽ bị chặn tuyệt đối.
    - Tính khoảng cách mới qua cạnh $e$: `newDist = dist[cur.node] + weight(e);`.
    - Nếu `newDist < dist[v]`: Nới lỏng thành công, cập nhật `dist[v]`, `parent[v]`, `parentEdge[v]` và đẩy `{newDist, v}` vào Min-Heap.
* **Tái tạo đường đi (Path Reconstruction - Dòng 83-99):**
  - Nếu `!visited[destination]`: Không tìm thấy đường tới đích $\to$ Trả về `result.reachable = false`.
  - Ngược lại: Truy ngược vết từ `destination` về `source` qua mảng `parentEdge` và `parent`, sau đó dùng reverse iterator `rbegin(), rend()` để thu được lộ trình xuôi chiều từ nguồn tới đích.

---

# CHƯƠNG 5: XỬ LÝ SỰ CỐ MẠNG, CÁP DỰ PHÒNG & CẠNH CẦU NGUY CẤP (RELIABILITY & FAILURE ANALYSIS)

Chương này phân tích cơ chế chịu lỗi của hệ thống: Khi một sợi cáp chính trên cây MST bị đứt (`LINK_DOWN`), thuật toán sẽ tự động phân tích đồ thị, tách cây làm hai nửa và tìm tuyến cáp dự phòng cứu mạng, hoặc nhận diện xem đó có phải là "Cạnh cầu nguy cấp" (Critical Bridge) làm mạng mất kết nối vĩnh viễn hay không.

---

## 5.1. File `include/Reliability.h` & `src/Reliability.cpp`

### Toàn bộ mã nguồn Header (`include/Reliability.h`):
```cpp
  1: #ifndef RELIABILITY_H
  2: #define RELIABILITY_H
  3: 
  4: #include "Graph.h"
  5: #include <string>
  6: #include <vector>
  7: 
  8: // Kết quả phân tích cáp dự phòng cứu một cạnh cây khung (Tree Edge)
  9: struct BackupCandidate {
 10:     int backupEdgeId;      // ID của dây dự phòng
 11:     double cost;           // Chi phí lắp đặt của dây này
 12:     std::string reason;    // Lý do chọn / phân tích liên thông
 13: };
 14: 
 15: namespace Reliability {
 16: 
 17:     // Task B5.2: Phân tích và tìm cáp dự phòng thay thế cho một cạnh trong cây khung (Tree Edge)
 18:     // - Tách graph (hoặc cây MST) khi giả định tree edge bị đứt
 19:     // - Xác định hai thành phần liên thông bị tách ra
 20:     // - Tìm các cạnh dự phòng đã xây (hoặc ứng viên ngoài cây) kết nối hai phía
 21:     // - Trả về cạnh backup tối ưu nhất (hoặc backupEdgeId = -1 nếu không có đường thay thế)
 22:     BackupCandidate findBackupForTreeEdge(const Graph& graph, 
 23:                                           const std::vector<int>& mstTreeEdges, 
 24:                                           int failedTreeEdgeId);
 25: 
 26:     // Tìm tất cả các cạnh dự phòng có thể thay thế khi một tree edge cụ thể bị đứt
 27:     std::vector<BackupCandidate> findAllBackupsForTreeEdge(const Graph& graph,
 28:                                                           const std::vector<int>& mstTreeEdges,
 29:                                                           int failedTreeEdgeId);
 30: 
 31: } // namespace Reliability
 32: 
 33: #endif // RELIABILITY_H

```

### Toàn bộ mã nguồn Cài đặt (`src/Reliability.cpp`):
```cpp
  1: #include "Reliability.h"
  2: #include "Validation.h"
  3: #include <queue>
  4: #include <unordered_set>
  5: #include <algorithm>
  6: 
  7: namespace Reliability {
  8: 
  9:     // Helper: Tìm các node thuộc thành phần liên thông chứa startNode trong cây MST khi bỏ qua failedEdgeId
 10:     static std::unordered_set<int> getComponentNodes(const Graph& graph, 
 11:                                                     const std::vector<int>& mstTreeEdges, 
 12:                                                     int failedEdgeId, 
 13:                                                     int startNode) {
 14:         std::unordered_set<int> comp;
 15:         std::queue<int> q;
 16: 
 17:         comp.insert(startNode);
 18:         q.push(startNode);
 19: 
 20:         while (!q.empty()) {
 21:             int u = q.front();
 22:             q.pop();
 23: 
 24:             // Duyệt qua các lân cận nhưng CHỈ đi qua các cạnh thuộc MST (trừ cạnh bị đứt failedEdgeId)
 25:             for (int edgeId : graph.getNeighbors(u)) {
 26:                 if (edgeId == failedEdgeId) continue;
 27: 
 28:                 // Kiểm tra xem edgeId này có nằm trong cây MST không
 29:                 bool inMST = false;
 30:                 for (int tId : mstTreeEdges) {
 31:                     if (tId == edgeId) {
 32:                         inMST = true;
 33:                         break;
 34:                     }
 35:                 }
 36:                 if (!inMST) continue;
 37: 
 38:                 int v = graph.otherEndpoint(u, edgeId);
 39:                 if (comp.find(v) == comp.end()) {
 40:                     comp.insert(v);
 41:                     q.push(v);
 42:                 }
 43:             }
 44:         }
 45:         return comp;
 46:     }
 47: 
 48:     std::vector<BackupCandidate> findAllBackupsForTreeEdge(const Graph& graph,
 49:                                                           const std::vector<int>& mstTreeEdges,
 50:                                                           int failedTreeEdgeId) {
 51:         std::vector<BackupCandidate> candidates;
 52: 
 53:         // Kiểm tra failedTreeEdgeId có thực sự nằm trong MST không
 54:         bool found = false;
 55:         for (int tId : mstTreeEdges) {
 56:             if (tId == failedTreeEdgeId) {
 57:                 found = true;
 58:                 break;
 59:             }
 60:         }
 61:         if (!found) return candidates;
 62: 
 63:         const Edge& failedEdge = graph.getEdge(failedTreeEdgeId);
 64: 
 65:         // 1. Tách cây khi remove failedTreeEdge: Tìm 2 thành phần liên thông
 66:         // Nhánh A chứa failedEdge.u, Nhánh B chứa failedEdge.v
 67:         std::unordered_set<int> compA = getComponentNodes(graph, mstTreeEdges, failedTreeEdgeId, failedEdge.u);
 68:         std::unordered_set<int> compB = getComponentNodes(graph, mstTreeEdges, failedTreeEdgeId, failedEdge.v);
 69: 
 70:         // 2. Tìm tất cả các cạnh trong đồ thị (không nằm trong MST) nối giữa 1 node thuộc compA và 1 node thuộc compB
 71:         for (int edgeId : graph.edgeIds()) {
 72:             if (edgeId == failedTreeEdgeId) continue;
 73: 
 74:             // Bỏ qua các cạnh khác của MST
 75:             bool inMST = false;
 76:             for (int tId : mstTreeEdges) {
 77:                 if (tId == edgeId) {
 78:                     inMST = true;
 79:                     break;
 80:                 }
 81:             }
 82:             if (inMST) continue;
 83: 
 84:             const Edge& e = graph.getEdge(edgeId);
 85: 
 86:             // Kiểm tra tính khả thi: cáp phải còn hoạt động (isUp) và thỏa mãn độ dài vật lý
 87:             if (!e.isUp || !Validation::isStaticFeasible(e)) continue;
 88: 
 89:             // Cạnh này có nối giữa hai phía compA và compB không?
 90:             bool connectsAB = (compA.count(e.u) && compB.count(e.v)) ||
 91:                              (compA.count(e.v) && compB.count(e.u));
 92: 
 93:             if (connectsAB) {
 94:                 BackupCandidate cand;
 95:                 cand.backupEdgeId = edgeId;
 96:                 cand.cost = e.length * e.unitPrice * e.terrainFactor + e.equipmentCost + e.maintenanceCost;
 97:                 
 98:                 if (e.isBuilt && e.isBackup) {
 99:                     cand.reason = "Cap du phong da duoc xay dung (isBuilt=true, isBackup=true) noi hai nhanh " 
100:                                 + std::to_string(e.u) + " va " + std::to_string(e.v);
101:                 } else if (e.isBuilt) {
102:                     cand.reason = "Cap mang da xay dung noi giua " + std::to_string(e.u) + " va " + std::to_string(e.v);
103:                 } else {
104:                     cand.reason = "Cap ung vien tiem nang ngoai cay noi giua " + std::to_string(e.u) + " va " + std::to_string(e.v);
105:                 }
106: 
107:                 candidates.push_back(cand);
108:             }
109:         }
110: 
111:         // Sắp xếp ưu tiên: cáp đã xây (isBuilt) lên trước, sau đó tới chi phí rẻ hơn
112:         std::sort(candidates.begin(), candidates.end(), [&](const BackupCandidate& a, const BackupCandidate& b) {
113:             const Edge& ea = graph.getEdge(a.backupEdgeId);
114:             const Edge& eb = graph.getEdge(b.backupEdgeId);
115:             if (ea.isBuilt != eb.isBuilt) return ea.isBuilt > eb.isBuilt;
116:             if (a.cost != b.cost) return a.cost < b.cost;
117:             return a.backupEdgeId < b.backupEdgeId;
118:         });
119: 
120:         return candidates;
121:     }
122: 
123:     BackupCandidate findBackupForTreeEdge(const Graph& graph, 
124:                                           const std::vector<int>& mstTreeEdges, 
125:                                           int failedTreeEdgeId) {
126:         std::vector<BackupCandidate> all = findAllBackupsForTreeEdge(graph, mstTreeEdges, failedTreeEdgeId);
127:         if (all.empty()) {
128:             return {-1, 0.0, "Khong tim thay bat ky cap du phong nao co the noi lai hai phan bi chia cat (Canh cau nguy cap / Critical Bridge)"};
129:         }
130:         return all[0];
131:     }
132: 
133: } // namespace Reliability

```

### Giải thích chi tiết từng dòng code:

#### 1. Cấu trúc ứng viên dự phòng `BackupCandidate` (`include/Reliability.h` dòng 9-13):
* `int backupEdgeId`: ID của sợi cáp ngoài cây có khả năng cứu mạng khi cạnh cây bị đứt. Bằng `-1` nếu không có bất kỳ cáp nào thay thế được.
* `double cost`: Tổng chi phí lắp đặt của sợi cáp dự phòng này.
* `std::string reason`: Chuỗi giải thích kỹ thuật (ví dụ: đã được xây dựng sẵn hay chỉ là ứng viên tiềm năng).

#### 2. Thuật toán tách nhánh BFS `getComponentNodes` (`src/Reliability.cpp` dòng 13-50):
* **Ý nghĩa toán học:** Cây khung (Tree) là đồ thị liên thông không có chu trình. Do đó, **mọi cạnh trên cây đều là Cạnh Cầu (Bridge)**. Khi xóa bỏ 1 cạnh cây `failedEdgeId`, cây MST bắt buộc bị phân rã thành đúng 2 thành phần liên thông riêng biệt (Nhánh A chứa đỉnh $u$ và Nhánh B chứa đỉnh $v$).
* **Thuật toán BFS duyệt tìm nhánh (Dòng 19-48):**
  - Bắt đầu từ đỉnh `startNode`, đẩy vào hàng đợi `std::queue<int> q`.
  - Trong vòng lặp BFS, duyệt qua tất cả các cạnh kề `edgeId` của đỉnh $u$:
    - `if (edgeId == failedEdgeId) continue;`: **Bỏ qua tuyệt đối cạnh đang giả định bị đứt**.
    - `if (!inMST) continue;`: **Chỉ lan truyền trên các cạnh thuộc cây khung MST**, không đi qua cạnh ngoài cây.
    - Lấy đỉnh $v$ ở đầu kia qua `graph.otherEndpoint(u, edgeId)` và thêm vào tập hợp `comp` (`std::unordered_set<int>`).
  - Kết quả trả về tập hợp tất cả các đỉnh thuộc cùng một nhánh cây.

#### 3. Thuật toán tìm toàn bộ cáp dự phòng `findAllBackupsForTreeEdge` (Dòng 52-121):
* **Bước 1 (Dòng 56-62):** Kiểm tra xem `failedTreeEdgeId` có thực sự nằm trong cây MST không. Nếu không thuộc MST thì không cần tìm backup.
* **Bước 2 (Dòng 66-68):** Tách cây thành 2 thành phần:
  - `compA = getComponentNodes(graph, mstTreeEdges, failedTreeEdgeId, failedEdge.u);`
  - `compB = getComponentNodes(graph, mstTreeEdges, failedTreeEdgeId, failedEdge.v);`
* **Bước 3 (Dòng 71-105 - Quét cạnh bắc cầu):**
  - Quét qua toàn bộ các cạnh trong đồ thị: Bỏ qua cạnh đang đứt và bỏ qua các cạnh khác đã thuộc MST.
  - Lọc điều kiện: Cáp phải còn sống (`e.isUp == true`) và đạt tiêu chuẩn cự ly vật lý (`Validation::isStaticFeasible(e)`).
  - **Điều kiện bắc cầu:** Cạnh $e$ phải có 1 đầu thuộc `compA` và đầu kia thuộc `compB`:
    `bool connectsAB = (compA.count(e.u) && compB.count(e.v)) || (compA.count(e.v) && compB.count(e.u));`
  - Nếu thỏa mãn: Tạo đối tượng `BackupCandidate`, tính chi phí và ghi rõ lý do.
* **Bước 4 (Dòng 108-118 - Sắp xếp ưu tiên):**
  - **Ưu tiên 1:** Cáp đã được triển khai xây dựng sẵn trong thực tế (`isBuilt == true`). Cáp đã chôn dưới đất luôn sẵn sàng gánh tải ngay lập tức.
  - **Ưu tiên 2:** Chi phí rẻ hơn (`a.cost < b.cost`).
  - **Ưu tiên 3:** ID nhỏ hơn (`a.backupEdgeId < b.backupEdgeId`).

#### 4. Hàm `findBackupForTreeEdge` (Dòng 123-134):
* Gọi `findAllBackupsForTreeEdge`.
* **Phát hiện Cạnh Cầu Nguy Cấp (Critical Bridge):** Nếu danh sách ứng viên trả về rỗng (`all.empty()`), nghĩa là giữa Nhánh A và Nhánh B **hoàn toàn không còn bất kỳ sợi cáp nào khác** $	o$ Trả về `backupEdgeId = -1` và cảnh báo: *"Cạnh cầu nguy cấp / Mất liên lạc hoàn toàn"*.
* Nếu có ứng viên: Trả về ứng viên số 1 (`all[0]`).

---

# CHƯƠNG 6: ĐỘNG CƠ MÔ PHỎNG SỰ KIỆN THEO THỜI GIAN & GHI LOG (SIMULATOR & LOGGER)

Chương này trình bày "trái tim vận hành" của đồ án: Nhận kịch bản biến cố theo trục thời gian $t$, đóng băng trạng thái mạng tại từng thời điểm (Snapshot), mô phỏng truyền các gói tin và ghi nhật ký toàn diện ra file text.

---

## 6.1. File `include/Simulator.h` & `src/Simulator.cpp`

### Toàn bộ mã nguồn Header (`include/Simulator.h`):
```cpp
  1: #ifndef SIMULATOR_H
  2: #define SIMULATOR_H
  3: 
  4: #include <unordered_map>
  5: #include <string>
  6: #include "Graph.h"
  7: #include "Scenario.h"
  8: #include "Packet.h"
  9: #include "PathResult.h"
 10: 
 11: // Task B4.5: Kết quả mô phỏng truyền gói tin
 12: struct SimulationResult {
 13:     bool success;
 14:     double totalDelay;
 15:     std::string failReason;
 16:     int fragmentCount;
 17: };
 18: 
 19: // Task B4.1: NetworkSnapshot
 20: // Lưu trữ bản sao trạng thái của mạng tại một thời điểm
 21: struct NetworkSnapshot {
 22:     std::unordered_map<int, bool> isUp;
 23:     std::unordered_map<int, double> load;
 24:     std::unordered_map<int, double> queueDelay;
 25: };
 26: 
 27: class Simulator {
 28: public:
 29:     // Task B4.2: Hàm tạo snapshot
 30:     static NetworkSnapshot snapshotNetworkState(const Graph& graph);
 31:     
 32:     // Task B4.4: Hàm áp dụng sự kiện vào đồ thị
 33:     static void applyEvent(Graph& graph, const ScenarioEvent& event);
 34:     
 35:     // Task B4.5: Gửi gói tin
 36:     static SimulationResult sendPacket(const Packet& packet, const PathResult& path, Graph& graph);
 37:     
 38:     // Task B4.6: Cập nhật trạng thái tải sau khi gửi
 39:     static void updateNetworkState(Edge& edge, double dTrans);
 40: 
 41:     // Task B6.1 & B6.5: Điều phối toàn bộ một kịch bản sự kiện & Logging ra file .txt
 42:     // Tách vòng lặp mô phỏng ra khỏi main.cpp để giữ cho main.cpp chỉ đóng vai trò Orchestrator
 43:     static void runScenarioSimulation(Graph& graph,
 44:                                      const std::vector<Packet>& packets,
 45:                                      const Scenario& scenario,
 46:                                      const std::string& logFilePath = "");
 47: };
 48: 
 49: #endif // SIMULATOR_H

```

### Toàn bộ mã nguồn Cài đặt (`src/Simulator.cpp`):
```cpp
  1: #include "Simulator.h"
  2: #include "DelayModel.h"
  3: #include "Config.h" // Chứa LOAD_WINDOW_SECONDS và LOAD_DECAY
  4: #include "Dijkstra.h"
  5: #include "RoutingCost.h"
  6: #include "EnumUtils.h"
  7: #include <algorithm>
  8: #include <iostream>
  9: #include <iomanip>
 10: 
 11: // Task B4.2: snapshotNetworkState()
 12: // Mục đích: Dijkstra chạy trên trạng thái cố định trong một lần tính.
 13: NetworkSnapshot Simulator::snapshotNetworkState(const Graph& graph) {
 14:     NetworkSnapshot snap;
 15:     
 16:     // Copy isUp, load, queueDelay từ graph sang snapshot
 17:     for (int edgeId : graph.edgeIds()) {
 18:         const Edge& edge = graph.getEdge(edgeId);
 19:         
 20:         snap.isUp[edgeId] = edge.isUp;
 21:         snap.load[edgeId] = edge.currentLoad;
 22:         snap.queueDelay[edgeId] = edge.queueDelay;
 23:     }
 24:     
 25:     return snap;
 26: }
 27: 
 28: // Task B4.4: applyEvent()
 29: // Mục đích: Áp dụng các sự kiện thay đổi trạng thái mạng (Load, Delay)
 30: void Simulator::applyEvent(Graph& graph, const ScenarioEvent& event) {
 31:     if (!graph.hasEdge(event.targetId)) {
 32:         std::cerr << "[Simulator] Canh bao: Event bo qua do Edge #" << event.targetId << " khong ton tai trong do thi!\n";
 33:         return;
 34:     }
 35: 
 36:     Edge& targetEdge = graph.getEdge(event.targetId);
 37: 
 38:     switch (event.eventType) {
 39:         case EventType::LOAD_UPDATE:
 40:             // Đảm bảo load không vượt quá 1.0 và không nhỏ hơn 0.0
 41:             targetEdge.currentLoad = std::max(0.0, std::min(1.0, event.value));
 42:             break;
 43: 
 44:         case EventType::DELAY_UPDATE:
 45:             // Đảm bảo delay không âm
 46:             targetEdge.queueDelay = std::max(0.0, event.value);
 47:             break;
 48: 
 49:         case EventType::LINK_DOWN:
 50:             // Làm đứt/hỏng đường truyền vật lý
 51:             targetEdge.isUp = false;
 52:             break;
 53: 
 54:         case EventType::LINK_UP:
 55:             // Phục hồi lại kết nối của cáp
 56:             targetEdge.isUp = true;
 57:             break;
 58: 
 59:         default:
 60:             break;
 61:     }
 62: }
 63: 
 64: // Task B4.5: sendPacket()
 65: SimulationResult Simulator::sendPacket(const Packet& packet, const PathResult& path, Graph& graph) {
 66:     SimulationResult result;
 70: 
 71:     // Nếu Dijkstra trả về không có đường đi (reachable = false)
 72:     if (!path.reachable || path.edges.empty()) {
 73:         result.success = false;
 74:         result.failReason = "NO_ROUTE_AVAILABLE";
 72:         return result;
 73:     }
 74: 
 75:     // Duyệt qua từng cạnh trên đường đi
 76:     for (size_t i = 0; i < path.edges.size(); ++i) {
 77:         int edgeId = path.edges[i];
 78:         if (!graph.hasEdge(edgeId)) {
 79:             result.success = false;
 80:             result.failReason = "EDGE_NOT_FOUND_ERROR";
 81:             return result;
 82:         }
 83:         Edge& edge = graph.getEdge(edgeId);
 84:         
 85:         // Tìm nextNode (sử dụng node tiếp theo trong path.nodes nếu có)
 86:         // Lấy ID node tiếp theo để DelayModel tính toán Processing Delay
 87:         int nextNodeId = (i + 1 < path.nodes.size()) ? path.nodes[i + 1] : graph.otherEndpoint(path.nodes[i], edgeId);
 88:         if (!graph.hasNode(nextNodeId)) {
 89:             result.success = false;
 90:             result.failReason = "NODE_NOT_FOUND_ERROR";
 91:             return result;
 92:         }
 93:         const Node& nextNode = graph.getNode(nextNodeId);
 94: 
 95:             // Kiểm tra MTU & Tính số mảnh bằng hàm có sẵn của DelayModel (Task B3.7)
 96:             int fragments = DelayModel::calcFragmentCount(packet.sizeBytes, edge.mtu, packet.canFragment);
 97:             if (fragments == 0) { // Trả về 0 nghĩa là không cho phép phân mảnh và quá kích thước
 98:                 result.success = false;
 99:                 result.failReason = "DF_BLOCKED_BY_MTU_AT_EDGE_" + std::to_string(edgeId);
100:                 return result; 
101:             }
102:             // Cập nhật fragmentCount (mảnh lớn nhất trên toàn bộ đường đi)
103:             result.fragmentCount = std::max(result.fragmentCount, fragments);
104: 
105:             // Tính độ trễ từng chặng (Hop Delay), tránh cộng dồn TCP handshake nhiều lần
106:             double edgeDelay = DelayModel::calculateHopDelay(packet, edge, nextNode);
107:             if (edgeDelay == Config::INF) {
108:                 result.success = false;
109:                 result.failReason = "LINK_DOWN_OR_UNREACHABLE";
110:                 return result;
111:             }
112:             result.totalDelay += edgeDelay;
113: 
114:             // CHÚ Ý DRY: Tính lại dTrans để phục vụ updateNetworkState 
115:             double dTrans = DelayModel::calcTransmissionDelay(packet.sizeBytes, edge.bandwidthMbps);
116:             
117:             // Cập nhật tải 
118:             updateNetworkState(edge, dTrans);
119:     }
120: 
121:     // Cộng TCP overhead một lần duy nhất vào tổng hành trình (Task B3.8)
122:     if (packet.protocol == Protocol::TCP) {
123:         // Dùng totalDelay vừa tính được để ước tính RTT (Round Trip Time)
124:         result.totalDelay += DelayModel::calcProtocolOverhead(packet, result.totalDelay);
125:     }
126: 
127:     return result;
128: }
129: 
130: // Task B4.6: updateNetworkState()
131: void Simulator::updateNetworkState(Edge& edge, double dTrans) {
132:     // Sử dụng giá trị từ Config, hoặc fallback về 1.0/0.99 nếu Config không có
133:     double loadWindow = 1.0; 
134:     double loadDecay = 0.99;
135:     
136:     // loadDelta = dTrans / LOAD_WINDOW_SECONDS
137:     double loadDelta = dTrans / loadWindow; 
138:     
139:     // currentLoad = min(1.0, currentLoad + loadDelta)
140:     edge.currentLoad = std::max(0.0, std::min(1.0, edge.currentLoad + loadDelta));
141:     
142:     // Có thể decay: currentLoad *= LOAD_DECAY
143:     edge.currentLoad *= loadDecay;
144: }
145: 
146: // Task B6.1: runScenarioSimulation()
147: // Thực thi trọn vẹn kịch bản mô phỏng sự kiện, in ấn kết quả tự động, giúp main.cpp hoàn toàn sạch sẽ
148: #include "Logger.h"
149: 
150: // Task B6.1, B6.2 & B6.5: runScenarioSimulation()
151: // Thực thi trọn vẹn kịch bản mô phỏng sự kiện, in ấn kết quả tự động, và xuất log ra file .txt nếu có yêu cầu
152: void Simulator::runScenarioSimulation(Graph& graph,
153:                                      const std::vector<Packet>& packets,
154:                                      const Scenario& scenario,
155:                                      const std::string& logFilePath) {
156:     std::cout << "\n========================================================================\n";
157:     std::cout << "        TIEN HANH CHAY KICH BAN MO PHONG: " << scenario.name << "\n";
158:     std::cout << "========================================================================\n";
159:     std::cout << "-> Tong so su kien duoc nap: " << scenario.events.size() << "\n";
160: 
161:     std::vector<Logger::SimulationLogEntry> logEntries;
162: 
163:     if (scenario.events.empty()) {
164:         std::cout << "(Kich ban tinh - Khong co su kien dong, tien hanh kiem tra truyen goi tin truc tiep):\n";
165:         for (const Packet& pkt : packets) {
166:             PathResult path = dijkstra(graph, pkt.source, pkt.destination,
167:                                        RoutingCost::makeWeightFn(pkt), /*onlyBuilt=*/true);
168: 
169:             std::string pktTypeStr = EnumUtils::toString(pkt.type);
170: 
171:             std::cout << "  * Packet #" << pkt.id << " (" << pkt.source << "->" << pkt.destination 
172:                       << ", " << pkt.sizeBytes << "B, DF=" << (pkt.canFragment ? "NO" : "YES") << ", " << pktTypeStr << "): ";
173: 
174:             Logger::SimulationLogEntry entry;
175:             entry.timestamp = 0.0;
176:             entry.packetId = pkt.id;
177:             entry.source = pkt.source;
178:             entry.destination = pkt.destination;
179:             entry.packetType = pktTypeStr;
180:             entry.failureEvent = "";
181:             entry.isRerouted = false;
182: 
183:             if (path.reachable) {
184:                 std::string routeStr = "";
185:                 for (size_t i = 0; i < path.nodes.size(); ++i) {
186:                     routeStr += std::to_string(path.nodes[i]) + (i + 1 < path.nodes.size() ? "->" : "");
187:                 }
188:                 std::cout << "Route: " << routeStr;
189:                 entry.path = routeStr;
190:                 entry.hopCount = path.hopCount;
191: 
192:                 SimulationResult res = sendPacket(pkt, path, graph);
193:                 entry.fragmentCount = res.fragmentCount;
194:                 if (res.success) {
195:                     entry.status = "SUCCESS";
196:                     entry.delay = res.totalDelay;
197:                     entry.dropReason = "";
198:                     std::cout << " => SUCCESS (Delay: " << std::fixed << std::setprecision(6) << res.totalDelay 
199:                               << "s, So manh Fragment: " << res.fragmentCount << ")\n";
200:                 } else {
201:                     entry.status = "DROP";
202:                     entry.delay = 0.0;
203:                     entry.dropReason = res.failReason;
204:                     std::cout << " => DROP (" << res.failReason << ")\n";
205:                 }
206:             } else {
207:                 std::cout << "=> DROP (UNREACHABLE: Khong tim thay duong di)\n";
208:                 entry.path = "UNREACHABLE";
209:                 entry.hopCount = 0;
210:                 entry.fragmentCount = 0;
211:                 entry.status = "DROP";
212:                 entry.delay = 0.0;
213:                 entry.dropReason = "NO_PATH_AVAILABLE";
214:             }
215:             logEntries.push_back(entry);
216:         }
217:         std::cout << "========================================================================\n";
218:         if (!logFilePath.empty()) {
219:             if (Logger::exportSimulationLog(logFilePath, scenario.name, logEntries)) {
220:                 std::cout << "[Logger] Da xuat log mo phong thanh cong ra: " << logFilePath << "\n";
221:             }
222:         }
223:         return;
224:     }
225: 
226:     // Nhóm các sự kiện theo từng mốc thời gian (Time) để tránh in lặp lại nhiều lần
227:     std::vector<std::pair<double, std::vector<ScenarioEvent>>> timeGroups;
228:     for (const auto& ev : scenario.events) {
229:         if (timeGroups.empty() || std::abs(timeGroups.back().first - ev.time) > 1e-6) {
230:             timeGroups.push_back({ev.time, {ev}});
231:         } else {
232:             timeGroups.back().second.push_back(ev);
233:         }
234:     }
235: 
236:     // Lưu lại lộ trình cũ của từng gói tin để nhận biết khi nào có sự thay đổi (Reroute)
237:     std::unordered_map<int, std::vector<int>> lastRoute;
238:     std::unordered_map<int, bool> lastSuccess;
239: 
240:     for (const auto& group : timeGroups) {
241:         double t = group.first;
242:         std::cout << "\n------------------------------------------------------------------------\n";
243:         std::cout << "[Thoi diem: " << std::fixed << std::setprecision(1) << t << "s]\n";
244: 
245:         std::string failureSummary = "";
246: 
247:         // 1. Áp dụng tất cả sự kiện xảy ra tại thời điểm t
248:         for (const auto& ev : group.second) {
249:             std::string evDesc = "";
250:             if (ev.eventType == EventType::LOAD_UPDATE) {
251:                 std::cout << "  * Su kien: LOAD_UPDATE tren Edge #" << ev.targetId 
252:                           << " -> Tai moi: " << std::setprecision(2) << ev.value * 100 << "%\n";
253:                 evDesc = "LOAD_UPDATE Edge #" + std::to_string(ev.targetId);
254:             } else if (ev.eventType == EventType::DELAY_UPDATE) {
255:                 std::cout << "  * Su kien: DELAY_UPDATE tren Edge #" << ev.targetId 
256:                           << " -> Trở ngai moi: " << std::setprecision(4) << ev.value << "s\n";
257:                 evDesc = "DELAY_UPDATE Edge #" + std::to_string(ev.targetId);
258:             } else if (ev.eventType == EventType::LINK_DOWN) {
259:                 std::cout << "  * CANH BAO: LINK_DOWN tren Edge #" << ev.targetId << " (Cap bi dut / hong hoc!)\n";
260:                 evDesc = "LINK_DOWN Edge #" + std::to_string(ev.targetId);
261:             } else if (ev.eventType == EventType::LINK_UP) {
262:                 std::cout << "  * PHUC HOI: LINK_UP tren Edge #" << ev.targetId << " (Cap da hoat dong tro lai)\n";
263:                 evDesc = "LINK_UP Edge #" + std::to_string(ev.targetId);
264:             }
265:             if (!evDesc.empty()) {
266:                 if (!failureSummary.empty()) failureSummary += ", ";
267:                 failureSummary += evDesc;
268:             }
269:             applyEvent(graph, ev);
270:         }
271: 
272:         // 2. Chụp snapshot trạng thái mạng cố định tại mốc t
273:         NetworkSnapshot snap = snapshotNetworkState(graph);
274: 
275:         // 3. Mô phỏng truyền các gói tin tại thời điểm t
276:         std::cout << "  -> Ket qua dinh tuyen va truyen goi tin:\n";
277:         for (const Packet& pkt : packets) {
278:             PathResult path = dijkstra(graph, pkt.source, pkt.destination,
279:                                        RoutingCost::makeWeightFn(pkt), /*onlyBuilt=*/true);
280: 
281:             std::string pktTypeStr = EnumUtils::toString(pkt.type);
282: 
283:             std::cout << "     Packet #" << pkt.id << " (" << pkt.source << "->" << pkt.destination 
284:                       << ", " << pktTypeStr << "): ";
285: 
286:             Logger::SimulationLogEntry entry;
287:             entry.timestamp = t;
288:             entry.packetId = pkt.id;
289:             entry.source = pkt.source;
290:             entry.destination = pkt.destination;
291:             entry.packetType = pktTypeStr;
292:             entry.failureEvent = failureSummary;
293: 
294:             if (path.reachable) {
295:                 std::string routeStr = "";
296:                 for (size_t i = 0; i < path.nodes.size(); ++i) {
297:                     routeStr += std::to_string(path.nodes[i]) + (i + 1 < path.nodes.size() ? "->" : "");
298:                 }
299: 
300:                 bool routeChanged = (lastRoute.find(pkt.id) != lastRoute.end() && lastRoute[pkt.id] != path.nodes);
301:                 entry.isRerouted = routeChanged;
302:                 entry.path = routeStr;
303:                 entry.hopCount = path.hopCount;
304: 
305:                 if (routeChanged) {
306:                     std::cout << " [DA DOI DUONG (REROUTE)] Route: " << routeStr;
307:                 } else {
308:                     std::cout << " Route: " << routeStr;
309:                 }
310:                 lastRoute[pkt.id] = path.nodes;
311: 
312:                 SimulationResult res = sendPacket(pkt, path, graph);
313:                 entry.fragmentCount = res.fragmentCount;
314:                 if (res.success) {
315:                     entry.status = "SUCCESS";
316:                     entry.delay = res.totalDelay;
317:                     entry.dropReason = "";
318:                     std::cout << " => SUCCESS (Delay: " << std::setprecision(6) << res.totalDelay 
319:                               << "s, Frags: " << res.fragmentCount << ")\n";
320:                 } else {
321:                     entry.status = "DROP";
322:                     entry.delay = 0.0;
323:                     entry.dropReason = res.failReason;
324:                     std::cout << " => DROP (" << res.failReason << ")\n";
325:                 }
326:                 lastSuccess[pkt.id] = res.success;
327:             } else {
328:                 std::cout << "=> DROP (UNREACHABLE: Mang bi chia cat, khong con duong)\n";
329:                 lastRoute[pkt.id] = {};
330:                 lastSuccess[pkt.id] = false;
331:                 entry.path = "UNREACHABLE";
332:                 entry.hopCount = 0;
333:                 entry.fragmentCount = 0;
334:                 entry.status = "DROP";
335:                 entry.delay = 0.0;
336:                 entry.dropReason = "NO_PATH_AVAILABLE";
337:                 entry.isRerouted = false;
338:             }
339:             logEntries.push_back(entry);
340:         }
341:     }
342:     std::cout << "========================================================================\n";
343:     if (!logFilePath.empty()) {
344:         if (Logger::exportSimulationLog(logFilePath, scenario.name, logEntries)) {
345:             std::cout << "[Logger] Da xuat log mo phong thanh cong ra: " << logFilePath << "\n";
346:         }
347:     }
348: }

```

### Giải thích chi tiết từng dòng code:

#### 1. Đóng băng trạng thái mạng `NetworkSnapshot` (Dòng 12-25 trong .cpp):
* **Ý nghĩa:** Tránh hiện tượng tranh chấp trạng thái (Race Condition). Dijkstra cần tính toán trên một "bức ảnh chụp" cố định của mạng tại mốc thời gian $t$.
* Sao chép toàn bộ `isUp`, `currentLoad` và `queueDelay` của các cạnh vào cấu trúc `NetworkSnapshot`.

#### 2. Áp dụng sự kiện biến cố `applyEvent` (Dòng 30-61):
* Nhận vào `ScenarioEvent`. Kiểm tra phòng thủ `if (!graph.hasEdge(event.targetId))`.
* Cập nhật trực tiếp vào đối tượng `Edge& targetEdge`:
  - `LOAD_UPDATE`: Cập nhật tải, kẹp giá trị an toàn trong đoạn $[0.0, 1.0]$.
  - `DELAY_UPDATE`: Cập nhật trễ hàng đợi, đảm bảo $\ge 0$.
  - `LINK_DOWN`: Đặt `isUp = false`.
  - `LINK_UP`: Đặt `isUp = true`.

#### 3. Mô phỏng truyền gói tin `sendPacket` (Dòng 65-125):
* Nếu `!path.reachable`: Gói tin bị DROP với lý do `NO_ROUTE_AVAILABLE`.
* Duyệt qua từng chặng `edgeId` trên lộ trình:
  - Kiểm tra `hasEdge` và `hasNode`.
  - Kiểm tra MTU: `DelayModel::calcFragmentCount`. Nếu gói không cho phép phân mảnh mà kích thước quá lớn $	o$ DROP với mã `DF_BLOCKED_BY_MTU_AT_EDGE_X`.
  - Cập nhật số mảnh lớn nhất: `result.fragmentCount = std::max(result.fragmentCount, fragments);`.
  - Tính trễ một chặng qua `DelayModel::calculateHopDelay(packet, edge, nextNode)`.
  - Cộng dồn trễ chặng vào `result.totalDelay`.
  - **Cập nhật tải động (Dòng 115):** Gọi `updateNetworkState(edge, dTrans)`: Thời gian truyền gói tin $d_{trans}$ sẽ làm tăng tải tức thời trên cạnh.
* **Cộng TCP Handshake một lần duy nhất (Dòng 127-130):**
  - Sau khi duyệt hết tất cả các chặng, nếu là `TCP` và `newConnection == true`:
    `result.totalDelay += DelayModel::calcProtocolOverhead(packet, result.totalDelay);`
    Đảm bảo thời gian bắt tay $1 \times RTT$ chỉ cộng đúng 1 lần cho cả hành trình đầu-cuối.

#### 4. Vòng lặp mô phỏng kịch bản `runScenarioSimulation` (Dòng 137-347):
* **Kịch bản tĩnh (Dòng 159-217):** Nếu file kịch bản không có sự kiện nào (như `basic.txt`), hệ thống bắn trực tiếp các gói tin tại $t = 0.0s$ và xuất log.
* **Kịch bản động (Dòng 219-346):**
  - **Gom nhóm sự kiện theo mốc thời gian (Dòng 220-227):** Gom các sự kiện xảy ra cùng thời điểm $t$ vào vector `timeGroups`.
  - Tại mỗi mốc thời gian $t$:
    1. Áp dụng tất cả sự kiện xảy ra tại $t$ (`applyEvent`).
    2. Chụp `NetworkSnapshot` trạng thái mạng.
    3. Duyệt qua từng gói tin trong danh sách:
       - Chạy **Dijkstra động** với `RoutingCost::makeWeightFn(pkt)`.
       - **Phát hiện đổi đường (Reroute Detection - Dòng 294):** So sánh `path.nodes` với `lastRoute[pkt.id]`. Nếu khác nhau, lập tức gắn cờ `routeChanged = true` và in thông báo `[DA DOI DUONG (REROUTE)]`.
       - Cho gói tin chạy qua `sendPacket()`.
       - In kết quả ra console: `SUCCESS (Delay, Frags)` hoặc `DROP (Reason)`.
       - Ghi lại bản ghi log `entry`.
  - Kết thúc kịch bản: Tự động gọi `Logger::exportSimulationLog()` ghi file `data/logs/log_<scenario>.txt`.

---

## 6.2. File `include/Logger.h` & `src/Logger.cpp`

### Toàn bộ mã nguồn Header (`include/Logger.h`):
```cpp
  1: #ifndef LOGGER_H
  2: #define LOGGER_H
  3: 
  4: #include <string>
  5: #include <vector>
  6: #include <iostream>
  7: #include "Graph.h"
  8: #include "Packet.h"
  9: #include "Scenario.h"
 10: 
 11: namespace Logger {
 12:     struct SimulationLogEntry {
 13:         double timestamp;         
 14:         int packetId;              
 15:         int source;                
 16:         int destination;           
 17:         std::string packetType;   
 18:         std::string path;        
 19:         double delay;             
 20:         int hopCount;             
 21:         int fragmentCount;        
 22:         std::string status;        
 23:         std::string dropReason;   
 24:         std::string failureEvent;  
 25:         bool isRerouted;          
 26:     };
 27: 
 28:     bool exportSimulationLog(const std::string& filepath,
 29:                              const std::string& scenarioName,
 30:                              const std::vector<SimulationLogEntry>& logs);
 31: 
 32: } 
 33: 
 34: #endif 

```

### Toàn bộ mã nguồn Cài đặt (`src/Logger.cpp`):
```cpp
  1: #include "Logger.h"
  2: #include <iostream>
  3: #include <fstream>
  4: #include <iomanip>
  5: #include <vector>
  6: 
  7: namespace Logger {
  8: 
  9: bool exportSimulationLog(const std::string& filepath,
 10:                          const std::string& scenarioName,
 11:                          const std::vector<SimulationLogEntry>& logs) {
 12:     std::ofstream outFile(filepath);
 13:     if (!outFile.is_open()) {
 14:         std::cerr << "Loi: Khong the mo hoac tao file log: " << filepath << "\n";
 15:         return false;
 16:     }
 17: 
 18:     outFile << "========================================================================================================\n";
 19:     outFile << "                               NHAT KY MO PHONG MANG (SIMULATION LOG)                                   \n";
 20:     outFile << "Kich ban (Scenario) : " << scenarioName << "\n";
 21:     outFile << "Tong so ban ghi log : " << logs.size() << "\n";
 22:     outFile << "========================================================================================================\n\n";
 23: 
 24:     outFile << std::left
 25:             << std::setw(8)  << "Time(s)"
 26:             << std::setw(8)  << "Packet"
 27:             << std::setw(6)  << "Src"
 28:             << std::setw(6)  << "Dst"
 29:             << std::setw(6)  << "Hop"
 30:             << std::setw(8)  << "Frag"
 31:             << std::setw(12) << "Delay(s)"
 32:             << std::setw(10) << "Status"
 33:             << std::setw(20) << "Path"
 34:             << std::setw(28) << "Failure / Note"
 35:             << "\n";
 36:     outFile << "--------------------------------------------------------------------------------------------------------\n";
 37: 
 38:     for (const auto& entry : logs) {
 39:         std::string delayStr = (entry.status == "SUCCESS") ? std::to_string(entry.delay) : "-";
 40:         if (entry.status == "SUCCESS") {
 41:             // Định dạng 6 chữ số thập phân cho delay
 42:             char buf[32];
 43:             std::snprintf(buf, sizeof(buf), "%.6f", entry.delay);
 44:             delayStr = buf;
 45:         }
 46: 
 47:         std::string note = "";
 48:         if (!entry.failureEvent.empty()) {
 49:             note = entry.failureEvent;
 50:         }
 51:         if (entry.status != "SUCCESS" && !entry.dropReason.empty()) {
 52:             if (!note.empty()) note += " | ";
 53:             note += "DROP: " + entry.dropReason;
 54:         } else if (entry.isRerouted) {
 55:             if (!note.empty()) note += " | ";
 56:             note += "[REROUTED]";
 57:         }
 58: 
 59:         outFile << std::left
 60:                 << std::setw(8)  << std::fixed << std::setprecision(1) << entry.timestamp
 61:                 << std::setw(8)  << ("#" + std::to_string(entry.packetId))
 62:                 << std::setw(6)  << entry.source
 63:                 << std::setw(6)  << entry.destination
 64:                 << std::setw(6)  << (entry.status == "SUCCESS" ? std::to_string(entry.hopCount) : "-")
 65:                 << std::setw(8)  << entry.fragmentCount
 66:                 << std::setw(12) << delayStr
 67:                 << std::setw(10) << entry.status
 68:                 << std::setw(20) << entry.path
 69:                 << note
 70:                 << "\n";
 71:     }
 72: 
 73:     outFile << "--------------------------------------------------------------------------------------------------------\n";
 74:     outFile << "Ket thuc nhat ky mo phong.\n";
 75:     outFile.close();
 76:     return true;
 77: }
 78: 
 79: } 

```

### Giải thích chi tiết từng dòng code:
* **Mục đích:** Đảm bảo tính minh bạch và khả năng tái lập (Reproducibility) của các thí nghiệm mô phỏng. Toàn bộ diễn biến mô phỏng được lưu lại vĩnh viễn dưới dạng file text.
* **Cấu trúc bản ghi `SimulationLogEntry` (`include/Logger.h` dòng 10-23):**
  - `timestamp`: Thời điểm mô phỏng (giây).
  - `packetId`, `source`, `destination`, `packetType`.
  - `path`: Chuỗi lộ trình (ví dụ `"0->2->3"`).
  - `hopCount`, `delay`, `fragmentCount`.
  - `status`: `"SUCCESS"` hoặc `"DROP"`.
  - `dropReason`: Nguyên nhân rớt gói.
  - `failureEvent`: Biến cố mạng đã xảy ra tại mốc đó.
  - `isRerouted`: Cờ đánh dấu gói tin vừa phải đổi đường tránh sự cố.
* **Hàm `exportSimulationLog` (`src/Logger.cpp` dòng 9-73):**
  - Mở file đầu ra qua `std::ofstream`.
  - Định dạng bảng căn cột chuyên nghiệp bằng `std::left`, `std::setw`, `std::fixed`, `std::setprecision(6)`.
  - Ghi tiêu đề kịch bản, thời gian, và in chi tiết từng dòng log.

---

# CHƯƠNG 7: BÁO CÁO KẾT QUẢ & NHẠC TRƯỞNG ĐIỀU PHỐI (REPORTER & MAIN ORCHESTRATOR)

Chương này trình bày tầng trình diễn thông tin (Presentation Layer) và điểm vào chính của chương trình (`main.cpp`), nơi toàn bộ các module được ráp nối hài hòa theo mô hình Orchestrator.

---

## 7.1. File `include/Reporter.h` & `src/Reporter.cpp`

### Toàn bộ mã nguồn Header (`include/Reporter.h`):
```cpp
  1: #ifndef REPORTER_H
  2: #define REPORTER_H
  3: 
  4: #include "Graph.h"
  5: #include "Packet.h"
  6: #include "Dijkstra.h"
  7: 
  8: #include "CostModel.h"
  9: 
 10: namespace Reporter {
 11:     // 1. Bài toán Quy hoạch đi dây (Kruskal)
 12:     // In báo cáo chi tiết về kết quả cây khung nhỏ nhất (MST) và các cáp dự phòng.
 13:     // So sánh chi phí tổng thể giữa: Xây tất cả, Chỉ xây MST, và Xây MST + Backup.
 14:     void printMSTReport(const Graph& graph, const CostModel& costModel);
 15: 
 16:     // 2. In Bảng định tuyến (Routing Table) cho toàn bộ các cặp đỉnh trong mạng
 17:     void printFullRoutingTable(const Graph& graph, const WeightFn& weight = lengthWeight);
 18: 
 19:     // 3. Bài toán Mô phỏng truyền gói tin (Data Transmission & Delay)
 20:     // Voi moi Packet, dung Dijkstra tim duong di, roi cong don do tre tung chang.
 21:     void printPacketRouteReport(const Graph& graph, const std::vector<Packet>& packets, const WeightFn& weight = lengthWeight);
 22: 
 23:     // 4. Báo cáo xử lý sự cố & Định tuyến lại (Task B5.3)
 24:     // Hiển thị OLD PATH -> FAILED EDGE -> NEW PATH theo đúng mẫu chuẩn README
 25:     void printFailureRecoveryReport(const Graph& graph,
 26:                                     const Packet& pkt,
 27:                                     const PathResult& oldPath,
 28:                                     int failedEdgeId,
 29:                                     const PathResult& newPath,
 30:                                     const std::string& note = "");
 31: }
 32: 
 33: #endif

```

### Toàn bộ mã nguồn Cài đặt (`src/Reporter.cpp`):
```cpp
  1: #include "Reporter.h"
  2: #include "CostModel.h"
  3: #include "Config.h"
  4: #include <iostream>
  5: #include <iomanip>
  6: #include <string>
  7: #include "DelayModel.h"
  8: #include "EnumUtils.h"
  9: 
 10: namespace Reporter {
 11: 
 12: void printMSTReport(const Graph& graph, const CostModel& costModel) {
 13:     std::cout << "==============================================================\n";
 14:     std::cout << "               BAO CAO KET QUA QUY HOACH CAP                  \n";
 15:     std::cout << "==============================================================\n";
 16:     
 17:     std::cout << std::left 
 18:               << std::setw(8)  << "EdgeID" 
 19:               << std::setw(8)  << "u-v" 
 20:               << std::setw(10) << "Media" 
 21:               << std::setw(15) << "Role" 
 22:               << std::setw(15) << "Cost (DV)" 
 23:               << "\n";
 24:     std::cout << "--------------------------------------------------------------\n";
 25: 
 26:     double totalBuildAll = 0.0;
 27:     double totalMST = 0.0;
 28:     double totalMSTBackup = 0.0;
 29: 
 30:     std::vector<int> edgeIds = graph.edgeIds();
 31:     for (int id : edgeIds) {
 32:         const Edge& edge = graph.getEdge(id);
 33:         
 34:         // Bỏ qua các cạnh đứt
 35:         if (!edge.isUp) continue;
 36: 
 37:         double cost = costModel.calculateCablingCost(edge).total;
 38:         
 39:         // Build All: cộng tất cả cạnh có thể đi được
 40:         if (cost != Config::INF) {
 41:             totalBuildAll += cost;
 42:         }
 43: 
 44:         std::string role = "None";
 45:         if (edge.isBackup) {
 46:             role = "Backup";
 47:             totalMSTBackup += cost;
 48:         } else if (edge.isBuilt) {
 49:             role = "MST";
 50:             totalMST += cost;
 51:             totalMSTBackup += cost;
 52:         }
 53: 
 54:         std::string mediaStr = EnumUtils::toString(edge.mediaType);
 55:         
 56:         std::string uv = std::to_string(edge.u) + "-" + std::to_string(edge.v);
 57: 
 58:         std::cout << std::left 
 59:                   << std::setw(8)  << edge.id 
 60:                   << std::setw(8)  << uv 
 61:                   << std::setw(10) << mediaStr 
 62:                   << std::setw(15) << role 
 63:                   << std::fixed << std::setprecision(2) << std::setw(15) << cost 
 64:                   << "\n";
 65:     }
 66: 
 67:     std::cout << "==============================================================\n";
 68:     std::cout << "TONG KET CHI PHI:\n";
 69:     std::cout << "1. Phuong an Xay tat ca cap (Build All)  : " << std::fixed << std::setprecision(2) << totalBuildAll << " DV\n";
 70:     std::cout << "2. Phuong an Toi gian (Chi xay MST)      : " << totalMST << " DV\n";
 71:     std::cout << "3. Phuong an An toan (MST + Backup)      : " << totalMSTBackup << " DV\n";
 72:     std::cout << "==============================================================\n";
 73: }
 74: 
 75: void printFullRoutingTable(const Graph& graph, const WeightFn& weight) {
 76:     std::cout << "\n========================================================================\n";
 77:     std::cout << "          BANG DINH TUYEN TOAN BO MANG LUOI (ROUTING TABLES)            \n";
 78:     std::cout << "========================================================================\n";
 79: 
 80:     std::vector<int> allNodes = graph.nodeIds();
 81: 
 82:     for (int src : allNodes) {
 83:         std::cout << "\n[ ROUTING TABLE CHO ROUTER / NODE " << src << " ]\n";
 84:         std::cout << std::left 
 85:                   << std::setw(12) << "Destination" 
 86:                   << std::setw(12) << "Next Hop" 
 87:                   << std::setw(15) << "Metric (Cost)" 
 88:                   << std::setw(10) << "Hops" 
 89:                   << "Path\n";
 90:         std::cout << "------------------------------------------------------------------------\n";
 91: 
 92:         for (int dest : allNodes) {
 93:             if (src == dest) continue; // Không cần tự định tuyến về chính mình
 94: 
 95:             // Tìm đường đi trên hạ tầng ĐÃ XÂY (onlyBuilt = true)
 96:             PathResult res = dijkstra(graph, src, dest, weight, /*onlyBuilt=*/true);
 97: 
 98:             if (!res.reachable) {
 99:                 std::cout << std::left 
100:                           << std::setw(12) << dest 
101:                           << std::setw(12) << "UNREACHABLE" 
102:                           << std::setw(15) << "INF" 
103:                           << std::setw(10) << "-" 
104:                           << "Khong co ket noi\n";
105:             } else {
106:                 // Next hop chính là node thứ 2 trên đường đi (res.nodes[1])
107:                 // Nếu res.nodes.size() < 2 thì có lỗi gì đó, an toàn thì check > 1
108:                 int nextHop = res.nodes.size() > 1 ? res.nodes[1] : src;
109: 
110:                 // Ghép chuỗi lộ trình (VD: 0 -> 1 -> 2)
111:                 std::string pathStr = "";
112:                 for (size_t i = 0; i < res.nodes.size(); ++i) {
113:                     pathStr += std::to_string(res.nodes[i]);
114:                     if (i + 1 < res.nodes.size()) pathStr += " -> ";
115:                 }
116: 
117:                 std::cout << std::left 
118:                           << std::setw(12) << dest 
119:                           << std::setw(12) << nextHop 
120:                           << std::fixed << std::setprecision(2) << std::setw(15) << res.totalCost 
121:                           << std::setw(10) << res.hopCount 
122:                           << pathStr << "\n";
123:             }
124:         }
125:     }
126:     std::cout << "========================================================================\n";
127: }
128: 
129: void printPacketRouteReport(const Graph& graph, const std::vector<Packet>& packets, const WeightFn& weight) {
130:     std::cout << "\n==============================================================\n";
131:     std::cout << "       BAO CAO DINH TUYEN + DO TRE GOI TIN (DIJKSTRA)          \n";
132:     std::cout << "==============================================================\n";
133: 
134:     for (const Packet& pkt : packets) {
135:         std::cout << "\nPacket #" << pkt.id << ": " << pkt.source << " -> " << pkt.destination
136:                   << "  (" << pkt.sizeBytes << " bytes)\n";
137: 
138:         if (!graph.hasNode(pkt.source) || !graph.hasNode(pkt.destination)) {
139:             std::cout << "  => Khong hop le: node nguon hoac dich khong ton tai.\n";
140:             continue;
141:         }
142: 
143:         // Buoc 1: dung Dijkstra de tim duong di.
144:         // onlyBuilt = true: goi tin CHI duoc di tren day da thuc su duoc
145:         // Kruskal chon xay (MST + Backup)
146:         PathResult path = dijkstra(graph, pkt.source, pkt.destination, weight, /*onlyBuilt=*/true);
147:         if (!path.reachable) {
148:             std::cout << "  => KHONG CO DUONG DI (mang bi chia cat)\n";
149:             continue;
150:         }
151: 
152:         std::cout << "  Duong di (" << path.hopCount << " chang): ";
153:         for (std::size_t i = 0; i < path.nodes.size(); ++i) {
154:             std::cout << path.nodes[i];
155:             if (i + 1 < path.nodes.size()) std::cout << " -> ";
156:         }
157:         std::cout << "\n";
158: 
159:         // Buoc 2: di doc duong do, kiem tra MTU, phan manh va cong don do tre tung chang
160:         double totalDelay = 0.0;
161:         bool dropped = false;
162:         std::string dropReason = "";
163:         int maxFragments = 1;
164:         int cur = pkt.source;
165:         for (int edgeId : path.edges) {
166:             const Edge& e = graph.getEdge(edgeId);
167:             int nextId = graph.otherEndpoint(cur, edgeId);
168:             const Node& nextNode = graph.getNode(nextId);
169: 
170:             int fragments = DelayModel::calcFragmentCount(pkt.sizeBytes, e.mtu, pkt.canFragment);
171:             if (fragments == 0) {
172:                 dropped = true;
173:                 dropReason = "DF_BLOCKED_BY_MTU_AT_EDGE_" + std::to_string(edgeId);
174:                 break;
175:             }
176:             maxFragments = std::max(maxFragments, fragments);
177: 
178:             double d = DelayModel::calculateHopDelay(pkt, e, nextNode);
179:             if (d == Config::INF) {
180:                 dropped = true;
181:                 dropReason = "LINK_DOWN_OR_UNREACHABLE";
182:                 break;
183:             }
184:             totalDelay += d;
185:             cur = nextId;
186:         }
187: 
188:         if (pkt.protocol == Protocol::TCP) {
189:             totalDelay += DelayModel::calcProtocolOverhead(pkt, totalDelay);
190:         }
191: 
192:         std::cout << "  So manh Fragment     : " << maxFragments << " manh\n";
193:         if (dropped) {
194:             std::cout << "  Trang thai Packet    : DROP (" << dropReason << ")\n";
195:         } else {
196:             std::cout << "  Trang thai Packet    : SUCCESS\n";
197:             std::cout << "  Tong do tre uoc tinh : " << std::fixed << std::setprecision(6)
198:                       << totalDelay << " s\n";
199:         }
200:     }
201:     std::cout << "==============================================================\n";
202: }
203: 
204: static std::string formatPathString(const PathResult& path) {
205:     if (!path.reachable || path.nodes.empty()) return "KHONG CO DUONG (UNREACHABLE)";
206:     std::string s = "";
207:     for (size_t i = 0; i < path.nodes.size(); ++i) {
208:         s += std::to_string(path.nodes[i]);
209:         if (i + 1 < path.nodes.size()) s += " -> ";
210:     }
211:     return s;
212: }
213: 
214: void printFailureRecoveryReport(const Graph& graph,
215:                                 const Packet& pkt,
216:                                 const PathResult& oldPath,
217:                                 int failedEdgeId,
218:                                 const PathResult& newPath,
219:                                 const std::string& note) {
220:     std::cout << "\n==============================================================\n";
221:     std::cout << "           BAO CAO SU CO & DINH TUYEN LAI (TASK B5.3)         \n";
222:     std::cout << "==============================================================\n";
223:     std::cout << "Packet ID    : #" << pkt.id << " (" << pkt.source << " -> " << pkt.destination << ")\n";
224:     
225:     // OLD PATH
226:     std::cout << "OLD PATH     : " << formatPathString(oldPath);
227:     if (oldPath.reachable) {
228:         std::cout << " (Hops: " << oldPath.hopCount << ", Cost: " << std::fixed << std::setprecision(6) << oldPath.totalCost << ")";
229:     }
230:     std::cout << "\n";
231: 
232:     // FAILED EDGE
233:     std::cout << "FAILED EDGE  : ";
234:     if (graph.hasEdge(failedEdgeId)) {
235:         const Edge& e = graph.getEdge(failedEdgeId);
236:         std::cout << "Edge #" << failedEdgeId << " (" << e.u << " - " << e.v << ") bi LINK_DOWN\n";
237:     } else {
238:         std::cout << "Edge #" << failedEdgeId << " (Unknown edge)\n";
239:     }
240: 
241:     // NEW PATH
242:     std::cout << "NEW PATH     : " << formatPathString(newPath);
243:     if (newPath.reachable) {
244:         std::cout << " (Hops: " << newPath.hopCount << ", Cost: " << std::fixed << std::setprecision(6) << newPath.totalCost << ")\n";
245:         std::cout << "KET QUA      : [THANH CONG] Da tim duong thay the thanh cong.\n";
246:     } else {
247:         std::cout << "\nKET QUA      : [DROP] Khong the dinh tuyen den dich (Bi co lap / Mat lien lac hoan toan).\n";
248:     }
249: 
250:     if (!note.empty()) {
251:         std::cout << "GHI CHU      : " << note << "\n";
252:     }
253:     std::cout << "==============================================================\n";
254: }
255: 
256: } // namespace Reporter

```

### Giải thích chi tiết từng dòng code:

#### 1. Báo cáo quy hoạch cáp `printMSTReport` (Dòng 12-73 trong .cpp):
* Nhận vào `const Graph& graph` và `const CostModel& costModel`.
* Duyệt qua danh sách cạnh, phân loại vai trò (`Role`):
  - `MST`: Cáp thuộc cây khung nhỏ nhất.
  - `Backup`: Cáp được chọn làm tuyến dự phòng chiến lược.
  - `None`: Cáp tiềm năng khảo sát trên giấy nhưng không được chọn xây dựng.
* In bảng chi tiết từng cạnh: ID, cặp nút $u-v$, loại môi trường (`EnumUtils::toString`), vai trò và tổng chi phí.
* **So sánh kinh tế:** In tổng chi phí phương án xây dựng toàn bộ mạng (Build All) so với phương án chỉ xây MST + Backup, chứng minh khoản ngân sách khổng lồ tiết kiệm được nhờ thuật toán Kruskal.

#### 2. In bảng định tuyến toàn mạng `printFullRoutingTable` (Dòng 75-127):
* Chạy thuật toán Dijkstra cho **tất cả mọi cặp đỉnh $(u, v)$** trong mạng trên hạ tầng đã xây dựng (`onlyBuilt = true`).
* Bảng định tuyến xuất ra gồm: Đích đến (Destination), Chặng kế tiếp (Next Hop), Số chặng (Hop Count), Chi phí tích lũy (Total Cost), và Chi tiết đường đi (Full Path).

#### 3. Báo cáo sự cố và khôi phục định tuyến `printFailureRecoveryReport` (Dòng 213-255):
* Định dạng chuẩn nghiệp vụ 3 giai đoạn:
  1. `OLD PATH`: Lộ trình ban đầu trước sự cố.
  2. `FAILED EDGE`: Cạnh bị đứt cáp (`LINK_DOWN`).
  3. `NEW PATH`: Tuyến đường vòng mới được Dijkstra tìm ra thay thế thành công, hoặc thông báo `[DROP]` nếu điểm đến bị ngắt kết nối hoàn toàn.
  4. `GHI CHU`: Diễn giải nguyên nhân kỹ thuật.

---

## 7.2. File `main.cpp`

### Toàn bộ mã nguồn (`main.cpp`):
```cpp
  1: #include <iostream>
  2: #include <vector>
  3: #include <string>
  4: #include "Graph.h"
  5: #include "IOManager.h"
  6: #include "Kruskal.h"
  7: #include "CostModel.h"
  8: #include "Reporter.h"
  9: #include "Simulator.h"
 10: #include "Scenario.h"
 11: 
 12: void displayMenu() {
 13:     std::cout << "\n==============================================================\n";
 14:     std::cout << "        DO AN PBL2: THIET KE & MO PHONG MANG VIEN THONG       \n";
 15:     std::cout << "                 MENU SCENARIO (TASK B6.2)                    \n";
 16:     std::cout << "==============================================================\n";
 17:     std::cout << "  [1] Scenario 1: BASIC       (Dieu kien mang ly tuong)\n";
 18:     std::cout << "  [2] Scenario 2: QoS         (Uu tien dich vu REAL_TIME vs BULK)\n";
 19:     std::cout << "  [3] Scenario 3: CONGESTION  (Tac nghen mang & tu dong ne duong)\n";
 20:     std::cout << "  [4] Scenario 4: MTU         (Kiem tra phan manh Fragmentation)\n";
 21:     std::cout << "  [5] Scenario 5: FAILURE     (Su co dut cap & tu dong reroute)\n";
 22:     std::cout << "  [6] Scenario 6: REDUNDANCY  (Phan tich vai tro cap Backup)\n";
 23:     std::cout << "  ------------------------------------------------------------\n";
 24:     std::cout << "  [7] Xem Bao cao Quy hoach Cap (MST & Backup Links)\n";
 25:     std::cout << "  [8] Xem Bang dinh tuyen toan mang (Full Routing Table)\n";
 26:     std::cout << "  [0] Thoat chuong trinh\n";
 27:     std::cout << "==============================================================\n";
 28:     std::cout << "Vui long chon thao tac [0-8]: ";
 29: }
 30: 
 31: void runSelectedScenario(Graph& baseGraph, const std::vector<Packet>& packets, const std::string& scenarioFile, const std::string& logFileName = "") {
 32:     // Luôn copy đồ thị để mỗi lần chạy một kịch bản, mạng được bắt đầu ở trạng thái sạch ban đầu
 33:     Graph testGraph = baseGraph;
 34: 
 35:     Scenario sc;
 36:     if (!IOManager::loadScenario(scenarioFile, sc)) {
 37:         std::cerr << "Loi: Khong the doc file kich ban " << scenarioFile << "\n";
 38:         return;
 39:     }
 40: 
 41:     std::string logPath = "";
 42:     if (!logFileName.empty()) {
 43:         logPath = "data/logs/" + logFileName;
 44:     }
 45: 
 46:     Simulator::runScenarioSimulation(testGraph, packets, sc, logPath);
 47: }
 48: 
 49: int main() {
 50:     // 1. NẠP DỮ LIỆU CỐT LÕI VÀ KIỂM TRA HỢP LỆ
 51:     Graph baseGraph;
 52:     if (!IOManager::loadNodes("data/nodes.txt", baseGraph) ||
 53:         !IOManager::loadEdges("data/edges.txt", baseGraph)) {
 54:         std::cerr << "Loi: Khong the khoi tao do thi ban dau!\n";
 55:         return 1;
 56:     }
 57: 
 58:     std::vector<Packet> packets;
 59:     IOManager::loadPackets("data/packets.txt", packets, &baseGraph);
 60: 
 61:     // 2. NẠP BẢNG GIÁ VÀ XÂY DỰNG MST + BACKUP
 62:     CostModel pricingModel;
 63:     pricingModel.loadPricing("data/pricing.txt");
 64:     auto costFn = [&pricingModel](const Edge& e) { return pricingModel.calculateCablingCost(e).total; };
 65:     
 66:     MSTResult mstResult = buildMST(baseGraph, costFn);
 67:     selectBackupLinks(baseGraph, mstResult, 2, costFn);
 68: 
 69:     std::cout << "=> He thong mang da san sang: " << baseGraph.V() << " nodes, " 
 70:               << baseGraph.E() << " edges, " << packets.size() << " packets.\n";
 71: 
 72:     // 3. VÒNG LẶP MENU TƯƠNG TÁC (TASK B6.2)
 73:     std::string inputLine;
 74:     while (true) {
 75:         displayMenu();
 76:         if (!std::getline(std::cin, inputLine)) {
 77:             std::cout << "\n[EOF] Ket thuc chuong trinh.\n";
 78:             break;
 79:         }
 80: 
 81:         // Loại bỏ khoảng trắng và \r thừa nếu có (đặc biệt khi pipe trên Windows)
 82:         while (!inputLine.empty() && (inputLine.back() == '\r' || inputLine.back() == ' ' || inputLine.back() == '\t')) {
 83:             inputLine.pop_back();
 84:         }
 85:         while (!inputLine.empty() && (inputLine.front() == ' ' || inputLine.front() == '\t')) {
 86:             inputLine.erase(inputLine.begin());
 87:         }
 88: 
 89:         if (inputLine.empty()) continue;
 90: 
 91:         int choice = -1;
 92:         try {
 93:             choice = std::stoi(inputLine);
 94:         } catch (...) {
 95:             choice = -1;
 96:         }
 97: 
 98:         if (choice == 0) {
 99:             std::cout << "\nCam on ban da su dung chuong trinh! Tam biet!\n";
100:             break;
101:         }
102: 
103:         switch (choice) {
104:             case 1:
105:                 runSelectedScenario(baseGraph, packets, "data/scenarios/basic.txt", "log_basic.txt");
106:                 break;
107:             case 2:
108:                 runSelectedScenario(baseGraph, packets, "data/scenarios/qos.txt", "log_qos.txt");
109:                 break;
110:             case 3:
111:                 runSelectedScenario(baseGraph, packets, "data/scenarios/congestion.txt", "log_congestion.txt");
112:                 break;
113:             case 4:
114:                 runSelectedScenario(baseGraph, packets, "data/scenarios/mtu.txt", "log_mtu.txt");
115:                 break;
116:             case 5:
117:                 runSelectedScenario(baseGraph, packets, "data/scenarios/failure.txt", "log_failure.txt");
118:                 break;
119:             case 6:
120:                 runSelectedScenario(baseGraph, packets, "data/scenarios/redundancy.txt", "log_redundancy.txt");
121:                 break;
122:             case 7:
123:                 Reporter::printMSTReport(baseGraph, pricingModel);
124:                 break;
125:             case 8:
126:                 Reporter::printFullRoutingTable(baseGraph);
127:                 break;
128:             default:
129:                 std::cout << "Lua chon khong hop le! Vui long nhap so tu 0 den 8.\n";
130:                 break;
131:         }
132: 
133:         std::cout << "\nNhan Enter de tiep tuc...";
134:         std::string pauseLine;
135:         std::getline(std::cin, pauseLine);
136:     }
137: 
138:     return 0;
139: }

```

### Giải thích chi tiết từng dòng code:

#### 1. Nguyên tắc kiến trúc:
* `main.cpp` tuân thủ triệt để nguyên lý **Single Responsibility Principle (SRP)**. File này không chứa bất kỳ vòng lặp thuật toán Dijkstra, Kruskal hay công thức vật lý nào. Nó chỉ đóng vai trò là "Nhạc trưởng" (Orchestrator) kết nối và điều phối các module độc lập.

#### 2. Hàm hỗ trợ `runSelectedScenario` (Dòng 31-48):
* **Tính độc lập của kịch bản (Dòng 33):**
  `Graph testGraph = baseGraph;`
  **Điểm sáng kỹ thuật:** Mỗi khi chạy một kịch bản, hệ thống luôn copy đồ thị gốc sạch `baseGraph` ra một bản sao tạm `testGraph`. Điều này đảm bảo các kịch bản chạy hoàn toàn độc lập, không bị "lưu vết" trạng thái đứt cáp hay nghẽn mạng của kịch bản trước đó.
* Nạp kịch bản từ `data/scenarios/<file>` và gọi `Simulator::runScenarioSimulation()`.

#### 3. Hàm `main()` (Dòng 50-139):
* **Bước 1 - Nạp dữ liệu cốt lõi (Dòng 51-60):**
  - Khởi tạo `Graph baseGraph`.
  - Nạp danh sách nút từ `data/nodes.txt` và cạnh từ `data/edges.txt`. Nếu lỗi file, in thông báo và dừng chương trình (`return 1`).
  - Nạp danh sách gói tin từ `data/packets.txt` có truyền `&baseGraph` để kiểm duyệt nguồn/đích.
* **Bước 2 - Quy hoạch hạ tầng cáp (Dòng 62-72):**
  - Nạp bảng giá từ `data/pricing.txt` vào `CostModel pricingModel`.
  - Tạo lambda `costFn`.
  - Chạy `buildMST(baseGraph, costFn)` tìm cây khung nhỏ nhất.
  - Chạy `selectBackupLinks(baseGraph, mstResult, 2, costFn)` chọn 2 sợi cáp dự phòng.
  - In thông báo hệ thống đã sẵn sàng với số node, số cạnh và số gói tin.
* **Bước 3 - Vòng lặp Menu tương tác (Dòng 74-136):**
  - Gọi `displayMenu()` in menu 9 lựa chọn `[0-8]`.
  - Đọc chuỗi nhập an toàn qua `std::getline(std::cin, inputLine)`, loại bỏ ký tự khoảng trắng và `\r` trên Windows.
  - Dùng `try-catch` bọc hàm `std::stoi()` để chương trình không bao giờ bị crash nếu người dùng vô tình gõ chữ cái thay vì số.
  - Sử dụng cấu trúc `switch-case`:
    - Case 1: Chạy kịch bản điều kiện lý tưởng (Basic).
    - Case 2: Chạy kịch bản ưu tiên dịch vụ (QoS).
    - Case 3: Chạy kịch bản tắc nghẽn mạng (Congestion).
    - Case 4: Chạy kịch bản kiểm tra phân mảnh MTU.
    - Case 5: Chạy kịch bản sự cố đứt cáp và reroute (Failure).
    - Case 6: Chạy kịch bản kiểm chứng vai trò cáp dự phòng (Redundancy).
    - Case 7: Xem báo cáo quy hoạch cáp (MST Report).
    - Case 8: Xem bảng định tuyến toàn mạng (Full Routing Table).
    - Case 0: Thoát chương trình.

---

# PHỤ LỤC: BỘ CÂU HỎI VÀ ĐÁP ÁN VẤN ĐÁP BẢO VỆ ĐỒ ÁN PBL2 (FAQ)

### Câu 1: Tại sao trong lớp `Graph`, danh sách kề `adjacency` lại lưu `edgeId` mà không lưu `nodeId` kề?
**Trả lời:** Trong mạng viễn thông thực tế, giữa hai nút mạng có thể tồn tại **nhiều đường cáp song song (Parallel Links/Multi-edges)**, ví dụ vừa có cáp quang vừa có cáp đồng giữa Router 1 và Router 2. Nếu danh sách kề chỉ lưu `nodeId`, ta không thể phân biệt được gói tin đang truyền trên sợi cáp quang (tốc độ cao, MTU lớn) hay cáp đồng (băng thông thấp). Lưu `edgeId` cho phép truy xuất chính xác $100\%$ thuộc tính vật lý của từng sợi cáp độc lập.

### Câu 2: Thuật toán Kruskal hoạt động như thế nào và tại sao cần cấu trúc DSU?
**Trả lời:** Thuật toán Kruskal tìm cây khung nhỏ nhất theo chiến lược tham lam (Greedy): Sắp xếp toàn bộ các cạnh theo chi phí tăng dần, sau đó lần lượt chọn từng cạnh rẻ nhất đưa vào cây nếu cạnh đó không tạo thành chu trình.
- Cấu trúc DSU (Disjoint Set Union) giúp kiểm tra hai đỉnh $u, v$ đã thuộc cùng một thành phần liên thông hay chưa trong thời gian gần như hằng số $\mathcal{O}(\alpha(V))$.
- Nhờ hai kỹ thuật tối ưu cốt lõi: **Nén đường đi (Path Compression)** (làm phẳng cây khi tìm gốc) và **Gộp theo Rank (Union by Rank)** (luôn gắn cây thấp vào cây cao hơn), DSU ngăn chặn hiện tượng cây suy biến thành danh sách liên kết.

### Câu 3: Thuật toán Dijkstra trong đồ án được cài đặt tối ưu như thế nào?
**Trả lời:**
- Thuật toán sử dụng **Min-Heap** thông qua `std::priority_queue<State, std::vector<State>, std::greater<State>>` với độ phức tạp $\mathcal{O}((V + E) \log V)$.
- Có cơ chế loại bỏ trạng thái lỗi thời: `if (cur.dist > dist[cur.node]) continue;` để không xử lý lại các đỉnh đã được tối ưu trước đó.
- Nhận hàm trọng số động `RoutingCost::makeWeightFn(pkt)`, cho phép tìm đường tối ưu thích ứng linh hoạt theo loại gói tin QoS và mức tải động của mạng.
- Nhận tham số `onlyBuilt = true`, buộc gói tin chỉ đi trên hạ tầng cáp đã được duyệt bỏ tiền xây dựng (MST + Backup).

### Câu 4: Phân biệt sự khác nhau giữa chi phí định tuyến `RoutingCost` và độ trễ thực tế `DelayModel`?
**Trả lời:**
- `DelayModel` tính toán **độ trễ vật lý thực tế** của gói tin khi di chuyển qua dây cáp và router (gồm 4 thành phần trễ: truyền dẫn, lan truyền, xử lý CPU router, và hàng đợi buffer + TCP handshake). Kết quả trả về bằng giây ($s$) để đánh giá hiệu năng mạng.
- `RoutingCost` là **hàm mục tiêu toán học** dùng riêng cho Dijkstra so sánh đường đi. Hàm này nhân thêm các **trọng số QoS** (ưu tiên gói REAL_TIME né hàng đợi, BULK_DATA ưu tiên băng thông), cộng thêm hệ số phạt tắc nghẽn tiệm cận hàm mũ và điểm phạt phân mảnh MTU.

### Câu 5: Tại sao phải quy hoạch thêm Cáp dự phòng (Backup Links)? Nếu chỉ xây cây khung MST thì có được không?
**Trả lời:**
- Cây khung MST là đồ thị liên thông tối thiểu chỉ có đúng $V - 1$ cạnh và không có chu trình. Do đó, **mọi cạnh trên cây MST đều là cạnh cầu độc đạo (Critical Bridge)**.
- Nếu chỉ xây MST, chỉ cần **1 sợi cáp bất kỳ bị đứt** (do đào đường, chuột cắn, hỏng module), toàn bộ mạng lập tức bị xé toạc làm đôi, lưu lượng giữa 2 nửa mạng sẽ bị **DROP $100\%$**.
- Do đó, việc quy hoạch thêm $K$ tuyến cáp dự phòng chiến lược (Backup Links) giúp tạo thành các chu trình khép kín. Khi cáp chính đứt, Dijkstra lập tức bẻ lái dòng dữ liệu sang cáp Backup, giữ cho mạng luôn thông suốt.

### Câu 6: Cơ chế phân tích cáp dự phòng khi đứt cáp cây khung trong `Reliability` hoạt động như thế nào?
**Trả lời:**
- Khi sợi cáp cây khung `failedTreeEdgeId` bị đứt, thuật toán dùng BFS duyệt qua các cạnh MST còn lại để tách mạng thành đúng 2 nửa: Nhánh A chứa đầu mút $u$ và Nhánh B chứa đầu mút $v$.
- Sau đó, thuật toán quét toàn bộ các cạnh ngoài cây còn hoạt động để tìm các sợi cáp bắc cầu giữa Nhánh A và Nhánh B.
- Nếu danh sách ứng viên rỗng $\to$ Cảnh báo đó là **Cạnh cầu nguy cấp (Critical Bridge)**, không có đường thay thế. Nếu có nhiều ứng viên, ưu tiên chọn cáp đã xây sẵn trong thực tế (`isBuilt = true`) và có chi phí rẻ nhất.

### Câu 7: Giải thích nguyên lý tính thời gian bắt tay TCP (Handshake Overhead)?
**Trả lời:**
- Kết nối TCP là kết nối đầu-cuối (End-to-End) giữa máy nguồn và máy đích, các router trung gian chỉ chuyển tiếp gói IP chứ không tham gia bắt tay.
- Khi mở kết nối mới (`newConnection = true`), gói tin tốn thêm đúng 1 RTT (Round Trip Time = $2 \times \text{One-Way Delay}$).
- Khoản trễ này được cộng **đúng 1 lần duy nhất** cho toàn bộ hành trình sau khi đã tính tổng trễ của tất cả các chặng, tuyệt đối không cộng lặp lại ở từng chặng riêng lẻ.## 3.2. File include/CostModel.h & src/CostModel.cpp

### Toàn bộ mã nguồn Header (include/CostModel.h):
`cpp
  1: #ifndef COSTMODEL_H
  2: #define COSTMODEL_H
  3: 
  4: #include "Edge.h"
  5: 
  6: #include <map>
  7: #include <string>
  8: 
  9: struct CostBreakdown {
 10:     double install;     
 11:     double material;      // Chi phí vật tư (chiều dài * đơn giá * hệ số địa hình)
 12:     double equipment;     
 13:     double maintenance;   
 14:     double total;         
 15: };
 16: 
 17: // Cấu trúc lưu trạng thái bảng giá nạp từ pricing.txt (khởi tạo rỗng đồng bộ)
 18: struct PricingConfig {
 19:     double installationCost = 0.0;
 20:     std::map<MediaType, double> unitPrice;
 21:     std::map<MediaType, double> equipmentCost;
 22:     std::map<MediaType, double> maintenanceCost;
 23: };
 24: 
 25: class CostModel {
 26: private:
 27:     PricingConfig config; // Thuộc tính private: Đảm bảo đóng gói tuyệt đối
 28: 
 29: public:
 30:     // Constructor mặc định (sẽ dùng cấu hình mặc định trong PricingConfig)
 31:     CostModel() = default;
 32: 
 33:     // Nạp cấu hình từ file text
 34:     bool loadPricing(const std::string& filepath);
 35: 
 36:     // Tính toán chi phí - Hàm const vì nó không làm thay đổi trạng thái của model
 37:     CostBreakdown calculateCablingCost(const Edge& edge) const;
 38: };
 39: 
 40: #endif 

`

### Toàn bộ mã nguồn Cài đặt (src/CostModel.cpp):
`cpp
  1: #include "CostModel.h"
  2: #include "Config.h"
  3: #include "EnumUtils.h"
  4: #include <fstream>
  5: #include <sstream>
  6: #include <iostream>
  7: #include <stdexcept>
  8: 
  9: bool CostModel::loadPricing(const std::string& filepath) {
 10:     std::ifstream file(filepath);
 11:     if (!file.is_open()) {
 12:         std::cerr << "[CostModel] Canh bao: Khong the mo " << filepath << ". Su dung thong so mac dinh tu danh sach canh.\n";
 13:         return false;
 14:     }
 15: 
 16:     std::string line;
 17:     int lineCount = 0;
 18:     while (std::getline(file, line)) {
 19:         lineCount++;
 20:         if (line.empty() || line[0] == '#') {
 21:             continue;
 22:         }
 23: 
 24:         std::stringstream ss(line);
 25:         std::string key;
 26:         ss >> key;
 27: 
 28:         if (key == "INSTALLATION_COST") {
 29:             if (!(ss >> config.installationCost)) {
 30:                 std::cerr << "[CostModel] Canh bao dong " << lineCount << ": Sai format INSTALLATION_COST trong " << filepath << ".\n";
 31:             }
 32:         } 
 33:         else if (key == "UNIT_PRICE" || key == "EQUIPMENT_COST" || key == "MAINTENANCE_COST") {
 34:             std::string typeStr;
 35:             double val;
 36:             if (ss >> typeStr >> val) {
 37:                 MediaType media;
 38:                 if (!EnumUtils::parseMediaType(typeStr, media)) {
 39:                     std::cerr << "[CostModel] Canh bao dong " << lineCount 
 40:                               << ": Loai cap khong hop le '" << typeStr 
 41:                               << "' trong " << filepath << ". Bo qua dong nay.\n";
 42:                     continue;
 43:                 }
 44:                 if (key == "UNIT_PRICE") {
 45:                     config.unitPrice[media] = val;
 46:                 } else if (key == "EQUIPMENT_COST") {
 47:                     config.equipmentCost[media] = val;
 48:                 } else {
 49:                     config.maintenanceCost[media] = val;
 50:                 }
 51:             } else {
 52:                 std::cerr << "[CostModel] Canh bao dong " << lineCount 
 53:                           << ": Sai format " << key << " trong " << filepath << ". Bo qua dong nay.\n";
 54:             }
 55:         }
 56:     }
 57:     std::cout << "[CostModel] Nhan cau hinh gia thanh cong tu: " << filepath << "\n";
 58:     return true;
 59: }
 60: 
 61: CostBreakdown CostModel::calculateCablingCost(const Edge& edge) const {
 62:     CostBreakdown breakdown;
 63:     
 64:     // Ràng buộc: "dùng INF cho cạnh không đi được"
 65:     if (!edge.isUp) {
 66:         breakdown.install = breakdown.material = breakdown.equipment = breakdown.maintenance = 0.0;
 67:         breakdown.total = Config::INF;
 68:         return breakdown;
 69:     }
 70: 
 71:     // 1. Chi phí lắp đặt cơ bản
 72:     breakdown.install = config.installationCost;
 73:     
 74:     // 2. Tính chi phí vật tư: Tra cứu trong map dựa vào loại cáp.
 75:     // Fallback: Nếu không có trong map thì dùng edge.unitPrice
 76:     double currentUnitPrice = config.unitPrice.count(edge.mediaType) 
 77:                               ? config.unitPrice.at(edge.mediaType) 
 78:                               : edge.unitPrice;
 79:     breakdown.material = edge.length * currentUnitPrice * edge.terrainFactor;
 80:     
 81:     // 3. Phí thiết bị
 82:     double currentEquipCost = config.equipmentCost.count(edge.mediaType)
 83:                               ? config.equipmentCost.at(edge.mediaType)
 84:                               : edge.equipmentCost;
 85:     breakdown.equipment = currentEquipCost;
 86:     
 87:     // 4. Phí bảo trì
 88:     double currentMaintCost = config.maintenanceCost.count(edge.mediaType)
 89:                               ? config.maintenanceCost.at(edge.mediaType)
 90:                               : edge.maintenanceCost;
 91:     breakdown.maintenance = currentMaintCost;
 92:     
 93:     // 5. Tổng tất cả các chi phí
 94:     breakdown.total = breakdown.install + breakdown.material + breakdown.equipment + breakdown.maintenance;
 95:     
 96:     return breakdown;
 97: }

`

### Giải thích chi tiết từng dòng code:

#### 1. Cấu trúc bảng giá PricingConfig & CostBreakdown:
* CostBreakdown (include/CostModel.h dòng 9-15): Cung cấp chi tiết 4 thành phần chi phí của một đường cáp:
  1. install: Phí nhân công thi công cơ bản.
  2. material: Tiền mua cáp: $\text{chiều dài} \times \text{đơn giá cáp/mét} \times \text{hệ số địa hình}$.
  3. equipment: Phí trang thiết bị đầu cuối (module quang, card chuyển đổi).
  4. maintenance: Phí bảo trì định kỳ.
  5. 	otal: Tổng chi phí lắp đặt =  + material + equipment + maintenance$.
* PricingConfig (include/CostModel.h dòng 18-24): Khởi tạo rỗng đồng bộ theo đúng nguyên lý của toàn bộ hệ thống (khi chưa nạp file pricing.txt thì các bảng giá rỗng). Hệ thống sẽ sử dụng các thông số kỹ thuật mặc định có sẵn của từng sợi cáp trong edges.txt, không cần hardcode bất kỳ con số ngầm nào trong Header.

#### 2. Các phương thức trong src/CostModel.cpp:
* **Dòng 9-57 (loadPricing):**
  - Mở file data/pricing.txt.
  - Quét từng dòng, nhận diện các từ khóa: INSTALLATION_COST, UNIT_PRICE, EQUIPMENT_COST, MAINTENANCE_COST.
  - **Cơ chế phòng thủ:** Sử dụng trực tiếp EnumUtils::parseMediaType(typeStr, media). Nếu phát hiện loại cáp lạ (ví dụ gõ nhầm FIBRE), hệ thống in cảnh báo kèm số dòng lineCount và bỏ qua dòng đó, đảm bảo bảng giá không bị ghi đè dữ liệu rác.
* **Dòng 59-95 (calculateCablingCost):**
  - Kiểm tra trạng thái vật lý: if (!edge.isUp) return breakdown với total = Config::INF;. Cáp đang đứt thì không thể vận hành, chi phí coi như vô cực.
  - Tra cứu đơn giá vật tư, phí thiết bị và phí bảo trì theo loại cáp (edge.mediaType). Nếu trong file cấu hình không có (hoặc chưa nạp file pricing) thì tự động lấy giá fallback lưu sẵn trong đối tượng edge.
  - Áp dụng đúng công thức:
    \text{material} = \text{edge.length} \times \text{unitPrice} \times \text{edge.terrainFactor}
    \text{total} = \text{install} + \text{material} + \text{equipment} + \text{maintenance}

---

## 3.2. File include/CostModel.h & src/CostModel.cpp

### Toàn bộ mã nguồn Header (include/CostModel.h):
`cpp
  1: #ifndef COSTMODEL_H
  2: #define COSTMODEL_H
  3: 
  4: #include "Edge.h"
  5: 
  6: #include <map>
  7: #include <string>
  8: 
  9: struct CostBreakdown {
 10:     double install;     
 11:     double material;      // Chi phí vật tư (chiều dài * đơn giá * hệ số địa hình)
 12:     double equipment;     
 13:     double maintenance;   
 14:     double total;         
 15: };
 16: 
 17: // Cấu trúc lưu trạng thái bảng giá nạp từ pricing.txt (khởi tạo rỗng đồng bộ)
 18: struct PricingConfig {
 19:     double installationCost = 0.0;
 20:     std::map<MediaType, double> unitPrice;
 21:     std::map<MediaType, double> equipmentCost;
 22:     std::map<MediaType, double> maintenanceCost;
 23: };
 24: 
 25: class CostModel {
 26: private:
 27:     PricingConfig config; // Thuộc tính private: Đảm bảo đóng gói tuyệt đối
 28: public:
 29:     // Constructor mặc định (sẽ dùng cấu hình mặc định trong PricingConfig)
 30:     CostModel() = default;
 31: 
 32:     // Nạp cấu hình từ file text
 33:     bool loadPricing(const std::string& filepath);
 34: 
 35:     // Tính toán chi phí - Hàm const vì nó không làm thay đổi trạng thái của model
 36:     CostBreakdown calculateCablingCost(const Edge& edge) const;
 37: };
 38: 
 39: #endif 
`

### Toàn bộ mã nguồn Cài đặt (src/CostModel.cpp):
`cpp
  1: #include "CostModel.h"
  2: #include "Config.h"
  3: #include "EnumUtils.h"
  4: #include <fstream>
  5: #include <sstream>
  6: #include <iostream>
  7: #include <stdexcept>
  8: 
  9: bool CostModel::loadPricing(const std::string& filepath) {
 10:     std::ifstream file(filepath);
 11:     if (!file.is_open()) {
 12:         std::cerr << "[CostModel] Canh bao: Khong the mo " << filepath << ". Su dung thong so mac dinh tu danh sach canh.\n";
 13:         return false;
 14:     }
 15: 
 16:     std::string line;
 17:     int lineCount = 0;
 18:     while (std::getline(file, line)) {
 19:         lineCount++;
 20:         if (line.empty() || line[0] == '#') {
 21:             continue;
 22:         }
 23: 
 24:         std::stringstream ss(line);
 25:         std::string key;
 26:         ss >> key;
 27: 
 28:         if (key == "INSTALLATION_COST") {
 29:             if (!(ss >> config.installationCost)) {
 30:                 std::cerr << "[CostModel] Canh bao dong " << lineCount << ": Sai format INSTALLATION_COST trong " << filepath << ".\n";
 31:             }
 32:         } 
 33:         else if (key == "UNIT_PRICE" || key == "EQUIPMENT_COST" || key == "MAINTENANCE_COST") {
 34:             std::string typeStr;
 35:             double val;
 36:             if (ss >> typeStr >> val) {
 37:                 MediaType media;
 38:                 if (!EnumUtils::parseMediaType(typeStr, media)) {
 39:                     std::cerr << "[CostModel] Canh bao dong " << lineCount 
 40:                               << ": Loai cap khong hop le '" << typeStr 
 41:                               << "' trong " << filepath << ". Bo qua dong nay.\n";
 42:                     continue;
 43:                 }
 44:                 if (key == "UNIT_PRICE") {
 45:                     config.unitPrice[media] = val;
 46:                 } else if (key == "EQUIPMENT_COST") {
 47:                     config.equipmentCost[media] = val;
 48:                 } else {
 49:                     config.maintenanceCost[media] = val;
 50:                 }
 51:             } else {
 52:                 std::cerr << "[CostModel] Canh bao dong " << lineCount 
 53:                           << ": Sai format " << key << " trong " << filepath << ". Bo qua dong nay.\n";
 54:             }
 55:         }
 56:     }
 57:     std::cout << "[CostModel] Nhan cau hinh gia thanh cong tu: " << filepath << "\n";
 58:     return true;
 59: }
 60: 
 61: CostBreakdown CostModel::calculateCablingCost(const Edge& edge) const {
 62:     CostBreakdown breakdown;
 63:     
 64:     // Ràng buộc: "dùng INF cho cạnh không đi được"
 65:     if (!edge.isUp) {
 66:         breakdown.install = breakdown.material = breakdown.equipment = breakdown.maintenance = 0.0;
 67:         breakdown.total = Config::INF;
 68:         return breakdown;
 69:     }
 70: 
 71:     // 1. Chi phí lắp đặt cơ bản
 72:     breakdown.install = config.installationCost;
 73:     
 74:     // 2. Tính chi phí vật tư: Tra cứu trong map dựa vào loại cáp.
 75:     // Fallback: Nếu không có trong map thì dùng edge.unitPrice
 76:     double currentUnitPrice = config.unitPrice.count(edge.mediaType) 
 77:                               ? config.unitPrice.at(edge.mediaType) 
 78:                               : edge.unitPrice;
 79:     breakdown.material = edge.length * currentUnitPrice * edge.terrainFactor;
 80:     
 81:     // 3. Phí thiết bị
 82:     double currentEquipCost = config.equipmentCost.count(edge.mediaType)
 83:                               ? config.equipmentCost.at(edge.mediaType)
 84:                               : edge.equipmentCost;
 85:     breakdown.equipment = currentEquipCost;
 86:     
 87:     // 4. Phí bảo trì
 88:     double currentMaintCost = config.maintenanceCost.count(edge.mediaType)
 89:                               ? config.maintenanceCost.at(edge.mediaType)
 90:                               : edge.maintenanceCost;
 91:     breakdown.maintenance = currentMaintCost;
 92:     
 93:     // 5. Tổng tất cả các chi phí
 94:     breakdown.total = breakdown.install + breakdown.material + breakdown.equipment + breakdown.maintenance;
 95:     
 96:     return breakdown;
 97: }
`

### Giải thích chi tiết từng dòng code:

#### 1. Cấu trúc bảng giá PricingConfig & CostBreakdown:
* CostBreakdown (include/CostModel.h dòng 9-15): Cung cấp chi tiết 4 thành phần chi phí của một đường cáp:
  1. install: Phí nhân công thi công cơ bản.
  2. material: Tiền mua cáp: $\text{chiều dài} \times \text{đơn giá cáp/mét} \times \text{hệ số địa hình}$.
  3. equipment: Phí trang thiết bị đầu cuối (module quang, card chuyển đổi).
  4. maintenance: Phí bảo trì định kỳ.
  5. 	otal: Tổng chi phí lắp đặt =  + material + equipment + maintenance$.
* PricingConfig (include/CostModel.h dòng 18-23): Khởi tạo rỗng đồng bộ theo đúng nguyên lý của toàn bộ hệ thống (khi chưa nạp file pricing.txt thì các bảng giá rỗng). Hệ thống sẽ sử dụng các thông số kỹ thuật mặc định có sẵn của từng sợi cáp trong edges.txt, không cần hardcode bất kỳ con số ngầm nào trong Header.

#### 2. Các phương thức trong src/CostModel.cpp:
* **Dòng 9-57 (loadPricing):**
  - Mở file data/pricing.txt.
  - Quét từng dòng, nhận diện các từ khóa: INSTALLATION_COST, UNIT_PRICE, EQUIPMENT_COST, MAINTENANCE_COST.
  - **Cơ chế phòng thủ:** Sử dụng trực tiếp EnumUtils::parseMediaType(typeStr, media). Nếu phát hiện loại cáp lạ (ví dụ gõ nhầm FIBRE), hệ thống in cảnh báo kèm số dòng lineCount và bỏ qua dòng đó, đảm bảo bảng giá không bị ghi đè dữ liệu rác.
* **Dòng 59-95 (calculateCablingCost):**
  - Kiểm tra trạng thái vật lý: if (!edge.isUp) return breakdown với total = Config::INF;. Cáp đang đứt thì không thể vận hành, chi phí coi như vô cực.
  - Tra cứu đơn giá vật tư, phí thiết bị và phí bảo trì theo loại cáp (edge.mediaType). Nếu trong file cấu hình không có (hoặc chưa nạp file pricing) thì tự động lấy giá fallback lưu sẵn trong đối tượng edge.
  - Áp dụng đúng công thức:
    \text{material} = \text{edge.length} \times \text{unitPrice} \times \text{edge.terrainFactor}
    \text{total} = \text{install} + \text{material} + \text{equipment} + \text{maintenance}

---


