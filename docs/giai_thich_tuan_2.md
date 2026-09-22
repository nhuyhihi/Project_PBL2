# BÁO CÁO VÀ GIẢI THÍCH CHI TIẾT TẤT CẢ CÁC FILE TUẦN 2 (Ý)

Tài liệu này tổng hợp toàn bộ các file, cấu trúc code, ý nghĩa từng dòng và mục đích của các Task đã thực hiện trong Tuần 2 thuộc đồ án PBL2 của bạn (Ý). Cuối bài sẽ có phần trả lời 3 câu hỏi để kiểm tra hiểu sâu bản chất vấn đề.

---

## 1. Task B2.1 & B2.2 – Mô hình Chi phí Cáp (CostModel)

**Mục đích:** Khai báo cấu trúc bóc tách chi phí và hàm tính toán chi phí cụ thể để kéo cáp. Tránh việc gộp chung thành một cục tiền khó quản lý.

### 📄 File 1: `include/CostModel.h`

```cpp
#ifndef COSTMODEL_H
#define COSTMODEL_H

#include "Edge.h"

// Struct chứa chi tiết cấu thành nên tổng chi phí cáp
struct CostBreakdown {
    double install;       // Phí nhân công lắp đặt
    double material;      // Phí vật tư (Dài * Đơn giá * Hệ số địa hình)
    double equipment;     // Phí thiết bị 2 đầu
    double maintenance;   // Phí bảo trì định kỳ
    double total;         // Tổng chi phí (Sum của 4 món trên)
};

class CostModel {
public:
    // Hàm tĩnh (static) để tính toán chi phí từ thông số của Edge
    static CostBreakdown calculateCablingCost(const Edge& edge);
};

#endif // COSTMODEL_H
```
*Giải thích dòng code:*
- Dòng `struct CostBreakdown`: Nhóm 5 biến số thực thành một kiểu dữ liệu. Giúp khi hàm trả về có thể lấy được từng khoản phí lẻ chứ không chỉ là 1 con số tổng.
- Dòng `static CostBreakdown...`: Dùng từ khóa `static` để biến hàm này thành hàm toàn cục của lớp. Nghĩa là bạn có thể gọi `CostModel::calculateCablingCost(edge)` ở bất cứ đâu trong dự án mà không cần phải dùng lệnh `new CostModel()`.

---

### 📄 File 2: `src/CostModel.cpp`

```cpp
#include "CostModel.h"
#include "Config.h"

CostBreakdown CostModel::calculateCablingCost(const Edge& edge) {
    CostBreakdown breakdown;
    
    // Ràng buộc: "dùng INF cho cạnh không đi được"
    if (!edge.isUp) {
        breakdown.install = 0.0;
        breakdown.material = 0.0;
        breakdown.equipment = 0.0;
        breakdown.maintenance = 0.0;
        breakdown.total = Config::INF; // Gắn INF để thuật toán Kruskal né cạnh này
        return breakdown;
    }

    // 1. Phí lắp đặt lấy từ hằng số dùng chung
    breakdown.install = Config::INSTALLATION_COST;
    
    // 2. Phí vật tư = Dài * Giá 1 mét * Độ khó địa hình
    breakdown.material = edge.length * edge.unitPrice * edge.terrainFactor;
    
    // 3. Phí thiết bị (modem, quang...) lấy từ Edge
    breakdown.equipment = edge.equipmentCost;
    
    // 4. Phí bảo trì lấy từ Edge
    breakdown.maintenance = edge.maintenanceCost;
    
    // 5. Cộng dồn vào tổng
    breakdown.total = breakdown.install + breakdown.material + breakdown.equipment + breakdown.maintenance;
    
    return breakdown;
}
```
*Giải thích dòng code:*
- **Dòng 8-16:** Rất quan trọng! Yêu cầu là nếu cáp đứt thì không đi được. Nếu không có đoạn `if (!edge.isUp)` này, thuật toán Kruskal vẫn lầm tưởng cáp hỏng là cáp có thể mua được, gây sập mạng ảo. Trả về `Config::INF` (vô cùng lớn) để giá tiền cực đắt, Kruskal tự động vứt cáp này đi.
- **Dòng 19-32:** Từng phép nhân, cộng đều đi đúng công thức đề bài. Cố tình sử dụng `Config::INSTALLATION_COST` để tránh lỗi hard-code (gõ cứng một con số cụ thể vào mã nguồn).

---

## 2. Task B2.3 – Ràng buộc Vật lý (Validation)

**Mục đích:** Lọc bỏ các cạnh vượt ngưỡng chịu đựng của môi trường vật lý (như cáp quá dài mất tín hiệu, hoặc Nút mạng hết lỗ cắm).

### 📄 File 3: Cập nhật trong `include/Validation.h` và `src/Validation.cpp`

```cpp
bool checkMaxLength(const Edge& edge) {
    // Trả về true nếu chiều dài cáp NHỎ HƠN HOẶC BẰNG sức chịu đựng tối đa
    return edge.length <= edge.maxSegmentLength;
}

bool checkPortCapacity(const Node& node) {
    // Trả về true nếu số cổng ĐÃ DÙNG vẫn còn NHỎ HƠN TỔNG SỐ CỔNG thiết kế
    return node.usedPorts < node.totalPorts;
}

bool isStaticFeasible(const Edge& edge) {
    // Hàm gom nhóm các điều kiện tĩnh. Hiện tại gọi checkMaxLength
    return checkMaxLength(edge);
}
```
*Giải thích dòng code:*
- **`checkMaxLength` (Ràng buộc Tĩnh):** Kiểm tra cáp có vượt chiều dài cho phép không. Là "tĩnh" vì dây cáp khi sinh ra đã dài như thế, mãi mãi không đổi.
- **`checkPortCapacity` (Ràng buộc Động):** Kiểm tra `usedPorts < totalPorts`. Là "động" vì `usedPorts` ban đầu bằng 0, nhưng mỗi lần Kruskal cắm một đầu dây vào Router thì lỗ cắm sẽ tăng lên 1 (`usedPorts++`). Phải check liên tục trong vòng lặp.

---

## 3. Task B2.4 – In Báo cáo MST (Reporter)

**Mục đích:** Thu thập số liệu sau khi đồ thị đã được quy hoạch, in ra bảng báo cáo chi tiết để so sánh tiền bạc.

### 📄 File 4: `include/Reporter.h`
```cpp
#ifndef REPORTER_H
#define REPORTER_H
#include "Graph.h"

namespace Reporter {
    void printMSTReport(const Graph& graph);
}
#endif // REPORTER_H
```

### 📄 File 5: `src/Reporter.cpp`

```cpp
#include "Reporter.h"
#include "CostModel.h"
#include "Config.h"
#include <iostream>
#include <iomanip>
#include <string>

namespace Reporter {

void printMSTReport(const Graph& graph) {
    std::cout << "==============================================================\n";
    // ... code std::setw() để vẽ bảng thẳng cột
    
    double totalBuildAll = 0.0;
    double totalMST = 0.0;
    double totalMSTBackup = 0.0;

    std::vector<int> edgeIds = graph.edgeIds();
    for (int id : edgeIds) {
        const Edge& edge = graph.getEdge(id);
        if (!edge.isUp) continue; // Bỏ qua cáp đứt

        double cost = CostModel::calculateCablingCost(edge).total;
        
        // Build All: cộng tất cả cạnh có thể đi được
        if (cost != Config::INF) totalBuildAll += cost;

        std::string role = "None";
        // Ưu tiên kiểm tra cờ isBackup trước cờ isBuilt
        if (edge.isBackup) {
            role = "Backup";
            totalMSTBackup += cost;
        } else if (edge.isBuilt) {
            role = "MST";
            totalMST += cost;
            totalMSTBackup += cost;
        }

        // Lệnh in từng dòng bảng (lược bỏ code rườm rà)
    }

    std::cout << "1. Phuong an Xay tat ca cap (Build All)  : " << totalBuildAll << " DV\n";
    std::cout << "2. Phuong an Toi gian (Chi xay MST)      : " << totalMST << " DV\n";
    std::cout << "3. Phuong an An toan (MST + Backup)      : " << totalMSTBackup << " DV\n";
}
}
```
*Giải thích dòng code:*
- **Dòng 23-26:** Gọi `calculateCablingCost` của bài B2.1 để tái sử dụng mã nguồn. Nếu cáp rẻ hơn INF thì tính tiền vào phương án 1 (Build All).
- **Dòng 30-38:** Nếu bạn đảo `if (edge.isBuilt)` lên trước, thuật toán sẽ bị tính sai tiền. Tại sao? Vì cáp Backup được Quyến thiết kế bật CẢ HAI CỜ (`isBuilt` và `isBackup` đều = true). Do đó phải check chữ `isBackup` trước, để nếu nó true thì chạy nhánh Backup luôn mà không lọt vào MST.
- Dòng cuối: In ra 3 đáp số, dùng để đối chiếu với Bảng 6.1 trong file Word của Giáo viên.

---

## 4. Kiểm tra chiều sâu (Deep Understanding)

1️⃣ Đầu vào, đầu ra và vị trí trong quy trình?
- **Vị trí:** Nằm ở Bước 2 và Bước 3 trong quy trình phát triển. Chạy ngay sau khi Bước 1 (Đọc file và Khởi tạo Graph) hoàn thành.
- **Đầu vào:** Một Đồ thị (`Graph`) với danh sách các Nút và Cạnh chưa được tính toán tiền bạc. Đối với `Reporter`, đầu vào là Đồ thị đã chạy qua Kruskal (các cờ `isBuilt` đã được bật).
- **Đầu ra:** 
  - Với `CostModel`: Hàm trả về cụ thể số Đơn Vị (ĐV) tiền tệ bóc tách 4 mục rạch ròi.
  - Với `Validation`: Hàm trả về `true/false` cho việc cắm cáp.
  - Với `Reporter`: Một bảng thống kê trên màn hình Console so sánh số tiền của 3 phương án quy hoạch, xác nhận đáp số 6544 và 12440.

2️⃣ Công thức nào được dùng và vì sao có mặt trong bài?
- **Công thức tính tiền cáp:** $\text{Total} = \text{Install} + \text{Length} \times \text{UnitPrice} \times \text{Terrain} + \text{Equip} + \text{Maintain}$
- **Công thức vật lý tĩnh:** $L \le L_{max}$
- **Công thức vật lý động:** $P_{used} \le P_{total}$
- **Vì sao có mặt:** Việc tạo mạng lưới (Kruskal) không đơn thuần chỉ là kết nối bằng mọi giá. Giống như ngoài đời thật, bạn không thể cắm 5 sợi dây cáp vào một cục Modem chỉ có 4 lỗ cắm ($P_{used} > P_{total}$), cũng không thể kéo cáp quang qua đại dương nếu tín hiệu suy hao ở chiều dài đó ($L > L_{max}$). Công thức Cost thì đảm bảo dự án chọn con đường rẻ tiền nhất, tối ưu ngân sách nhà nước.

3️⃣ Nếu đổi một tham số thì kết quả thay đổi thế nào và vì sao?
- **Kịch bản:** Sửa trong hàm `calculateCablingCost`, khi cáp đứt (`!isUp`) thay vì trả về `Config::INF` ta lại trả về giá là `0.0`.
- **Kết quả thay đổi:** Thuật toán Kruskal sẽ lao vào nhặt toàn bộ các đoạn cáp bị đứt để ráp vào mạng lưới. Tổng chi phí MST báo cáo ra màn hình sẽ giảm xuống rất mạnh, thậm chí = 0.
- **Vì sao:** Vì bản chất thuật toán Kruskal của Quyến là "Lòng tham" (Greedy). Nó tìm mọi cáp có GIÁ TRỊ NHỎ NHẤT (Rẻ nhất) để nhặt trước. Khi bạn set giá đứt cáp = 0.0, Kruskal sẽ ưu tiên chọn nó đầu tiên, dẫn đến mạng bị đứt hàng loạt nhưng máy tính lại tưởng đã lắp xong mạng miễn phí! Do đó, cáp đứt phải được gán giá = `INF` (vô cực) để đánh lừa lòng tham của Kruskal, ép nó tránh xa cáp hỏng ra.
