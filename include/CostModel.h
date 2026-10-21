#ifndef COSTMODEL_H
#define COSTMODEL_H

#include "Edge.h"

#include <map>
#include <string>

struct CostBreakdown {
    double install;     
    double material;      // Chi phí vật tư (chiều dài * đơn giá * hệ số địa hình)
    double equipment;     
    double maintenance;   
    double total;         
};

// Cấu trúc lưu trạng thái bảng giá nạp từ pricing.txt (khởi tạo rỗng đồng bộ)
struct PricingConfig {
    double installationCost = 0.0;
    std::map<MediaType, double> unitPrice;
    std::map<MediaType, double> equipmentCost;
    std::map<MediaType, double> maintenanceCost;
};

class CostModel {
private:
    PricingConfig config; // Thuộc tính private: Đảm bảo đóng gói tuyệt đối
public:
    // Constructor mặc định (sẽ dùng cấu hình mặc định trong PricingConfig)
    CostModel() = default;

    // Nạp cấu hình từ file text
    bool loadPricing(const std::string& filepath);

    // Tính toán chi phí - Hàm const vì nó không làm thay đổi trạng thái của model
    CostBreakdown calculateCablingCost(const Edge& edge) const;
};

#endif 
