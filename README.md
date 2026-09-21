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
│   ├── STPSimulator.h
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
│   ├── STPSimulator.cpp
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

- [ ] Load được `nodes.txt`
- [ ] Load được `edges.txt`
- [ ] Load được `packets.txt`
- [ ] Validation hoạt động
- [ ] Graph + adjacency hoạt động
- [ ] Struct lõi và API được chốt
---
# 6.1. CẢ HAI – NGÀY 1–2

## Task 1.1 – Chốt glossary và số liệu mẫu

### Việc cần làm

- [ ] Thống nhất nghĩa của `bandwidth`
- [ ] Thống nhất `queueDelay`
- [ ] Thống nhất `currentLoad`
- [ ] Thống nhất `MTU`
- [ ] Thống nhất `terrainFactor`
- [ ] Thống nhất `processingTime`
- [ ] Thống nhất đơn vị nội bộ

```text
size       → byte
bandwidth  → Mbps
length     → m
delay      → second
```

### Kiểm tra

- [ ] Hai bạn giải thích giống nhau khi hỏi cùng một thuật ngữ
- [ ] Không có file nào dùng đơn vị khác mà không chuyển đổi

---

## Task 1.2 – Vẽ mạng mẫu

### Việc cần làm

- [ ] Vẽ 6–8 node
- [ ] Ghi tên node
- [ ] Ghi các edge
- [ ] Ghi cost cáp
- [ ] Ghi bandwidth
- [ ] Ghi queue
- [ ] Ghi MTU
- [ ] Tính tay MST
- [ ] Tính tay route
### Kiểm tra
- [ ] Có đáp án chuẩn để cuối tuần 2–3 đối chiếu
- [ ] Có thể giải thích vì sao cạnh được chọn/bị loại
---
## Task 1.3 – Tạo repository
- [ ] Tạo Git repository
- [ ] Tạo branch `develop`
- [ ] Tạo branch riêng cho Quyến
- [ ] Tạo branch riêng cho Ý
- [ ] Tạo khung thư mục
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
- [ ] Không duplicate struct
- [ ] Cả Quyến/Ý compile cùng API
- [ ] Không tự thêm field làm thay đổi contract mà chưa thống nhất
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

- [ ] Thêm 3 node → `V() = 3`
- [ ] Thêm 2 edge → `E() = 2`
- [ ] Mỗi edge xuất hiện ở adjacency của cả hai đầu
- [ ] `sum(adj[u].size()) == 2E`
- [ ] `getEdge(edgeId)` trả đúng edge
- [ ] `otherEndpoint()` trả đúng node còn lại

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

- [ ] Tạo `parent`
- [ ] Tạo rank
- [ ] Khai báo `find`
- [ ] Khai báo `unite`

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

- [ ] Edge node ID không tồn tại
- [ ] Bandwidth = 0
- [ ] MTU = 0
- [ ] Load > 1
- [ ] Port âm
- [ ] Packet size âm

### Kiểm tra

- [ ] Có valid case
- [ ] Có invalid case
- [ ] Có output mong đợi cho từng invalid case

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

- [ ] Graph compile
- [ ] Add/get hoạt động
- [ ] Adjacency đúng
- [ ] DSU API tồn tại
- [ ] Dataset mẫu có sẵn

## Ý

- [x] Parser hoạt động
- [x] Validation hoạt động
- [x] String → enum hoạt động
- [x] Valid/invalid data đều test được

## Cả hai

- [ ] `main.cpp` đọc 3 file data không crash
- [ ] `sum(adj[u].size()) == 2E`
- [ ] Dùng chung `Types.h`
- [ ] Không duplicate struct

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

- [ ] Path compression
- [ ] Union by rank
- [ ] `find`
- [ ] `unite`

### Kiểm tra

- [ ] Các node cùng component có cùng root
- [ ] Unite cùng component trả false
- [ ] Không tạo cycle bằng cách unite hai node đã cùng root

---

## Task A2.2 – `canSelectEdge()`

### Edge chỉ được chọn khi

```text
1. Không tạo cycle
2. Không vượt port
3. Đạt physical constraint
```

### Kiểm tra

- [ ] Same component → reject
- [ ] Node hết port → reject
- [ ] Edge quá dài → reject

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

- [ ] Cost nhỏ hơn đứng trước
- [ ] Cost bằng nhau → ID nhỏ hơn
- [ ] Hai lần chạy cùng dữ liệu cho cùng thứ tự

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

- [ ] Không cycle
- [ ] Connected
- [ ] Tổng cost đúng

Graph disconnected:

- [ ] `connected = false`
- [ ] Report rõ không có spanning tree

---

## Task A2.5 – `selectBackupLinks()`

### Đây là task bắt buộc của Tuần 2.

### Cách làm cơ bản

- [ ] Lấy edge chưa được chọn vào MST
- [ ] Sort theo cost
- [ ] Chọn `k` edge phù hợp
- [ ] Không vượt port
- [ ] `isBackup = true`
- [ ] `isBuilt = true`

### Cách làm tốt hơn

Với mỗi tree edge:

- [ ] Tạm loại tree edge
- [ ] Xác định hai phía bị tách
- [ ] Tìm edge ngoài cây nối hai phía
- [ ] Chọn backup phù hợp

### Kiểm tra

- [ ] Backup không trùng MST
- [ ] Backup có `isBackup = true`
- [ ] Backup thật sự tạo đường thay thế
- [ ] Backup không làm vượt port

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

- [ ] DSU
- [ ] Kruskal
- [ ] Tie-breaker
- [ ] `canSelectEdge`
- [ ] `buildMST`
- [ ] Backup

## Ý

- [ ] CostBreakdown
- [ ] CablingCost
- [ ] Static constraints
- [ ] Port validation
- [ ] MST report

## Cả hai

- [ ] T01
- [ ] T02
- [ ] T03
- [ ] T04
- [ ] T05
- [ ] Backup có trong report

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

- [ ] Route success → `reachable = true`
- [ ] Route fail → `reachable = false`
- [ ] `hopCount = edges.size()`
- [ ] Node path và edge path khớp nhau

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

- [ ] Kết quả hop count đúng
- [ ] Kết quả length đúng
- [ ] Có thể so sánh với BFS cho trường hợp weight = 1

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

- [ ] Pop state nhỏ nhất
- [ ] Bỏ record cũ
- [ ] Không dùng queue thường

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

- [ ] Parent đúng
- [ ] ParentEdge đúng
- [ ] Cạnh song song vẫn xác định đúng edge

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

- [ ] Byte → bit đúng
- [ ] Mbps → bps đúng
- [ ] Kết quả tính tay khớp code

---

## Task B3.2 – Propagation Delay

```text
dProp = length / propagationSpeed
```

### Kiểm tra

- [ ] Đơn vị m / m/s = s

---

## Task B3.3 – Processing Delay

```text
dProc = nextNode.processingTime
```

### Kiểm tra

- [ ] Dùng đúng node kế tiếp

---

## Task B3.4 – Queue Delay

```text
dQueue =
queueDelay × congestionPenalty(load)
```

### Kiểm tra

- [ ] Không nhân penalty vào toàn bộ delay
- [ ] Queue nền đúng với data

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

- [ ] Từng thành phần đúng
- [ ] Tổng đúng
- [ ] Không chứa cabling cost

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

- [ ] DF packet không đi qua edge không đủ MTU
- [ ] Fragment packet vẫn có thể đi

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

- [ ] Fragment count = 3
- [ ] Penalty đúng
- [ ] Không nhầm `(n-1)` với `n`

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

- [ ] TCP new connection có overhead
- [ ] UDP không có overhead TCP
- [ ] Không dùng `TCP × 1.5`

---

# 8.3. ✅ CHECKPOINT TUẦN 3

## Quyến

- [ ] Dijkstra
- [ ] Min heap
- [ ] Relaxation
- [ ] Parent
- [ ] ParentEdge
- [ ] Reconstruct path

## Ý

- [ ] Transmission
- [ ] Propagation
- [ ] Processing
- [ ] Queue
- [ ] Base delay
- [ ] MTU
- [ ] Fragmentation
- [ ] TCP/UDP

## Test

- [ ] T06
- [ ] T07
- [ ] T08
- [ ] T09
- [ ] T10

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

- [ ] Load hợp lệ
- [ ] Penalty không âm
- [ ] Load = 1 không vượt max model

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

- [ ] Weight ở `Config.h`
- [ ] Không hard-code trong function
- [ ] Đổi weight → route có thể đổi

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
```

### Kiểm tra

- [ ] Link DOWN → INF
- [ ] DF vượt MTU → INF
- [ ] Weight không âm
- [ ] Không NaN

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

- [ ] Copy isUp
- [ ] Copy load
- [ ] Copy queueDelay
- [ ] Return snapshot

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

- [ ] Event có type
- [ ] Có target
- [ ] Có time

---

## Task B4.4 – `applyEvent()`

Hỗ trợ:

```text
LOAD_UPDATE
DELAY_UPDATE
```

### Kiểm tra

- [ ] Đúng target
- [ ] Đúng value
- [ ] Đúng thời gian

---

## Task B4.5 – `sendPacket()`

### Làm

- [ ] Duyệt path
- [ ] Tính 4 delay
- [ ] Kiểm tra MTU
- [ ] Tính fragmentation
- [ ] Cộng TCP overhead một lần
- [ ] Tạo SimulationResult

---

## Task B4.6 – `updateNetworkState()`

### Quy tắc

```text
loadDelta = dTrans / LOAD_WINDOW_SECONDS

currentLoad =
min(1.0, currentLoad + loadDelta)
```

Có thể decay:

```text
currentLoad *= LOAD_DECAY
```

### Kiểm tra

- [ ] Load không vượt 1
- [ ] Nhiều packet → load tăng
- [ ] Có thể giảm theo decay

---

## Task B4.7 – Scenario data

Tạo:

```text
congestion.txt
qos.txt
mtu.txt
```

### Kiểm tra

- [ ] Thay scenario không cần sửa code
- [ ] Chạy cùng scenario 2 lần → output tái lập

---

# 9.3. ✅ CHECKPOINT TUẦN 4

## Quyến

- [ ] Congestion
- [ ] QoS
- [ ] Dynamic routing cost
- [ ] Dijkstra integration

## Ý

- [ ] Snapshot
- [ ] Scenario
- [ ] Event
- [ ] Send packet
- [ ] Update load

## Test

- [ ] T11
- [ ] T12
- [ ] T13

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

- [ ] Penalty tăng mạnh
- [ ] Route có thể đổi

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

STP là **phần mở rộng**, không phải phần lõi bắt buộc nếu tiến độ không đủ.

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

- [ ] Link trên path DOWN
- [ ] Link ngoài path DOWN
- [ ] Bridge DOWN
- [ ] Multiple failures
- [ ] LINK_UP phục hồi

---

## Task B5.2 – Backup analysis

### `findBackupForTreeEdge()`

Nếu chọn cách nâng cao:

- [ ] Tách graph khi remove tree edge
- [ ] Tìm edge nối hai phía
- [ ] Chọn backup phù hợp
- [ ] Ghi lại lý do chọn

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

---

## Task B5.4 – STP (MỞ RỘNG)

### File

```text
include/STPSimulator.h
src/STPSimulator.cpp
```

### Hàm

```cpp
buildForwardingTree()
blockRedundantEdges()
```

### Logic

```text
Redundant edge
→ BLOCKING
```

Nhưng:

```text
physical UP
logical BLOCKING
```

Khi forwarding link down:

```text
backup
→ FORWARDING
```

---

# 10.3. ✅ CHECKPOINT TUẦN 5

## Quyến

- [ ] Link state
- [ ] Connectivity
- [ ] Reroute
- [ ] Failure handler
- [ ] Reliability metric

## Ý

- [ ] Failure scenario
- [ ] Backup analysis
- [ ] Failure report
- [ ] STP nếu còn thời gian

## Test

- [ ] T14
- [ ] T15
- [ ] T16
- [ ] T17 nếu làm STP

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

- [ ] Kruskal loop
- [ ] Dijkstra priority queue
- [ ] Delay formula
- [ ] QoS formula
- [ ] Congestion formula

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

---

## Task B6.3 – Reporter

### In được

- [ ] MST
- [ ] Cabling cost
- [ ] Backup cost
- [ ] Path
- [ ] Delay
- [ ] Hop
- [ ] Fragment
- [ ] Packet status
- [ ] Failure
- [ ] Old/New path

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

- [ ] Mỗi bộ có mục đích riêng
- [ ] Có expected result
- [ ] Chạy được không sửa source

---

## Task B6.5 – Logging

Xuất:

```text
.txt
```

### Log

```text
packet
source
destination
path
delay
hop
fragment
status
failure
```

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

- [ ] Architecture
- [ ] 11-step flow
- [ ] Formulas
- [ ] Kruskal
- [ ] Dijkstra
- [ ] MTU
- [ ] TCP/UDP
- [ ] QoS
- [ ] Congestion
- [ ] Failure
- [ ] Backup
- [ ] Test result
- [ ] Demo result

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
| STP | Ý | Blocking/Forwarding |

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
| T17 | STP | Redundant edge BLOCKING |
| T18 | Scenario | TXT event apply |
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
Ý → Failure Scenario + Backup Analysis + STP
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

# END