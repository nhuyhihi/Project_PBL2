#ifndef CONFIG_H
#define CONFIG_H

namespace Config {
    // Thời gian tính theo second (giây)
    // Dung lượng tính theo byte
    // Băng thông tính theo Mbps
    // Khoảng cách tính theo m (mét)

    const double MAX_LOAD = 1.0;
    const double DEFAULT_TERRAIN_FACTOR = 1.0;
    
    // Ngưỡng penalty cho congestion (Tùy biến ở Tuần 4)
    // 0.80 -> 1.00
    // 0.85 -> 1.56
    // 0.90 -> 3.25
    // 0.95 -> 6.06
    // 1.00 -> 10.00
}

#endif // CONFIG_H
