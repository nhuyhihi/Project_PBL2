#ifndef CONFIG_H
#define CONFIG_H

#include <limits> 

namespace Config {
    // Hằng số mạng và tải trọng
    const double MAX_LOAD = 1.0;
    const double DEFAULT_TERRAIN_FACTOR = 1.0;
    const double LOAD_WINDOW_SECONDS = 1.0;
    const double LOAD_DECAY_RATE = 0.99;

    // Hằng số thuật toán và vật lý truyền dẫn
    const double INF = std::numeric_limits<double>::infinity();
    const double SPEED_OF_LIGHT_FIBER = 2.0e8; // m/s (Cáp quang/đồng)
    const double SPEED_OF_LIGHT_AIR = 3.0e8;   // m/s (Không dây)

    // Bộ trọng số QoS cho hàm chi phí định tuyến
    struct QoSWeight {
        double wTrans;
        double wProp;
        double wQueue;
    };

    const QoSWeight QOS_WEIGHT_REAL_TIME = {1.0, 1.0, 4.0}; // Ưu tiên né nghẽn hàng đợi
    const QoSWeight QOS_WEIGHT_BULK_DATA = {2.0, 1.0, 2.0}; // Ưu tiên băng thông và tải
    const QoSWeight QOS_WEIGHT_CONTROL   = {1.0, 1.0, 1.0}; // Cân bằng các thành phần

    // Phạt thêm (giây) cho mỗi mảnh vượt quá 1 khi phải phân mảnh
    const double FRAGMENT_PENALTY_PER_EXTRA = 0.0003;
}

#endif // CONFIG_H
