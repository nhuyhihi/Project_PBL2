# PBL2

# 2. Cấu trúc thư mục dự án

```text
PBL2/
│
├── .vscode/
│   ├── tasks.json
│   └── launch.json
│
├── .antigravity/
│   └── rules/
│       └── rule.md
│
├── bin/
│   └── main.exe
│
├── include/
│   ├── Config.h
│   ├── Types.h
│   ├── Node.h
│   ├── Edge.h
│   ├── Packet.h
│   ├── PathResult.h
│   ├── Graph.h
│   ├── DSU.h
│   ├── Validation.h
│   ├── CostModel.h
│   ├── PacketRules.h
│   ├── ProtocolModel.h
│   ├── Kruskal.h
│   ├── Dijkstra.h
│   ├── Reliability.h
│   ├── Scenario.h
│   ├── Simulator.h
│   ├── IOManager.h
│   └── Reporter.h
│
├── src/
│   ├── Graph.cpp
│   ├── DSU.cpp
│   ├── Validation.cpp
│   ├── CostModel.cpp
│   ├── PacketRules.cpp
│   ├── ProtocolModel.cpp
│   ├── Kruskal.cpp
│   ├── Dijkstra.cpp
│   ├── Reliability.cpp
│   ├── Simulator.cpp
│   ├── IOManager.cpp
│   └── Reporter.cpp
│
├── data/
│   ├── nodes.txt
│   ├── edges.txt
│   ├── packets.txt
│   └── scenarios/
│       ├── basic.txt
│       ├── congestion.txt
│       ├── failure.txt
│       ├── qos.txt
│       └── mtu.txt
│
├── tests/
│   ├── test_graph/
│   ├── test_kruskal/
│   ├── test_dijkstra/
│   ├── test_cost/
│   ├── test_packet/
│   └── test_simulator/
│
├── docs/
│   ├── architecture/
│   ├── formulas/
│   ├── test-results/
│   └── demo/
│
├── main.cpp
└── README.md
```
## Nguyên tắc 3 – Mỗi task phải có test

```text
Code
  ↓
Test
  ↓
Đối chiếu kết quả
  ↓
Review chéo
  ↓
Merge
```
---
# 6. TUẦN 1 – KHUNG + DATA + GRAPH + VALIDATION

## 🎯 Mục tiêu tuần

- [x] Load được `nodes.txt`
- [x] Load được `edges.txt`
- [x] Load được `packets.txt`
- [x] Validation hoạt động
- [x] Graph + adjacency hoạt động
- [x] Struct lõi và API được chốt
---
# 6.1. CẢ HAI – NGÀY 1–2

## Task 1.1 – Chốt glossary và số liệu mẫu

### Việc cần làm

- [x] Thống nhất nghĩa của `bandwidth`
- [x] Thống nhất `queueDelay`
- [x] Thống nhất `currentLoad`
- [x] Thống nhất `MTU`
- [x] Thống nhất `terrainFactor`
- [x] Thống nhất `processingTime`
- [x] Thống nhất đơn vị nội bộ

```text
size       → byte
bandwidth  → Mbps
length     → m
delay      → second
```

### Kiểm tra

- [x] Hai bạn giải thích giống nhau khi hỏi cùng một thuật ngữ
- [x] Không có file nào dùng đơn vị khác mà không chuyển đổi

---

## Task 1.2 – Vẽ mạng mẫu

### Việc cần làm

- [x] Vẽ 6–8 node
- [x] Ghi tên node
- [x] Ghi các edge
- [x] Ghi cost cáp
- [x] Ghi bandwidth
- [x] Ghi queue
- [x] Ghi MTU
- [x] Tính tay MST
- [x] Tính tay route
### Kiểm tra
- [x] Có đáp án chuẩn để cuối tuần 2–3 đối chiếu
- [x] Có thể giải thích vì sao cạnh được chọn/bị loại
---
## Task 1.3 – Tạo repository
- [x] Tạo Git repository
- [x] Tạo branch `develop`
- [x] Tạo branch riêng cho Quyến
- [x] Tạo branch riêng cho Ý
- [x] Tạo khung thư mục
---

## Task 1.4 – Chốt struct
Cả hai cùng viết và review:
```text
Types.h
Node.h
Edge.h
Packet.h
Config.h
```
### Kiểm tra
- [x] Không duplicate struct
- [x] Cả Quyến/Ý compile cùng API
- [x] Không tự thêm field làm thay đổi contract mà chưa thống nhất
--
# 6.2. 👨‍💻 QUYẾN – TUẦN 1

## Task A1.1 – Xây Graph

### File

```text
include/Graph.h
src/Graph.cpp
```

### Hàm

```cpp
addNode()
addEdge()
getNeighbors()
getEdge()
buildAdjacency()
V()
E()
otherEndpoint()
```

### Việc cần làm

Graph lưu:

```text
nodes
edges
adjacency
```

Adjacency lưu `edgeId`, không chỉ `nodeId`, vì hai node có thể có nhiều edge ứng viên.

### Kiểm tra đúng/sai

- [x] Thêm 3 node → `V() = 3`
- [x] Thêm 2 edge → `E() = 2`
- [x] Mỗi edge xuất hiện ở adjacency của cả hai đầu
- [x] `sum(adj[u].size()) == 2E`
- [x] `getEdge(edgeId)` trả đúng edge
- [x] `otherEndpoint()` trả đúng node còn lại

---

## Task A1.2 – Khung DSU

### File

```text
include/DSU.h
```

### Hàm

```cpp
find()
unite()
```

### Việc cần làm

- [x] Tạo `parent`
- [x] Tạo rank
- [x] Khai báo `find`
- [x] Khai báo `unite`

### Kiểm tra

```text
unite(0,1)
unite(1,2)

find(0) == find(1)
find(1) == find(2)
find(0) == find(2)
```

---

## Task A1.3 – Dataset mẫu và file sai

### Việc cần làm

Tạo data cho:

- [x] Edge node ID không tồn tại
- [x] Bandwidth = 0
- [x] MTU = 0
- [x] Load > 1
- [x] Port âm
- [x] Packet size âm

### Kiểm tra

- [x] Có valid case
- [x] Có invalid case
- [x] Có output mong đợi cho từng invalid case

---

# 6.3. 🕵️ Ý – TUẦN 1

## Task B1.1 – IOManager

### File

```text
include/IOManager.h
src/IOManager.cpp
```

### Hàm

```cpp
loadNodes()
loadEdges()
loadPackets()
isSkippableLine()
```

### Việc cần làm

- [x] Mở TXT
- [x] Bỏ dòng trống
- [x] Bỏ comment `#`
- [x] Tách field
- [x] Convert string → enum
- [x] Tạo Node/Edge/Packet
- [x] Gửi dữ liệu vào Graph

### Kiểm tra

- [x] File hợp lệ đọc được
- [x] Comment bị bỏ qua
- [x] Dòng trống không crash
- [x] Sai format được báo rõ

---

## Task B1.2 – String → Enum

### Hàm

```cpp
stringToNodeType()
stringToMediaType()
stringToProtocol()
stringToPacketType()
stringToEventType()
```

### Kiểm tra

```text
ROUTER → NodeType::ROUTER
FIBER  → MediaType::FIBER
TCP    → Protocol::TCP
```

- [x] Mapping đúng
- [x] Không nhầm media
- [x] Không nhầm protocol

---

## Task B1.3 – Validation

### File

```text
include/Validation.h
src/Validation.cpp
```

### Hàm

```cpp
isValidNode()
isValidEdge()
isValidPacket()
```

### Phải reject

- [x] Node không hợp lệ
- [x] Edge tham chiếu node không tồn tại
- [x] `bandwidthMbps <= 0`
- [x] `mtu <= 0`
- [x] `currentLoad < 0`
- [x] `currentLoad > 1`
- [x] `totalPorts < 0`
- [x] `sizeBytes < 0`

### Kiểm tra

Kết quả phải có:

```text
REJECT + lý do
```

Không chỉ trả `false` mà không biết vì sao khi đang debug/test.

---

# 6.4. ✅ CHECKPOINT CUỐI TUẦN 1

## Quyến

- [x] Graph compile
- [x] Add/get hoạt động
- [x] Adjacency đúng
- [x] DSU API tồn tại
- [x] Dataset mẫu có sẵn

## Ý

- [x] Parser hoạt động
- [x] Validation hoạt động
- [x] String → enum hoạt động
- [x] Valid/invalid data đều test được

## Cả hai

- [x] `main.cpp` đọc 3 file data không crash
- [x] `sum(adj[u].size()) == 2E`
- [x] Dùng chung `Types.h`
- [x] Không duplicate struct

```text
week1-done
```

---

# 7. TUẦN 2 – KRUSKAL + CHI PHÍ CÁP + BACKUP

## 🎯 Mục tiêu tuần

```text
Topology
  ↓
Physical feasibility
  ↓
Cabling Cost
  ↓
Kruskal
  ↓
MST
  ↓
Backup links
```

---

# 7.1. 👨‍💻 QUYẾN – TUẦN 2

## Task A2.1 – Hoàn thiện DSU

### File

```text
include/DSU.h
src/DSU.cpp
```

### Việc cần làm

- [x] Path compression
- [x] Union by rank
- [x] `find`
- [x] `unite`

### Kiểm tra

- [x] Các node cùng component có cùng root
- [x] Unite cùng component trả false
- [x] Không tạo cycle bằng cách unite hai node đã cùng root

---

## Task A2.2 – `canSelectEdge()`

### Edge chỉ được chọn khi

```text
1. Không tạo cycle
2. Không vượt port
3. Đạt physical constraint
```

### Kiểm tra

- [x] Same component → reject
- [x] Node hết port → reject
- [x] Edge quá dài → reject

---

## Task A2.3 – `compareEdges()`

### Sort

```text
cablingCost tăng dần
```

Tie-breaker:

```text
edgeId tăng dần
```

### Kiểm tra

- [x] Cost nhỏ hơn đứng trước
- [x] Cost bằng nhau → ID nhỏ hơn
- [x] Hai lần chạy cùng dữ liệu cho cùng thứ tự

---

## Task A2.4 – `buildMST()`

### File

```text
include/Kruskal.h
src/Kruskal.cpp
```

### Quy trình

```text
sort
 ↓
DSU
 ↓
canSelectEdge
 ↓
unite
 ↓
update usedPorts
 ↓
add edge to MST
```

### Kiểm tra

Graph connected:

```text
edgeCount = V - 1
```

- [x] Không cycle
- [x] Connected
- [x] Tổng cost đúng

Graph disconnected:

- [x] `connected = false`
- [x] Report rõ không có spanning tree

---

## Task A2.5 – `selectBackupLinks()`

### Đây là task bắt buộc của Tuần 2.

### Cách làm cơ bản

- [x] Lấy edge chưa được chọn vào MST
- [x] Sort theo cost
- [x] Chọn `k` edge phù hợp
- [x] Không vượt port
- [x] `isBackup = true`
- [x] `isBuilt = true`

### Cách làm tốt hơn

Với mỗi tree edge:

- [x] Tạm loại tree edge
- [x] Xác định hai phía bị tách
- [x] Tìm edge ngoài cây nối hai phía
- [x] Chọn backup phù hợp

### Kiểm tra

- [x] Backup không trùng MST
- [x] Backup có `isBackup = true`
- [x] Backup thật sự tạo đường thay thế
- [x] Backup không làm vượt port

---

# 7.2. 🕵️ Ý – TUẦN 2

## Task B2.1 – `CostBreakdown`

### File

```text
include/CostModel.h
```

### Thành phần

```text
install
material
equipment
maintenance
total
```

### Công thức

```text
CablingCost
=
installationCost
+
length × unitPrice × terrainFactor
+
equipmentCost
+
maintenanceCost
```

---

## Task B2.2 – `calculateCablingCost()`

### File

```text
src/CostModel.cpp
```

### Kiểm tra

- [x] Tính tay một edge
- [x] Code khớp kết quả tính tay
- [x] Terrain tăng → cost tăng
- [x] Media thay đổi → cost thay đổi
- [x] Không cộng routing delay vào cabling cost

---

## Task B2.3 – Physical Constraints

### File

```text
include/Validation.h
src/Validation.cpp
```

### Hàm

```cpp
isStaticFeasible()
checkMaxLength()
checkPortCapacity()
```

### Phân biệt

**Static:**

```text
length <= maxSegmentLength
```

**Dynamic trong Kruskal:**

```text
usedPorts < totalPorts
```

### Kiểm tra

- [x] Copper 130 m, max 100 m → reject
- [x] Node hết port → reject
- [x] Edge bị loại không làm tăng `usedPorts`

---

## Task B2.4 – Reporter

### File

```text
include/Reporter.h
src/Reporter.cpp
```

### `printMSTReport()`

Phải có:

```text
Edge ID
u-v
Media
MST/Backup
Cabling Cost
```

### So sánh

```text
Xây tất cả
MST
MST + Backup
```

---

# 7.3. ✅ CHECKPOINT TUẦN 2

## Quyến

- [x] DSU
- [x] Kruskal
- [x] Tie-breaker
- [x] `canSelectEdge`
- [x] `buildMST`
- [x] Backup

## Ý

- [x] CostBreakdown
- [x] CablingCost
- [x] Static constraints
- [x] Port validation
- [x] MST report

## Cả hai

- [x] T01
- [x] T02
- [x] T03
- [x] T04
- [x] T05
- [x] Backup có trong report

### Đối chiếu mạng mẫu

```text
MST cost = 6544 ĐV
```

Ví dụ thêm backup:

```text
MST + backup cost = 12440 ĐV
```

```text
week2-done
```

---

# 8. TUẦN 3 – DIJKSTRA + DELAY + MTU + TCP/UDP

## 🎯 Mục tiêu tuần

Route phải chạy được trên topology đã xây:

```text
MST + Backup
       ↓
Dijkstra
       ↓
PathResult
       ↓
Delay / MTU / TCP / UDP
```

---

# 8.1. 👨‍💻 QUYẾN – TUẦN 3

## Task A3.1 – `PathResult`

### File

```text
include/PathResult.h
```

### Fields

```text
reachable
nodes
edges
totalCost
hopCount
```

### Kiểm tra

- [x] Route success → `reachable = true`
- [x] Route fail → `reachable = false`
- [x] `hopCount = edges.size()`
- [x] Node path và edge path khớp nhau

---

## Task A3.2 – Dijkstra với trọng số đơn giản

Trước tiên test với:

```text
weight = 1
```

Sau đó:

```text
weight = length
```

### Kiểm tra

- [x] Kết quả hop count đúng
- [x] Kết quả length đúng
- [x] Có thể so sánh với BFS cho trường hợp weight = 1

---

## Task A3.3 – Min heap

```cpp
priority_queue<
    State,
    vector<State>,
    greater<State>
>
```

### Kiểm tra

- [x] Pop state nhỏ nhất
- [x] Bỏ record cũ
- [x] Không dùng queue thường

---

## Task A3.4 – Relaxation

```text
if dist[v] > dist[u] + w
```

Update:

```text
dist[v]
parent[v]
parentEdge[v]
```

### Kiểm tra

- [x] Parent đúng
- [x] ParentEdge đúng
- [x] Cạnh song song vẫn xác định đúng edge

---

## Task A3.5 – Reconstruct path

### Quy trình

```text
destination
 ↓
parent[]
 ↓
source
 ↓
reverse
```

### Kiểm tra

Output:

```text
H1 → R1 → R2 → R4 → H2
```

Không được ngược chiều.

---

# 8.2. 🕵️ Ý – TUẦN 3

## Task B3.1 – Transmission Delay

```text
dTrans =
(sizeBytes × 8)
/
(bandwidthMbps × 10^6)
```

### Kiểm tra

- [x] Byte → bit đúng
- [x] Mbps → bps đúng
- [x] Kết quả tính tay khớp code

---

## Task B3.2 – Propagation Delay

```text
dProp = length / propagationSpeed
```

### Kiểm tra

- [x] Đơn vị m / m/s = s

---

## Task B3.3 – Processing Delay

```text
dProc = nextNode.processingTime
```

### Kiểm tra

- [x] Dùng đúng node kế tiếp

---

## Task B3.4 – Queue Delay

```text
dQueue =
queueDelay × congestionPenalty(load)
```

### Kiểm tra

- [x] Không nhân penalty vào toàn bộ delay
- [x] Queue nền đúng với data

---

## Task B3.5 – Base Delay

```text
dBase =
dTrans
+
dProp
+
dProc
+
dQueue
```

### Kiểm tra

- [x] Từng thành phần đúng
- [x] Tổng đúng
- [x] Không chứa cabling cost

---

## Task B3.6 – MTU / DF

### Trường hợp

```text
size <= MTU
→ 1 mảnh
```

```text
size > MTU
+
canFragment = false
→ INF
```

```text
size > MTU
+
canFragment = true
→ fragmentation
```

### Kiểm tra

- [x] DF packet không đi qua edge không đủ MTU
- [x] Fragment packet vẫn có thể đi

---

## Task B3.7 – Fragmentation

```text
payload = MTU - 20
n = ceil(size / payload)
```

### Ví dụ

```text
3000 B
MTU = 1500
payload = 1480

n = ceil(3000 / 1480)
  = 3
```

### Kiểm tra

- [x] Fragment count = 3
- [x] Penalty đúng
- [x] Không nhầm `(n-1)` với `n`

---

## Task B3.8 – TCP / UDP

### TCP new connection

```text
protocol = TCP
newConnection = true
```

→ `estimateRTT(path)`.

### UDP

→ không có TCP setup.

### Kiểm tra

- [x] TCP new connection có overhead
- [x] UDP không có overhead TCP
- [x] Không dùng `TCP × 1.5`

---

# 8.3. ✅ CHECKPOINT TUẦN 3

## Quyến

- [x] Dijkstra
- [x] Min heap
- [x] Relaxation
- [x] Parent
- [x] ParentEdge
- [x] Reconstruct path

## Ý

- [x] Transmission
- [x] Propagation
- [x] Processing
- [x] Queue
- [x] Base delay
- [x] MTU
- [x] Fragmentation
- [x] TCP/UDP

## Test

- [x] T06
- [x] T07
- [x] T08
- [x] T09
- [x] T10

```text
week3-done
```

---

# 9. TUẦN 4 – QoS + CONGESTION + SIMULATION

## 🎯 Mục tiêu tuần

```text
Packet
 ↓
Snapshot
 ↓
Dynamic Cost
 ↓
Dijkstra
 ↓
Transmit
 ↓
Update Load
 ↓
Event
 ↓
Route có thể đổi
```

---

# 9.1. 👨‍💻 QUYẾN – TUẦN 4

## Task A4.1 – Congestion penalty

```text
load <= 0.80
→ penalty = 1

load > 0.80
→ penalty tăng theo model
```

Với model chuẩn:

```text
0.80 → 1.00
0.85 → 1.56
0.90 → 3.25
0.95 → 6.06
1.00 → 10.00
```

### Kiểm tra

- [x] Load hợp lệ
- [x] Penalty không âm
- [x] Load = 1 không vượt max model
- [x] ⚠️ CHÚ Ý DRY: Hàm `DelayModel::calcQueueDelay()` của Ý phải gọi hàm này của Quyến, không được để 2 nơi cùng tính penalty.

---

## Task A4.2 – QoS

### REAL_TIME

Ưu tiên:

```text
Queue
Delay
```

### BULK_DATA

Quan tâm:

```text
Transmission
Bandwidth
Congestion
```

### CONTROL

Cost trung tính.

### Kiểm tra

- [x] Weight ở `Config.h`
- [x] Không hard-code trong function
- [x] Đổi weight → route có thể đổi

---

## Task A4.3 – `calculateRoutingCost()`

```text
wTrans × dTrans
+
wProp × dProp
+
wProc × dProc
+
wQueue × dQueue × congestionPenalty
+
fragmentPenalty

(⚠️ CHÚ Ý DRY: Để lấy dTrans, dProp..., Dijkstra CẦN phải #include "DelayModel.h" và gọi hàm của Ý. Tuyệt đối không tự viết lại công thức toán học!)
```

### Kiểm tra

- [x] Link DOWN → INF
- [x] DF vượt MTU → INF
- [x] Weight không âm
- [x] Không NaN

---

## Task A4.4 – Tích hợp với Dijkstra

Dijkstra chỉ gọi:

```cpp
calculateRoutingCost(...)
```

Không tự tính:

```text
QoS
Congestion
MTU
Delay
```

---

# 9.2. 🕵️ Ý – TUẦN 4

## Task B4.1 – NetworkSnapshot

### Lưu

```text
isUp[]
load[]
queueDelay[]
```

### Mục đích

Dijkstra chạy trên trạng thái cố định trong một lần tính.

---

## Task B4.2 – `snapshotNetworkState()`

### Việc cần làm

- [x] Copy isUp
- [x] Copy load
- [x] Copy queueDelay
- [x] Return snapshot

### Kiểm tra

State Graph không tự thay đổi chỉ vì Dijkstra chạy.

---

## Task B4.3 – Scenario

```cpp
struct ScenarioEvent {
    EventType eventType;
    int targetId;
    double value;
    double time;
};
```

### Kiểm tra

- [x] Event có type
- [x] Có target
- [x] Có time

---

## Task B4.4 – `applyEvent()`

Hỗ trợ:

```text
LOAD_UPDATE
DELAY_UPDATE
```

### Kiểm tra

- [x] Đúng target
- [x] Đúng value
- [x] Đúng thời gian

---

## Task B4.5 – `sendPacket()`

### Làm

- [x] Duyệt path
- [x] Tính 4 delay (⚠️ CHÚ Ý DRY: BẮT BUỘC gọi `DelayModel::calculateTotalDelay()`, KHÔNG tự bóc tách tính toán lại công thức)
- [x] Kiểm tra MTU
- [x] Tính fragmentation
- [x] Cộng TCP overhead một lần
- [x] Tạo SimulationResult

---

## Task B4.6 – `updateNetworkState()`

### Quy tắc

```text
loadDelta = dTrans / LOAD_WINDOW_SECONDS
(⚠️ CHÚ Ý DRY: Phải gọi lại `DelayModel::calcTransmissionDelay(...)` để lấy dTrans, KHÔNG viết lại công thức size*8/bw)

currentLoad =
min(1.0, currentLoad + loadDelta)
```

Có thể decay:

```text
currentLoad *= LOAD_DECAY
```

### Kiểm tra

- [x] Load không vượt 1
- [x] Nhiều packet → load tăng
- [x] Có thể giảm theo decay

---

## Task B4.7 – Scenario data

Tạo:

```text
congestion.txt
qos.txt
mtu.txt
```

### Kiểm tra

- [x] Thay scenario không cần sửa code
- [x] Chạy cùng scenario 2 lần → output tái lập

---

# 9.3. ✅ CHECKPOINT TUẦN 4

## Quyến

- [x] Congestion
- [x] QoS
- [x] Dynamic routing cost
- [x] Dijkstra integration

## Ý

- [x] Snapshot
- [x] Scenario
- [x] Event
- [x] Send packet
- [x] Update load

## Test

- [x] T11
- [x] T12
- [x] T13

### Demo

```text
REAL_TIME
→ policy 1

BULK_DATA
→ policy 2
```

Load:

```text
0.20 → 0.95
```

- [x] Penalty tăng mạnh
- [x] Route có thể đổi

```text
week4-done
```

---

# 10. TUẦN 5 – FAILURE + REROUTE + ĐỘ TIN CẬY

## 🎯 Mục tiêu tuần

```text
LINK_DOWN
 ↓
Kiểm tra path
 ↓
Reroute nếu cần
 ↓
Không có route → DROP
```

---

# 10.1. 👨‍💻 QUYẾN – TUẦN 5

## Task A5.1 – `setLinkState()`

### Hỗ trợ

```text
LINK_DOWN
LINK_UP
```

### Kiểm tra

- [ ] Link DOWN → `isUp = false`
- [ ] Link UP → `isUp = true`

---

## Task A5.2 – `isConnected()`

Dùng BFS/DFS.

### Kiểm tra

- [ ] Connected → true
- [ ] Disconnected → false
- [ ] Bỏ qua link DOWN

---

## Task A5.3 – `rerouteIfNeeded()`

```text
old path
 ↓
check edge status
 ↓
path còn hợp lệ?
 ├─ YES → giữ
 └─ NO → Dijkstra lại
```

### Kiểm tra

- [ ] Failure trên path → reroute
- [ ] Failure ngoài path → không reroute
- [ ] No route → DROP

---

## Task A5.4 – `handleFailureEvent()`

### Việc cần làm

- [ ] Apply failure
- [ ] Update state
- [ ] Check old path
- [ ] Rerun Dijkstra
- [ ] Check connectivity
- [ ] Report

---

## Task A5.5 – Fault-tolerance metric

### Chỉ số

```text
tỷ lệ cặp node còn liên lạc
=
số cặp còn connected
/
tổng số cặp
```

### So sánh

```text
MST
vs
MST + backup
```

### Kiểm tra

- [ ] Có dữ liệu
- [ ] Có bảng kết quả
- [ ] Có thể đưa vào report

---

# 10.2. 🕵️ Ý – TUẦN 5

## Task B5.1 – `failure.txt`

### Tạo case

- [x] Link trên path DOWN
- [x] Link ngoài path DOWN
- [x] Bridge DOWN
- [x] Multiple failures
- [x] LINK_UP phục hồi

---

## Task B5.2 – Backup analysis

### `findBackupForTreeEdge()`

Nếu chọn cách nâng cao:

- [x] Tách graph khi remove tree edge
- [x] Tìm edge nối hai phía
- [x] Chọn backup phù hợp
- [x] Ghi lại lý do chọn

---

## Task B5.3 – Failure report

Report:

```text
OLD PATH
↓
FAILED EDGE
↓
NEW PATH
```

### Ví dụ

```text
Old:
H1 → R1 → R2 → R4 → H2

Failure:
e0 DOWN

New:
H1 → R1 → R3 → R4 → H2
```

- [x] Đã hoàn thành hàm `printFailureRecoveryReport`
- [x] Định dạng chuẩn: OLD PATH -> FAILED EDGE -> NEW PATH
- [x] Báo cáo chi tiết các trường hợp Reroute thành công và DROP khi mất liên lạc

---

# 10.3. ✅ CHECKPOINT TUẦN 5

## Quyến

- [ ] Link state
- [ ] Connectivity
- [ ] Reroute
- [ ] Failure handler
- [ ] Reliability metric

## Ý

- [x] Failure scenario
- [x] Backup analysis
- [x] Failure report

## Test

- [ ] T14
- [ ] T15
- [ ] T16

### Kịch bản bắt buộc

```text
MST thuần
→ tắt bridge
→ mất liên lạc

MST + backup
→ tắt edge chính
→ có route mới

Tắt edge ngoài path
→ không reroute

Tắt đủ nhiều edge
→ DROP + lý do
```

```text
week5-done
```

---

# 11. TUẦN 6 – TÍCH HỢP + TEST + DEMO + BÁO CÁO

## 🎯 Mục tiêu

```text
Freeze
 ↓
Benchmark
 ↓
Correctness
 ↓
Regression
 ↓
Demo
 ↓
Report
 ↓
Release Candidate
```

---

# 11.1. 👨‍💻 QUYẾN – TUẦN 6

## Task A6.1 – Benchmark Kruskal

### Ghi

```text
V
E
runtime
```

### Bảng

```text
| V | E | Runtime |
```

---

## Task A6.2 – Benchmark Dijkstra

Tương tự:

```text
| V | E | Runtime |
```

---

## Task A6.3 – Correctness

### Kruskal

- [ ] V−1 edge
- [ ] No cycle
- [ ] Connected
- [ ] Cost đúng

### Dijkstra

- [ ] Source distance = 0
- [ ] Parent hợp lệ
- [ ] Route đúng
- [ ] Không đi link DOWN

---

## Task A6.4 – Edge cases

- [ ] Empty graph
- [ ] 1 node
- [ ] Disconnected
- [ ] Duplicate edge
- [ ] Invalid node
- [ ] Invalid packet
- [ ] Invalid MTU

---

## Task A6.5 – Refactor

- [ ] Xóa duplicate code
- [ ] Xóa debug
- [ ] Chuẩn hóa naming
- [ ] Chuẩn hóa include
- [ ] Không duplicate formula

---

## Task A6.6 – Complexity report

```text
Kruskal:
O(E log E)

Dijkstra:
O((V + E) log V)
```

### Kiểm tra

- [ ] Giải thích được
- [ ] Khớp implementation

---

# 11.2. 🕵️ Ý – TUẦN 6

## Task B6.1 – `main.cpp`

### Main chỉ làm

```text
load data
↓
validate
↓
build MST
↓
load scenario
↓
run simulation
↓
report
```

### Không được đặt trong main

- [x] Kruskal loop (đã đóng gói trong Kruskal.h/cpp)
- [x] Dijkstra priority queue (đã đóng gói trong Dijkstra.h/cpp)
- [x] Delay formula (đã đóng gói trong DelayModel.h/cpp)
- [x] QoS formula (đã đóng gói trong RoutingCost.h/cpp)
- [x] Congestion formula (đã đóng gói trong RoutingCost.h/cpp)
- [x] Simulation pipeline (đã đóng gói trong Simulator::runScenarioSimulation)

---

## Task B6.2 – Menu Scenario

```text
1. Basic
2. QoS
3. Congestion
4. MTU
5. Failure
6. Redundancy
```

- [x] Menu điều hướng đầy đủ 6 kịch bản mô phỏng
- [x] Tích hợp thêm xem báo cáo quy hoạch MST và bảng định tuyến
- [x] Tự động reset trạng thái mạng giữa các lần chạy kịch bản
- [x] Không cần sửa code hay build lại khi chuyển kịch bản

---

## Task B6.3 – Reporter

### In được

- [x] MST
- [x] Cabling cost
- [x] Backup cost
- [x] Path
- [x] Delay
- [x] Hop
- [x] Fragment
- [x] Packet status
- [x] Failure
- [x] Old/New path

---

## Task B6.4 – 6 bộ demo

```text
basic
qos
congestion
mtu
failure
redundancy
```

### Kiểm tra

- [x] Mỗi bộ có mục đích riêng
- [x] Có expected result
- [x] Chạy được không sửa source

---

## Task B6.5 – Logging

Xuất:

- [x] `.txt` (Đã lưu tại `data/logs/log_<scenario>.txt`)

### Log

- [x] packet
- [x] source
- [x] destination
- [x] path
- [x] delay
- [x] hop
- [x] fragment
- [x] status
- [x] failure

---

## Task B6.6 – Documentation

### Tạo

```text
docs/architecture/
docs/formulas/
docs/test-results/
docs/demo/
```

### Nội dung

- [x] Architecture
- [x] 11-step flow
- [x] Formulas
- [x] Kruskal
- [x] Dijkstra
- [x] MTU
- [x] TCP/UDP
- [x] QoS
- [x] Congestion
- [x] Failure
- [x] Backup
- [x] Test result
- [x] Demo result

---

# 11.3. 🤝 CẢ HAI – CUỐI TUẦN 6

## Regression

Chạy:

```text
T01 → T20
```

### Kiểm tra

- [ ] Module mới pass
- [ ] Module cũ vẫn pass
- [ ] Không có regression

## Integration

- [ ] Load
- [ ] Validate
- [ ] MST
- [ ] Backup
- [ ] Packet
- [ ] Snapshot
- [ ] Dijkstra
- [ ] Transmission
- [ ] Update Load
- [ ] Event
- [ ] Reroute
- [ ] Report

## Tập bảo vệ

### Quyến phải hiểu

- [ ] IOManager
- [ ] CostModel
- [ ] Snapshot
- [ ] Simulator

### Ý phải hiểu

- [ ] DSU
- [ ] Kruskal
- [ ] Dijkstra
- [ ] Path reconstruction
- [ ] Complexity

---

# 12. MA TRẬN KIỂM TRA THEO TUẦN

## Tuần 1

| Test | Người | Pass condition |
|---|---|---|
| Graph add/get | Quyến | Node/edge đúng |
| Adjacency | Quyến | `sum(adj)=2E` |
| DSU cơ bản | Quyến | Component đúng |
| Parser | Ý | TXT load đúng |
| Enum mapping | Ý | Mapping đúng |
| Invalid data | Ý | Reject + lý do |
| Full load | Quyến+Ý | Main load 3 file không crash |

## Tuần 2

| Test | Người | Pass condition |
|---|---|---|
| DSU | Quyến | Root đúng |
| Kruskal | Quyến | `V-1`, connected |
| Tie-breaker | Quyến | Deterministic |
| Backup | Quyến | Có đường dự phòng |
| CablingCost | Ý | Khớp tính tay |
| Terrain | Ý | Terrain ↑ → cost ↑ |
| Port | Quyến+Ý | Không vượt port |
| Reporter | Ý | Output đúng |

## Tuần 3

| Test | Người | Pass condition |
|---|---|---|
| Dijkstra | Quyến | Route đúng |
| Parent | Quyến | Path reconstruction đúng |
| Transmission | Ý | Formula đúng |
| Propagation | Ý | Formula đúng |
| Processing | Ý | Next node đúng |
| Queue | Ý | Queue đúng |
| MTU | Ý | DF block đúng |
| Fragment | Ý | Count đúng |
| TCP | Ý | RTT overhead |
| UDP | Ý | Không setup |

## Tuần 4

| Test | Người | Pass condition |
|---|---|---|
| Congestion | Quyến | Penalty tăng |
| QoS | Quyến | Policy khác |
| Routing cost | Quyến | Cost đúng |
| Snapshot | Ý | State ổn định |
| Event | Ý | Apply đúng |
| Load update | Ý | Load thay đổi |
| Scenario | Ý | TXT chạy |
| Dynamic route | Quyến+Ý | Route có thể đổi |

## Tuần 5

| Test | Người | Pass condition |
|---|---|---|
| Link down | Quyến | `isUp=false` |
| Connectivity | Quyến | BFS/DFS đúng |
| Reroute | Quyến | Broken path có route mới |
| Fault metric | Quyến | Có số liệu |
| Failure TXT | Ý | Scenario chạy |
| Backup analysis | Ý | Backup hợp lệ |
| Failure report | Ý | Old/New path |

## Tuần 6

| Test | Người | Pass condition |
|---|---|---|
| Benchmark | Quyến | Runtime recorded |
| Correctness | Quyến | Invariants pass |
| Edge cases | Quyến | Không crash |
| Main | Ý | Orchestration only |
| Reporter | Ý | Output rõ |
| 6 datasets | Ý | Đủ demo |
| Log | Ý | TXT |
| Regression | Quyến+Ý | T01–T20 pass |

---

# 13. 20 TEST CASE CHÍNH

| ID | Test | Pass condition |
|---|---|---|
| T01 | MST cơ bản | `V-1`, connected, no cycle |
| T02 | MST cost | Tổng = sum cost |
| T03 | Terrain | Terrain tăng → cost tăng |
| T04 | Media | Media đổi → thứ tự có thể đổi |
| T05 | Port | Không vượt port |
| T06 | Shortest path | Route cost min |
| T07 | Delay | 4 delay components đúng |
| T08 | MTU + DF | Size > MTU + DF → blocked |
| T09 | Fragment | Count/penalty đúng |
| T10 | TCP/UDP | TCP setup đúng, UDP không setup |
| T11 | QoS | Policy khác nhau |
| T12 | Congestion | High load → penalty cao |
| T13 | Load reroute | Route có thể đổi |
| T14 | Failure | Broken path reroutes |
| T15 | Disconnected | No route → DROP |
| T16 | Backup | Backup restore path |
| T17 | Scenario Event | TXT event apply đúng |
| T18 | Multi Failure | Reroute qua nhiều sự cố |
| T19 | Performance | Runtime recorded |
| T20 | Regression | Test cũ vẫn pass |

---

# 14. CHECKLIST "ĐÚNG / SAI" TRƯỚC KHI MERGE

## 👨‍💻 Quyến

### Tôi phải trả lời được

- [ ] Task này giải quyết vấn đề gì?
- [ ] Input của task là gì?
- [ ] Output của task là gì?
- [ ] Thuật toán/công thức nào được dùng?
- [ ] Test case nào chứng minh task đúng?
- [ ] Vì sao output này là đúng?

### Không merge nếu

- [ ] Chưa có test
- [ ] Chưa biết expected result
- [ ] Không giải thích được code
- [ ] Tự sửa API chung
- [ ] Tạo struct duplicate

---

## 🕵️ Ý

### Tôi phải trả lời được

- [ ] Dữ liệu vào từ file nào?
- [ ] Validation nào áp dụng?
- [ ] Công thức nào?
- [ ] Unit là gì?
- [ ] State trước/sau thay đổi thế nào?
- [ ] Scenario nào kiểm tra phần này?

### Không merge nếu

- [ ] Hard-code số chính sách
- [ ] Dữ liệu sai format
- [ ] Không có valid/invalid case
- [ ] Dijkstra tự sửa state
- [ ] Đưa logic module vào `main.cpp`

---

# 15. QUY TRÌNH COMMIT / REVIEW

## Đầu tuần

```text
pull develop
↓
feature/<module>
↓
đọc API chung
```

## Trong tuần

```text
code
↓
test
↓
commit nhỏ
```

## Giữa tuần

```text
Quyến review Ý
Ý review Quyến
```

## Cuối tuần

```text
compile
↓
run all tests
↓
integration
↓
merge
↓
tag weekN-done
```

### Commit convention

```text
feat: add dijkstra shortest path
feat: add congestion penalty
fix: correct transmission delay unit
test: add mtu fragmentation scenario
refactor: move routing weights to Config
docs: update week 4 design
```

---

# 16. KHÔNG MERGE KHI

- [ ] Code chưa compile
- [ ] Chưa có test scenario
- [ ] Sửa API nhưng chưa thông báo
- [ ] Thay data chưa review
- [ ] Có duplicate struct
- [ ] Hard-code policy
- [ ] Không giải thích được kết quả test

---

# 17. FINAL DEFINITION OF DONE

- [ ] Load topology từ data
- [ ] Validation
- [ ] Graph + adjacency
- [ ] Kruskal
- [ ] DSU
- [ ] Cabling cost
- [ ] Port constraint
- [ ] Max segment length
- [ ] Backup links
- [ ] Reliability cost report
- [ ] Dijkstra
- [ ] Path reconstruction
- [ ] Transmission delay
- [ ] Propagation delay
- [ ] Processing delay
- [ ] Queue delay
- [ ] MTU
- [ ] DF
- [ ] Fragmentation
- [ ] TCP/UDP
- [ ] QoS
- [ ] Congestion
- [ ] Snapshot
- [ ] Load update
- [ ] Scenario
- [ ] Link failure
- [ ] Reroute
- [ ] Fault-tolerance metric
- [ ] 6 demo scenarios
- [ ] TXT logging
- [ ] Regression
- [ ] Benchmark
- [ ] Report
- [ ] Slides
- [ ] `release-candidate`

---

# 18. ROADMAP NGẮN GỌN

```text
WEEK 1
Quyến → Graph + DSU
Ý → IO + Validation
        ↓
Topology chạy được

WEEK 2
Quyến → DSU + Kruskal + Backup
Ý → Cost + Constraint + Report
        ↓
MST + Backup

WEEK 3
Quyến → Dijkstra + Path
Ý → Delay + MTU + TCP/UDP
        ↓
Route chạy được

WEEK 4
Quyến → QoS + Congestion + Dynamic Cost
Ý → Snapshot + Simulation + Scenario
        ↓
Dynamic Routing

WEEK 5
Quyến → Failure + Reroute + Reliability
Ý → Failure Scenario + Backup Analysis + Failure Report
        ↓
Fault Tolerance

WEEK 6
Quyến → Benchmark + Correctness + Regression
Ý → Main + Reporter + Dataset + Docs
        ↓
FINAL DEMO
```

---

# 19. SIGN-OFF HÀNG TUẦN

| Tuần | Quyến | Ý | Integration | Tag |
| Week 1 | [ ] | [ ] | [ ] | `week1-done` |
| Week 2 | [ ] | [ ] | [ ] | `week2-done` |
| Week 3 | [ ] | [ ] | [ ] | `week3-done` |
| Week 4 | [ ] | [ ] | [ ] | `week4-done` |
| Week 5 | [ ] | [ ] | [ ] | `week5-done` |
| Week 6 | [ ] | [ ] | [ ] | `release-candidate` |

---

# 20. KẾ HOẠCH TRIỂN KHAI GIAO DIỆN ĐỒ HỌA (GUI - SFML + IMGUI)

## 20.1. Mục tiêu và Định hướng thiết kế (KISS - Đơn giản, Thực tế, Dễ hiểu)
1. **Tách biệt Backend và Frontend theo cách tự nhiên nhất:**
   - **Backend:** Giữ nguyên 100% các class thuật toán lõi (`Graph`, `CostModel`, `Kruskal`, `Dijkstra`, `Simulator`).
   - **Frontend:** Dùng **SFML** phụ trách vẽ canvas 2D (Node, Edge) và **ImGui** phụ trách thanh điều khiển, tabs số liệu, dropdown kịch bản.
   - **Không áp dụng mô hình phức tạp (MVC, SOLID, đa luồng):** Toàn bộ giao diện chạy trong 1 luồng duy nhất (Single-threaded Game Loop). Chỉ dùng 1 cấu trúc trạng thái đơn giản `AppState` để lưu kết quả tính toán gần nhất (Node đang chọn, danh sách cạnh MST, đường đi Dijkstra hiện tại) để hàm vẽ lấy ra hiển thị.
2. **Loại bỏ triệt để các tính năng rườm rà (No Over-engineering):**
   - **Không làm Animation di chuyển gói tin**: Dùng cơ chế **đổi màu highlight trực tiếp** (nhanh, rõ ràng khi thuyết trình).
   - **Không làm Zoom/Pan/Drag-Drop**: Thay vào đó, dùng **Circular Layout (Bố trí vòng tròn tự động)** bằng lượng giác cơ bản để đồ thị luôn cân đối bất kể file có bao nhiêu node.
   - **Không vẽ nét đứt (Dashed Lines)**: Quy ước cáp Backup bằng **màu xanh dương mờ (kênh Alpha thấp)** để SFML vẽ 1 lệnh mượt mà 60 FPS.
   - **Mô phỏng từng bước (Step-by-step)**: Dùng nút bấm `[▶ Bước kế tiếp (Step Next)]` thay vì chạy timeline phức tạp, giúp người thuyết trình hoàn toàn làm chủ tiến trình demo.

---

## 20.2. Bố cục màn hình tổng thể (UI Layout: 1280 x 720)

```text
+---------------------------------------------------------------------------------------------------+
| [VÙNG 1: TOP TOOLBAR (ImGui Menu Bar)]                                                            |
| Chế độ hiển thị: (o) Lớp Vật lý (MST/Backup)   ( ) Lớp Logic (Routing/Traffic)                    |
| Thao tác nhanh:  [⚡ Chạy Kruskal]   [💥 Mô phỏng Cắt Cáp]   [🔄 Khôi phục Mạng]   [❌ Thoát]      |
+-------------------------------------------------------------------+-------------------------------+
|                                                                   | [VÙNG 3: BẢNG ĐIỀU KHIỂN &    |
| [VÙNG 2: CANVAS ĐỒ THỊ MẠNG (SFML RenderWindow)]                  |  SỐ LIỆU ĐỘNG (ImGui Panel)]  |
| - Chiếm 65% chiều rộng bên trái (vùng X: 0 -> 820px).             | - Chiếm 35% chiều rộng phải   |
| - Tọa độ các Node tự động xếp thành hình tròn quanh tâm Canvas.    |   (vùng X: 820 -> 1280px).    |
| - Node: Hình tròn Router/Host kèm ID và Name.                     | - Hệ thống Tabs trực quan:    |
| - Edge: Vẽ đường nối giữa 2 Node:                                 |   + Tab 1: Bảng Cáp (MST)     |
|   + Cáp MST: Nét đậm màu xanh lá cây (Alpha 255).                 |   + Tab 2: Bảng Định Tuyến    |
|   + Cáp Backup: Màu xanh dương mờ (Alpha 120).                    |   + Tab 3: Mô phỏng Scenario  |
|   + Cáp bị đứt (Down): Màu đỏ tươi.                               |                               |
|   + Tuyến đường gói tin được chọn: Màu vàng viền sáng.            |                               |
| - Click chuột trái vào 1 Node trên canvas -> Tab 2 tự động        |                               |
|   chọn xem bảng định tuyến của Node đó.                           |                               |
+-------------------------------------------------------------------+-------------------------------+
| [STATUS BAR (ImGui)]: Hiển thị trạng thái hệ thống và thông báo tóm tắt sự kiện vừa diễn ra        |
+---------------------------------------------------------------------------------------------------+
```

---

## 20.3. Chi tiết các Task triển khai theo từng Phase

### PHASE 1: TÍCH HỢP MÔI TRƯỜNG & KHỞI TẠO CỬA SỔ
- **Mục tiêu:** Cấu hình thư viện SFML + ImGui-SFML, tạo file `main_gui.cpp` với game loop tuần tự đơn giản.
- **Task G1.1 - Cấu hình thư viện và Build System:**
  - Tích hợp header & lib SFML và bộ wrapper `imgui-sfml` (`imgui.h`, `imgui.cpp`, `imgui_draw.cpp`, `imgui_tables.cpp`, `imgui_widgets.cpp`, `imgui-SFML.h`, `imgui-SFML.cpp`).
  - Cập nhật lệnh biên dịch: `g++ -std=c++17 -Wall -Iinclude main_gui.cpp src/*.cpp ... -lsfml-graphics -lsfml-window -lsfml-system -lopengl32 -o bin/main_gui.exe`.
- **Task G1.2 - Tạo game loop cơ bản (`main_gui.cpp`):**
  - Khởi tạo cửa sổ SFML `1280 x 720` và `ImGui::SFML::Init(window);`.
  - Khai báo struct trạng thái đơn giản `AppState` có hàm `reset()` để dọn sạch rác bộ nhớ khi khôi phục mạng:
    ```cpp
    struct AppState {
        int selectedNode = 0;
        int activeScenarioIdx = 0;
        int currentEventStep = 0;
        MSTResult mstResult;
        PathResult currentPath;

        void reset(int defaultNodeId = 0) {
            selectedNode = defaultNodeId;
            currentEventStep = 0;
            mstResult = MSTResult();
            currentPath = PathResult();
        }
    };
    ```
  - Xây dựng vòng lặp tuần tự: Nhận event -> Update ImGui -> SFML Clear -> Vẽ Canvas đồ họa -> Render ImGui -> Window Display.

---

### PHASE 2: CANVAS ĐỒ THỊ MẠNG (SFML RENDERER)
- **Mục tiêu:** Hiển thị trực quan mạng từ `Graph` lên Canvas bên trái một cách tự động, không hardcode tọa độ.
- **Task G2.1 - Thuật toán Circular Layout (Bố trí vòng tròn tự động):**
  - Thay vì gán cứng tọa độ, tính toán tự động theo số lượng node trong `Graph`:
    ```cpp
    std::unordered_map<int, sf::Vector2f> nodePositions;
    float centerX = 410.0f, centerY = 360.0f, radius = 240.0f;
    std::vector<int> ids = graph.nodeIds();
    int n = ids.size();
    for (int i = 0; i < n; ++i) {
        float angle = i * (2.0f * 3.14159265f / n) - 3.14159265f / 2.0f;
        nodePositions[ids[i]] = sf::Vector2f(centerX + radius * cos(angle), centerY + radius * sin(angle));
    }
    ```
  - Giữ cho đồ thị luôn tròn đều, đẹp mắt, không bao giờ bị đè node dù nạp file có 4 hay 20 node.
- **Task G2.2 - Thứ tự vẽ (Z-Order) & Vẽ liên kết cáp (Edges):**
  - **QUY TẮC RENDER QUAN TRỌNG:** Luôn vẽ toàn bộ Dây cáp (Edges) **TRƯỚC**, sau đó mới vẽ các Node (Circles + Labels) đè lên trên để các đường dây không cắt ngang tâm và đè lên chữ.
  - Dùng `sf::VertexArray(sf::Lines, 2)` nối giữa 2 tọa độ node.
  - Quy ước màu sắc đơn giản (KISS):
    - Cáp chưa xây (Unbuilt): Xám mờ `sf::Color(120, 120, 120, 70)`.
    - Cáp MST chính: Xanh lá cây đậm `sf::Color(46, 204, 113, 255)`.
    - Cáp Backup: Xanh dương mờ `sf::Color(52, 152, 219, 120)` (thay thế cho việc vẽ nét đứt phức tạp).
    - Cáp bị đứt (`isUp == false`): Đỏ tươi `sf::Color(231, 76, 60, 255)`.
    - Cáp thuộc tuyến Dijkstra đang chọn: Vàng kim viền sáng `sf::Color(241, 196, 15, 255)`.
- **Task G2.3 - Vẽ Node và Nhãn tên (Vẽ sau Edges):**
  - Vẽ đè hình tròn `sf::CircleShape(20.0f)` lên trên đầu các đoạn dây cáp.
  - In ID và tên node (ví dụ: `Node 0: HN`) rõ ràng bên trong hoặc bên cạnh node.
- **Task G2.4 - Bắt sự kiện click chuột chọn Node (Chống xuyên thấu ImGui):**
  - **CHỐNG EVENT BLEEDING:** Chỉ xử lý click chọn Node khi chuột KHÔNG nằm trên giao diện ImGui:
    ```cpp
    if (!ImGui::GetIO().WantCaptureMouse) {
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            sf::Vector2f mousePos(event.mouseButton.x, event.mouseButton.y);
            // Kiểm tra click trúng Node nào thì gán appState.selectedNode = clickedId
        }
    }
    ```

---

### PHASE 3: BẢNG SỐ LIỆU & ĐIỀU KHIỂN (IMGUI CONTROLLER)
- **Mục tiêu:** Nối toàn bộ chức năng của PBL2 (Kruskal, Routing Table, 6 Kịch bản) thành các nút bấm và bảng trực quan.
- **Task G3.1 - Thanh Top Toolbar (Thao tác nhanh):**
  - Nút bấm `[⚡ Chạy MST (Kruskal)]`: Gọi `buildMST()` và `selectBackupLinks()`, cập nhật ngay vào `appState.mstResult`. Canvas lập tức đổi màu các dây MST và Backup.
  - Nút bấm `[💥 Cắt cáp thử nghiệm]`: Danh sách combo chọn 1 cạnh bất kỳ để đặt `isUp = false`, kiểm tra tính năng chịu lỗi.
  - Nút bấm `[🔄 Khôi phục Mạng]`: Đặt lại đồ thị về trạng thái sạch ban đầu, đồng thời gọi `appState.reset(graph.nodeIds().front());` để xóa sạch trạng thái cũ, chống lỗi truy xuất ngoài vùng nhớ.
- **Task G3.2 - Tab 1: Báo cáo Quy hoạch Cáp (Physical View):**
  - Dùng `ImGui::BeginTable()` hiển thị: `Edge ID`, `Link (U-V)`, `Media`, `Role (MST/Backup)`, `Cost (DV)`, `Status`.
  - Hiển thị 3 con số tổng kết chi phí: *Build All*, *Chỉ xây MST*, *MST + Backup*.
- **Task G3.3 - Tab 2: Bảng Định Tuyến Toàn Mạng (Logical View):**
  - Dropdown chọn Node nguồn (mặc định lấy theo `appState.selectedNode`).
  - Bảng định tuyến chi tiết: `Node Đích`, `Next Hop`, `Số Hop`, `Chi phí trễ (Cost)`, `Đường đi chi tiết`.
  - Khi có sự cố đứt cáp ở Task G3.1, bảng định tuyến tự động cập nhật dòng đường đi mới (Reroute) ngay trước mắt người xem.
- **Task G3.4 - Tab 3: Mô phỏng Kịch Bản Sự Kiện (Scenario Step-by-Step):**
  - Dropdown chọn 1 trong 6 kịch bản sẵn có (`basic`, `qos`, `congestion`, `mtu`, `failure`, `redundancy`).
  - Nút bấm `[▶ Bước kế tiếp (Step Next)]` và `[🔄 Bắt đầu lại kịch bản]`:
    - Mỗi lần bấm `[Step Next]`, áp dụng sự kiện tiếp theo trong kịch bản (ví dụ mốc `0.0s` -> `1.5s`).
    - Dây cáp tương ứng đổi màu, bảng log bên dưới đẩy thêm dòng sự kiện mới.
    - Cực kỳ thuận tiện, chủ động cho sinh viên khi đứng thuyết trình và trả lời câu hỏi của hội đồng chấm thi.

---

## 20.4. Tiêu chí nghiệm thu hoàn thành (Acceptance Criteria)
1. Biên dịch thành công file đồ họa `bin/main_gui.exe`, không có lỗi warning hoặc xung đột với code lõi.
2. Canvas SFML tự động bố trí các Node theo vòng tròn đều đặn, không bị chồng chéo hay tràn màn hình.
3. Bấm `[Chạy Kruskal]` -> Màu dây cáp trên canvas đổi sang Xanh lá (MST) và Xanh dương mờ (Backup) chuẩn xác.
4. Bấm `[Step Next]` trong Tab Kịch bản -> Sự kiện áp dụng tuần tự từng bước, đường đi Dijkstra đổi màu vàng/đỏ tương ứng, bảng định tuyến nhảy số đồng bộ.
5. Mã nguồn gọn gàng, chạy 1 luồng duy nhất, không dùng các mô hình thiết kế phức tạp (MVC, SOLID) gây khó đọc.

---

# END