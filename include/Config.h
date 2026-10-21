#ifndef CONFIG_H
#define CONFIG_H

#include <limits> 

namespace Config {
    // 1. Hằng số Mạng cơ bản
    const double MAX_LOAD = 1.0;               // Tải tối đa
    const double DEFAULT_TERRAIN_FACTOR = 1.0; // Hệ số địa hình mặc định

    // 2. Hằng số Thuật toán 
    const double INF = std::numeric_limits<double>::infinity(); // Đại diện cho đứt cáp/không liên thông

    // 3. Hằng số Vật lý mạng 
    const double SPEED_OF_LIGHT_FIBER = 2.0e8; // m/s (Trong cáp quang/cáp đồng)
    const double SPEED_OF_LIGHT_AIR = 3.0e8;   // m/s (Sóng Wireless)

    // 5. Trọng số QoS cho Dynamic Routing Cost (Tuần 4, Task A4.2)
    // Dùng trong RoutingCost::calculateRoutingCost() để Dijkstra chọn đường
    // phù hợp với từng loại gói tin. KHÔNG hard-code các số này trong .cpp.
    struct QoSWeight {
        double wTrans;   // trọng số độ trễ truyền tải (liên quan băng thông)
        double wProp;    // trọng số độ trễ lan truyền (liên quan khoảng cách)
        double wQueue;   // trọng số độ trễ hàng đợi (liên quan tắc nghẽn)
    };

    // REAL_TIME: rất nhạy với độ trễ hàng đợi/tắc nghẽn (jitter khó chịu),
    // nên wQueue cao nhất -> sẵn sàng đi đường xa hơn để né nghẽn.
    const QoSWeight QOS_WEIGHT_REAL_TIME = {1.0, 1.0, 4.0};

    // BULK_DATA: chú trọng băng thông (wTrans cao hơn) và vẫn né tắc nghẽn
    // nặng, nhưng không nhạy cảm bằng REAL_TIME.
    const QoSWeight QOS_WEIGHT_BULK_DATA = {2.0, 1.0, 2.0};

    // CONTROL: cân bằng, không nghiêng hẳn về phía nào.
    const QoSWeight QOS_WEIGHT_CONTROL = {1.0, 1.0, 1.0};

    // Phạt thêm (giây) cho MỖI mảnh vượt quá 1 khi gói tin phải phân mảnh,
    // để Dijkstra ưu tiên đường có MTU đủ lớn, tránh phải băm nhỏ gói tin.
    const double FRAGMENT_PENALTY_PER_EXTRA = 0.0003;
}

#endif // CONFIG_H
