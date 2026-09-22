# BÁO CÁO VÀ GIẢI THÍCH CHI TIẾT TẤT CẢ CÁC FILE TUẦN 3 (Ý)

Tài liệu này tổng hợp toàn bộ các file, cấu trúc code, ý nghĩa từng dòng và mục đích của các Task đã thực hiện trong **Tuần 3** (Phần tính toán thời gian trễ của mạng). Dù ở yêu cầu trước bạn có ghi nhầm là Tuần 4, nhưng những task tính Delay, MTU, TCP này đều thuộc Tuần 3 nhé! Cuối bài vẫn sẽ có phần trả lời 3 câu hỏi để kiểm tra hiểu sâu bản chất vấn đề.

---

## 1. File Khai báo Cấu trúc (Header)

**Mục đích:** Khai báo các hàm toán học để tính độ trễ vật lý và giao thức của mạng. Quyết định gom chung tất cả vào một file giúp quản lý mã nguồn siêu gọn gàng.

### 📄 `include/DelayModel.h`

```cpp
#ifndef DELAYMODEL_H
#define DELAYMODEL_H

#include "Edge.h"
#include "Node.h"
#include "Packet.h"

namespace DelayModel {

    // Tính thời gian nhét bit vào cáp
    double calcTransmissionDelay(int sizeBytes, double bandwidthMbps);

    // Tính thời gian sóng/ánh sáng lan truyền trong cáp
    double calcPropagationDelay(double length, double propagationSpeed);

    // Tính thời gian CPU Router xử lý
    double calcProcessingDelay(const Node& nextNode);

    // Tính thời gian nằm trong hàng đợi
    double calcQueueDelay(double baseQueueDelay, double currentLoad);

    // Tính tổng 4 độ trễ vật lý trên
    double calcBaseDelay(const Packet& packet, const Edge& edge, const Node& nextNode);

    // Tính số mảnh cần băm dựa vào MTU
    int calcFragmentCount(int sizeBytes, int mtu, bool canFragment);

    // Tính phụ phí thời gian cho giao thức TCP (Bắt tay 3 bước)
    double calcProtocolOverhead(const Packet& packet, double oneWayDelay);

    // Hàm TỔNG HỢP: Gọi từ ngoài vào để lấy ngay độ trễ tổng cộng
    double calculateTotalDelay(const Packet& packet, const Edge& edge, const Node& nextNode);
    
}
#endif // DELAYMODEL_H
```
*Giải thích code:* Khối mã này sử dụng `namespace DelayModel` thay vì `class` vì ở đây ta chỉ chứa toàn hàm toán học thuần túy, không cần lưu trữ dữ liệu (trạng thái đối tượng). Thiết kế này giúp hệ thống chạy nhanh hơn.

---

## 2. File Triển khai Logic (Source)

**Mục đích:** Chứa toàn bộ các công thức toán học và logic băm dữ liệu, tính toán RTT.

### 📄 `src/DelayModel.cpp`

```cpp
#include "DelayModel.h"
#include "Config.h"

namespace DelayModel {

    // [1] TRUYỀN TẢI
    double calcTransmissionDelay(int sizeBytes, double bandwidthMbps) {
        if (bandwidthMbps <= 0) return Config::INF;
        // Đổi byte ra bit (*8), đổi Mbps ra bps (*1,000,000)
        return (sizeBytes * 8.0) / (bandwidthMbps * 1000000.0);
    }

    // [2] LAN TRUYỀN
    double calcPropagationDelay(double length, double propagationSpeed) {
        if (propagationSpeed <= 0) return 0.0;
        return length / propagationSpeed; // Mét chia (Mét/giây) = Giây
    }

    // [3] XỬ LÝ (Processing)
    double calcProcessingDelay(const Node& nextNode) {
        return nextNode.processingTime;
    }

    // [4] HÀNG ĐỢI (Queue)
    static double congestionPenalty(double load) {
        // Mô phỏng hàm phạt tắc nghẽn (sẽ làm kỹ ở Tuần 4)
        if (load <= 0.80) return 1.0;
        if (load <= 0.85) return 1.56;
        if (load <= 0.90) return 3.25;
        if (load <= 0.95) return 6.06;
        return 10.00;
    }
    double calcQueueDelay(double baseQueueDelay, double currentLoad) {
        return baseQueueDelay * congestionPenalty(currentLoad);
    }

    // [5] BASE DELAY (Trễ vật lý cơ sở)
    double calcBaseDelay(const Packet& packet, const Edge& edge, const Node& nextNode) {
        if (!edge.isUp) return Config::INF; // Nếu cáp đứt thì rớt luôn
        // ... Gọi 4 hàm trên cộng lại
    }

    // [6] PHÂN MẢNH (Fragmentation)
    int calcFragmentCount(int sizeBytes, int mtu, bool canFragment) {
        if (sizeBytes <= mtu) return 1; // Nhỏ hơn thì không cần băm
        if (!canFragment) return 0;     // To hơn mà bị cấm băm -> Trả về 0 (Rớt gói)
        
        int payload = mtu - 20;         // Trừ đi 20 byte Header của IP
        if (payload <= 0) return 0; 
        
        int n = sizeBytes / payload;
        if (sizeBytes % payload != 0) n++; // Làm tròn lên (Ví dụ chia ra 2.1 thì lấy 3)
        return n;
    }

    // [7] GIAO THỨC (Protocol Overhead)
    double calcProtocolOverhead(const Packet& packet, double oneWayDelay) {
        if (packet.protocol == Protocol::TCP && packet.newConnection) {
            // RTT (Round Trip Time) = 2 lần One-way Delay
            return 2.0 * oneWayDelay; 
        }
        return 0.0; // UDP hoặc kết nối TCP đã có sẵn thì phí = 0
    }

    // [8] TÍNH TỔNG (Integration)
    double calculateTotalDelay(const Packet& packet, const Edge& edge, const Node& nextNode) {
        if (!edge.isUp) return Config::INF;

        int fragments = calcFragmentCount(packet.sizeBytes, edge.mtu, packet.canFragment);
        if (fragments == 0) return Config::INF; // Cấm phân mảnh -> Đứt

        // ... Tính toán Base
        // Transmission bị nhân lên theo số lượng mảnh
        double oneWayDelay = (dTrans * fragments) + dProp + dProc + dQueue;

        double overhead = calcProtocolOverhead(packet, oneWayDelay);
        return oneWayDelay + overhead; // Trả về con số cuối cùng
    }
}
```
*Giải thích dòng code:*
- **Phân mảnh:** Công thức `mtu - 20` là vì mỗi mảnh bị cắt ra đều phải được dán thêm một cái "Nhãn dán" (Header IP) nặng 20 byte để biết đường đi. Nên số byte thực chứa dữ liệu (payload) bị hụt đi 20.
- **TCP Overhead:** Ở lần đầu tiên nói chuyện, TCP bắt buộc phải đi một vòng (Ping - Pong) để chào hỏi (gọi là Bắt tay 3 bước). Một vòng Ping-Pong chính là 2 chiều (`2.0 * oneWayDelay`).
- **Tổng hợp (`calculateTotalDelay`):** Nhờ có hàm này, ở bên thuật toán Dijkstra, người ta không cần biết bạn đang tính TCP, MTU hay gì cả. Cứ ném Packet vào là ra thời gian. Kiến trúc này gọi là "Trừu tượng hóa" (Abstraction).

---

## 3. Kiểm tra chiều sâu (Deep Understanding)

1️⃣ **Đầu vào, đầu ra và vị trí trong 11 bước?**
- **Vị trí:** Nó nằm ở Bước tìm đường định tuyến, lúc thuật toán Dijkstra của Quyến đang chạy tìm đường ngắn nhất.
- **Đầu vào:** Một gói tin (chứa kích thước, giao thức), Một cáp mạng (chứa độ dài, MTU, băng thông), và Một Nút đích.
- **Đầu ra:** Trả về một con số thập phân duy nhất biểu diễn TỔNG SỐ GIÂY (s) mà gói tin này cần để vượt qua đoạn dây cáp đó. (Hoặc trả về `INF` nếu rớt).

2️⃣ **Công thức nào được dùng và vì sao có mặt trong bài?**
- **Công thức: `Số mảnh = ceil(sizeBytes / (MTU - 20))`** 
  - *Vì sao có mặt:* Dây mạng (đặc biệt là cáp đồng Ethernet) có ống truyền rất nhỏ (thường 1500 byte). Bạn không thể nhét một bộ phim 3GB vào một lượt được, mà phải băm nhỏ thành hàng triệu mảnh, gọi là Fragmentation.
- **Công thức: `Thời gian truyền tải = Số lượng mảnh * dTrans`**
  - *Vì sao có mặt:* Việc băm nhỏ làm lãng phí Header (cứ mỗi mảnh dư 20 byte) và Router phải đẩy nhiều gói tin vào dây cáp hơn. Do đó truyền tải bị kéo dài ra so với lý thuyết.
- **Công thức: `TCP Overhead = 2 * One Way Delay`**
  - *Vì sao có mặt:* Giao thức UDP cứ thế gửi đại, nhưng TCP thì đảm bảo 100% không mất dữ liệu. Sự an toàn này phải trả giá bằng việc lần đầu gửi, hai máy tính phải mất thời gian chào hỏi nhau từ xa, tốn đúng 1 vòng tròn đi về (Round Trip Time).

3️⃣ **Nếu đổi một tham số thì kết quả thay đổi thế nào và vì sao?**
- **Kịch bản:** Khi truyền qua cáp có MTU = 1500, tôi sửa gói tin `sizeBytes = 3000` thành cờ `canFragment = false` (Cấm phân mảnh - Don't Fragment).
- **Kết quả thay đổi:** Hàm `calcFragmentCount` lập tức bắt được cờ này và trả về 0. Sau đó hàm tổng `calculateTotalDelay` thấy số lượng mảnh bằng 0 sẽ thẳng tay ném gói tin đi và trả về độ trễ là vô cực (`INF`).
- **Vì sao:** Trong mạng thực tế, đôi khi người ta muốn đo xem dây mạng có đủ to để chuyển thẳng 1 gói bự không. Họ cố tình cấm phân mảnh. Nếu dây cáp quá hẹp (MTU nhỏ), thiết bị mạng Router sẽ vứt ngay gói tin đó vào sọt rác và gửi lại lỗi "Fragmentation Needed" (ICMP error). Gán `INF` chính là cách ta mô phỏng việc gói tin bị ném vào sọt rác trong đồ án này!
