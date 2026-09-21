#ifndef COSTMODEL_H
#define COSTMODEL_H

#include "Edge.h"

// Struct chứa chi tiết cấu thành nên tổng chi phí cáp
// Tất cả các đơn vị tiền tệ đều cùng một hệ quy chiếu
struct CostBreakdown {
    double install;       // Chi phí lắp đặt (nhân công)
    double material;      // Chi phí vật tư (chiều dài * đơn giá * hệ số địa hình)
    double equipment;     // Chi phí thiết bị 2 đầu cáp
    double maintenance;   // Chi phí bảo trì
    double total;         // Tổng cộng
};

class CostModel {
public:
    // Tính toán và bóc tách chi phí cáp cho một đường truyền cụ thể
    // Đầu vào: Một đối tượng Edge mang đầy đủ thông số
    // Đầu ra: Struct CostBreakdown ghi rõ từng loại chi phí
    static CostBreakdown calculateCablingCost(const Edge& edge);
};

#endif // COSTMODEL_H
