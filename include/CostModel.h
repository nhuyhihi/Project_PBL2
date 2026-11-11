#ifndef COSTMODEL_H
#define COSTMODEL_H

#include "Edge.h"

#include <map>
#include <string>

struct CostBreakdown {
    double install;
    double material;
    double equipment;
    double maintenance;
    double total;
};

// Cấu hình đơn giá nạp từ pricing.txt
struct PricingConfig {
    double installationCost = 0.0;
    std::map<MediaType, double> unitPrice;
    std::map<MediaType, double> equipmentCost;
    std::map<MediaType, double> maintenanceCost;
};

// Mô hình tính chi phí lắp đặt tuyến cáp
class CostModel {
private:
    PricingConfig config;
public:
    CostModel() = default;

    bool loadPricing(const std::string& filepath);
    CostBreakdown calculateCablingCost(const Edge& edge) const;
};

#endif
