# BÁO CÁO VÀ GIẢI THÍCH CHI TIẾT TẤT CẢ CÁC FILE TUẦN 5
**Chủ đề:** Failure Simulation + Backup Analysis + Rerouting + Failure Report (Xử lý sự cố & Khả năng chịu lỗi)

---

Tài liệu này tổng hợp toàn bộ các đoạn code quan trọng nhất của **Tuần 5**, kèm comment giải thích chi tiết ngay tại từng dòng code và phân tích cơ chế xử lý sự cố đứt cáp, tự động phục hồi đường truyền trong mạng viễn thông.

---

## 1. Task B5.1 – Kịch bản sự cố đứt cáp & phục hồi (`LINK_DOWN` / `LINK_UP`)

* **File:** [src/Simulator.cpp](file:///d:/PBL2/Project_PBL2/src/Simulator.cpp) (phương thức `Simulator::applyEvent`)
* **Ý nghĩa:** Tiếp nhận các sự kiện vật lý xảy ra theo thời gian: khi dây cáp bị đứt (`LINK_DOWN`), cạnh sẽ bị vô hiệu hóa (`isUp = false`), khiến Dijkstra trả về chi phí vô cực ($INF$) và buộc thuật toán phải tìm kiếm đường đi thay thế qua nhánh dự phòng.

```cpp
void Simulator::applyEvent(Graph& graph, const ScenarioEvent& event) {
    try {
        // Truy xuất tham chiếu đến cạnh cần tác động theo ID
        Edge& targetEdge = graph.getEdge(event.targetId);

        switch (event.eventType) {
            case EventType::LINK_DOWN:
                // Sự cố vật lý: Đứt cáp, hỏng module quang hoặc router sập nguồn
                // Đánh dấu isUp = false -> Cáp bị loại khỏi quá trình định tuyến
                targetEdge.isUp = false;
                break;

            case EventType::LINK_UP:
                // Khôi phục kết nối: Đã nối xong cáp quang hoặc khởi động lại thiết bị
                // Đánh dấu isUp = true -> Cáp sẵn sàng cho lưu lượng lưu thông trở lại
                targetEdge.isUp = true;
                break;

            case EventType::LOAD_UPDATE:
                // Cập nhật mức tải lưu lượng
                targetEdge.currentLoad = std::max(0.0, std::min(1.0, event.value));
                break;

            case EventType::DELAY_UPDATE:
                // Cập nhật biến thiên trễ hàng đợi
                targetEdge.queueDelay = std::max(0.0, event.value);
                break;

            default:
                break;
        }
    } catch (const std::exception& e) {
        // Phòng ngừa trường hợp targetId trong file kịch bản không tồn tại
    }
}
```

* **5 kịch bản kiểm thử mẫu (trong `data/scenarios/failure.txt`):**
  1. *Link ngoài path DOWN:* Đứt cáp không thuộc lộ trình $\to$ Giữ nguyên đường đi, không reroute thừa.
  2. *Link trên path DOWN:* Cáp đường chính đứt $\to$ Tự động bẻ lái qua đường vòng dự phòng.
  3. *LINK_UP phục hồi:* Cáp chính sửa xong $\to$ Tự động chuyển tuyến về lại đường tối ưu ban đầu.
  4. *Đa sự cố (Multiple failures):* Đứt toàn bộ lối thoát $\to$ Hủy gói tin an toàn (`DROP`) với lý do `NO_ROUTE_AVAILABLE`.
  5. *Đứt cạnh cầu (Critical Bridge):* Đứt dây độc đạo dẫn vào Host $\to$ Host bị cô lập hoàn toàn.

---

## 2. Task B5.2 – Thuật toán phân tích cáp dự phòng (`Reliability::findAllBackupsForTreeEdge`)

* **File:** [include/Reliability.h](file:///d:/PBL2/Project_PBL2/include/Reliability.h) & [src/Reliability.cpp](file:///d:/PBL2/Project_PBL2/src/Reliability.cpp)
* **Ý nghĩa:** Khi một cạnh cây khung chính (Tree Edge) bị đứt, cây MST bị vỡ thành 2 nửa riêng biệt. Thuật toán sử dụng BFS để tìm tập node của hai nhánh, sau đó quét toàn bộ các cạnh ngoài cây còn hoạt động để tìm ra dây cáp nối được 2 nửa này lại với nhau.

```cpp
// Helper: Duyệt BFS tìm tất cả các node thuộc cùng 1 nhánh cây MST khi bỏ qua cạnh failedEdgeId
static std::unordered_set<int> getComponentNodes(const Graph& graph, 
                                                const std::vector<int>& mstTreeEdges, 
                                                int failedEdgeId, 
                                                int startNode) {
    std::unordered_set<int> comp; // Tập chứa các node cùng nhánh
    std::queue<int> q;            // Hàng đợi phục vụ duyệt theo chiều rộng (BFS)

    comp.insert(startNode);
    q.push(startNode);

    while (!q.empty()) {
        int u = q.front();
        q.pop();

        // Duyệt qua tất cả các cạnh kề với u
        for (int edgeId : graph.getNeighbors(u)) {
            // TUYỆT ĐỐI BỎ QUA cạnh đang giả định bị đứt
            if (edgeId == failedEdgeId) continue;

            // CHỈ cho phép duyệt qua các cạnh thuộc cây khung MST
            bool inMST = false;
            for (int tId : mstTreeEdges) {
                if (tId == edgeId) {
                    inMST = true;
                    break;
                }
            }
            if (!inMST) continue; // Cạnh ngoài cây không dùng để lan truyền tìm nhánh MST

            // Lấy đỉnh đầu kia của cạnh
            int v = graph.otherEndpoint(u, edgeId);
            if (comp.find(v) == comp.end()) {
                comp.insert(v); // Thêm vào danh sách các node cùng nhánh
                q.push(v);
            }
        }
    }
    return comp;
}

// Hàm quét tìm tất cả các ứng viên cáp dự phòng có thể nối lại 2 nhánh bị chia cắt
std::vector<BackupCandidate> findAllBackupsForTreeEdge(const Graph& graph,
                                                      const std::vector<int>& mstTreeEdges,
                                                      int failedTreeEdgeId) {
    std::vector<BackupCandidate> candidates;

    // 1. Kiểm tra xem cạnh đứt có thực sự là cạnh thuộc cây MST không
    bool found = false;
    for (int tId : mstTreeEdges) {
        if (tId == failedTreeEdgeId) {
            found = true;
            break;
        }
    }
    if (!found) return candidates; // Nếu không phải cạnh cây khung thì không cần tìm backup thay thế

    const Edge& failedEdge = graph.getEdge(failedTreeEdgeId);

    // 2. Chia cây MST thành 2 thành phần liên thông:
    // Nhánh A chứa đầu mút u, Nhánh B chứa đầu mút v
    std::unordered_set<int> compA = getComponentNodes(graph, mstTreeEdges, failedTreeEdgeId, failedEdge.u);
    std::unordered_set<int> compB = getComponentNodes(graph, mstTreeEdges, failedTreeEdgeId, failedEdge.v);

    // 3. Quét toàn bộ cạnh trong đồ thị để tìm các cạnh bắc cầu giữa Nhánh A và Nhánh B
    for (int edgeId : graph.edgeIds()) {
        // Bỏ qua chính cạnh đang đứt
        if (edgeId == failedTreeEdgeId) continue;

        // Bỏ qua tất cả các cạnh khác đã thuộc MST
        bool inMST = false;
        for (int tId : mstTreeEdges) {
            if (tId == edgeId) {
                inMST = true;
                break;
            }
        }
        if (inMST) continue;

        const Edge& e = graph.getEdge(edgeId);

        // Ràng buộc điều kiện cứu mạng: Cáp phải còn hoạt động (isUp) và thỏa mãn cự ly truyền dẫn
        if (!e.isUp || !Validation::isStaticFeasible(e)) continue;

        // Kiểm tra xem cạnh e có 1 đầu thuộc compA và 1 đầu thuộc compB hay không
        bool connectsAB = (compA.count(e.u) && compB.count(e.v)) ||
                          (compA.count(e.v) && compB.count(e.u));

        if (connectsAB) {
            BackupCandidate cand;
            cand.backupEdgeId = edgeId;
            // Tính chi phí lắp đặt của cáp dự phòng
            cand.cost = e.length * e.unitPrice * e.terrainFactor + e.equipmentCost + e.maintenanceCost;
            
            // Diễn giải lý do chọn chi tiết:
            if (e.isBuilt && e.isBackup) {
                cand.reason = "Cap du phong da duoc quy hoach xay dung (isBuilt=true, isBackup=true) noi hai nhanh " 
                            + std::to_string(e.u) + " va " + std::to_string(e.v);
            } else if (e.isBuilt) {
                cand.reason = "Cap mang da xay dung noi giua " + std::to_string(e.u) + " va " + std::to_string(e.v);
            } else {
                cand.reason = "Cap ung vien tiem nang ngoai cay noi giua " + std::to_string(e.u) + " va " + std::to_string(e.v);
            }

            candidates.push_back(cand);
        }
    }

    // 4. Sắp xếp các ứng viên theo thứ tự ưu tiên:
    // - Ưu tiên 1: Cáp đã được triển khai sẵn trong thực tế (isBuilt = true)
    // - Ưu tiên 2: Chi phí lắp đặt rẻ nhất
    std::sort(candidates.begin(), candidates.end(), [&](const BackupCandidate& a, const BackupCandidate& b) {
        const Edge& ea = graph.getEdge(a.backupEdgeId);
        const Edge& eb = graph.getEdge(b.backupEdgeId);
        if (ea.isBuilt != eb.isBuilt) return ea.isBuilt > eb.isBuilt; // Đã xây ưu tiên trước
        if (a.cost != b.cost) return a.cost < b.cost;                 // Chi phí rẻ hơn ưu tiên trước
        return a.backupEdgeId < b.backupEdgeId;
    });

    return candidates;
}
```

---

## 3. Task B5.2 (tiếp theo) – Chọn lựa Backup tối ưu hoặc phát hiện Cạnh Cầu Nguy Cấp (`findBackupForTreeEdge`)

* **File:** [src/Reliability.cpp](file:///d:/PBL2/Project_PBL2/src/Reliability.cpp)

```cpp
BackupCandidate findBackupForTreeEdge(const Graph& graph, 
                                      const std::vector<int>& mstTreeEdges, 
                                      int failedTreeEdgeId) {
    // Lấy toàn bộ danh sách ứng viên đã được sắp xếp ưu tiên
    std::vector<BackupCandidate> all = findAllBackupsForTreeEdge(graph, mstTreeEdges, failedTreeEdgeId);
    
    // Nếu danh sách rỗng: Nghĩa là không có bất kỳ dây nào khác nối được 2 phần của mạng
    // Đây là cạnh cầu độc đạo (Critical Bridge) -> Mất kết nối vĩnh viễn khi đứt
    if (all.empty()) {
        return {-1, 0.0, "Khong tim thay bat ky cap du phong nao co the noi lai hai phan bi chia cat (Canh cau nguy cap / Critical Bridge)"};
    }
    
    // Trả về ứng viên số 1 (ứng viên tốt nhất thỏa mãn cả tính sẵn sàng và giá thành)
    return all[0];
}
```

---

## 4. Task B5.3 – Báo cáo sự cố và khôi phục định tuyến (`Reporter::printFailureRecoveryReport`)

* **File:** [include/Reporter.h](file:///d:/PBL2/Project_PBL2/include/Reporter.h) & [src/Reporter.cpp](file:///d:/PBL2/Project_PBL2/src/Reporter.cpp)
* **Ý nghĩa:** Trực quan hóa diễn tiến xử lý sự cố đứt cáp theo cấu trúc chuẩn mực: `OLD PATH -> FAILED EDGE -> NEW PATH`, giúp kỹ sư mạng theo dõi rõ ràng lộ trình trước và sau biến cố.

```cpp
// Hàm in báo cáo phân tích sự cố và kết quả định tuyến lại
void printFailureRecoveryReport(const Graph& graph,
                                const Packet& pkt,
                                const PathResult& oldPath,
                                int failedEdgeId,
                                const PathResult& newPath,
                                const std::string& note) {
    std::cout << "\n==============================================================\n";
    std::cout << "           BAO CAO SU CO & DINH TUYEN LAI (TASK B5.3)         \n";
    std::cout << "==============================================================\n";
    std::cout << "Packet ID    : #" << pkt.id << " (" << pkt.source << " -> " << pkt.destination << ")\n";
    
    // 1. In đường đi ban đầu trước khi sự cố xảy ra (OLD PATH)
    std::cout << "OLD PATH     : " << formatPathString(oldPath);
    if (oldPath.reachable) {
        std::cout << " (Hops: " << oldPath.hopCount << ", Cost: " << std::fixed << std::setprecision(6) << oldPath.totalCost << ")";
    }
    std::cout << "\n";

    // 2. In thông tin chi tiết về cạnh cáp bị đứt (FAILED EDGE)
    std::cout << "FAILED EDGE  : ";
    if (graph.hasEdge(failedEdgeId)) {
        const Edge& e = graph.getEdge(failedEdgeId);
        std::cout << "Edge #" << failedEdgeId << " (" << e.u << " - " << e.v << ") bi LINK_DOWN\n";
    } else {
        std::cout << "Edge #" << failedEdgeId << " (Unknown edge)\n";
    }

    // 3. In đường đi mới sau khi hệ thống tự động tìm tuyến thay thế (NEW PATH)
    std::cout << "NEW PATH     : " << formatPathString(newPath);
    if (newPath.reachable) {
        // Tìm thấy đường vòng thay thế thành công
        std::cout << " (Hops: " << newPath.hopCount << ", Cost: " << std::fixed << std::setprecision(6) << newPath.totalCost << ")\n";
        std::cout << "KET QUA      : [THANH CONG] Da tim duong thay the thanh cong.\n";
    } else {
        // Không còn đường thoát -> Gói tin bị hủy an toàn
        std::cout << "\nKET QUA      : [DROP] Khong the dinh tuyen den dich (Bi co lap / Mat lien lac hoan toan).\n";
    }

    // In diễn giải nguyên nhân kỹ thuật hoặc thông báo kèm theo
    if (!note.empty()) {
        std::cout << "GHI CHU      : " << note << "\n";
    }
    std::cout << "==============================================================\n";
}
```

---

## 5. Tóm tắt kết quả kiểm thử Tuần 5
* `tests/test_failure_scenario.cpp`: 5/5 kịch bản đứt nối cáp đều xử lý chính xác tuyệt đối. Hệ thống bẻ lái thành công gói tin sang nhánh dự phòng và kích hoạt trạng thái DROP khi bị cô lập hoàn toàn.
* `tests/test_backup_analysis.cpp`: Tách cây chính xác 100%, tìm đúng cáp cứu viện số #2 và #3, đồng thời gắn cờ cảnh báo đúng đắn cạnh cầu độc đạo Edge #6 (nối tới Host 4).
* `tests/test_failure_report.cpp`: Kiểm tra định dạng đầu ra khớp hoàn toàn mẫu chuẩn `OLD PATH -> FAILED EDGE -> NEW PATH`.
