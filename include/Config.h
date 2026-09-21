#ifndef CONFIG_H
#define CONFIG_H

#include <limits> 

namespace Config {
    // 1. Hằng số Mạng cơ bản
    const double MAX_LOAD = 1.0;               // Tải tối đa (100%)
    const double DEFAULT_TERRAIN_FACTOR = 1.0; // Hệ số địa hình mặc định
    
    // 2. Hằng số Thuật toán (Cho bạn Quyến dùng ở Tuần 2 & 3)
    const double INF = std::numeric_limits<double>::infinity(); // Đại diện cho đứt cáp/không liên thông

    // 3. Hằng số Chi phí (Cho bạn Ý dùng ở module CostModel)
    const double INSTALLATION_COST = 0.0; // Phí nhân công kéo 1 dây cáp (hiện tại = 0)
}

#endif // CONFIG_H

