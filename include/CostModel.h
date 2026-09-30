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

// Cấu trúc lưu trạng thái bảng giá (Tương thích ngược qua khởi tạo mặc định)
struct PricingConfig {
    double installationCost = 0.0;
    std::map<MediaType, double> unitPrice = {
        {MediaType::COPPER, 1.0}, 
        {MediaType::FIBER, 5.0}, 
        {MediaType::WIRELESS, 0.0}
    };
    std::map<MediaType, double> equipmentCost = {
        {MediaType::COPPER, 50.0}, 
        {MediaType::FIBER, 50.0}, 
        {MediaType::WIRELESS, 80.0}
    };
    std::map<MediaType, double> maintenanceCost = {
        {MediaType::COPPER, 30.0}, 
        {MediaType::FIBER, 48.0}, 
        {MediaType::WIRELESS, 20.0}
    };
};

class CostModel {
private:
    PricingConfig config; // Thuộc tính private: Đảm bảo đóng gói tuyệt đối

    // Hàm phụ trợ chuyển chuỗi thành Enum an toàn
    MediaType stringToMediaType(const std::string& str) const;

public:
    // Constructor mặc định (sẽ dùng cấu hình mặc định trong PricingConfig)
    CostModel() = default;

    // Nạp cấu hình từ file text
    bool loadPricing(const std::string& filepath);

    // Tính toán chi phí - Hàm const vì nó không làm thay đổi trạng thái của model
    CostBreakdown calculateCablingCost(const Edge& edge) const;
};

#endif 
