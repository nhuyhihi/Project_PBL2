#include "CostModel.h"
#include "Config.h"
#include "EnumUtils.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>

bool CostModel::loadPricing(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "[CostModel] Loi: Khong the mo file bang gia " << filepath << ".\n";
        return false;
    }

    std::string line;
    int lineCount = 0;
    while (std::getline(file, line)) {
        lineCount++;
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::stringstream ss(line);
        std::string key;
        ss >> key;

        if (key == "INSTALLATION_COST") {
            if (!(ss >> config.installationCost)) {
                std::cerr << "[CostModel] Canh bao dong " << lineCount << ": Sai format INSTALLATION_COST trong " << filepath << ".\n";
            }
        } 
        else if (key == "UNIT_PRICE" || key == "EQUIPMENT_COST" || key == "MAINTENANCE_COST") {
            std::string typeStr;
            double val;
            if (ss >> typeStr >> val) {
                MediaType media;
                if (!EnumUtils::parseMediaType(typeStr, media)) {
                    std::cerr << "[CostModel] Canh bao dong " << lineCount 
                              << ": Loai cap khong hop le '" << typeStr 
                              << "' trong " << filepath << ". Bo qua dong nay.\n";
                    continue;
                }
                if (key == "UNIT_PRICE") {
                    config.unitPrice[media] = val;
                } else if (key == "EQUIPMENT_COST") {
                    config.equipmentCost[media] = val;
                } else {
                    config.maintenanceCost[media] = val;
                }
            } else {
                std::cerr << "[CostModel] Canh bao dong " << lineCount 
                          << ": Sai format " << key << " trong " << filepath << ". Bo qua dong nay.\n";
            }
        }
    }
    std::cout << "[CostModel] Nhan cau hinh gia thanh cong tu: " << filepath << "\n";
    return true;
}

CostBreakdown CostModel::calculateCablingCost(const Edge& edge) const {
    CostBreakdown breakdown;
    
    // Ràng buộc: "dùng INF cho cạnh không đi được"
    if (!edge.isUp) {
        breakdown.install = breakdown.material = breakdown.equipment = breakdown.maintenance = 0.0;
        breakdown.total = Config::INF;
        return breakdown;
    }

    // 1. Kiểm tra loại cáp đã được định giá trong pricing.txt chưa
    if (config.unitPrice.count(edge.mediaType) == 0 ||
        config.equipmentCost.count(edge.mediaType) == 0 ||
        config.maintenanceCost.count(edge.mediaType) == 0) {
        breakdown.install = breakdown.material = breakdown.equipment = breakdown.maintenance = 0.0;
        breakdown.total = Config::INF;
        return breakdown;
    }

    // 2. Chi phí lắp đặt cơ bản
    breakdown.install = config.installationCost;
    
    // 3. Chi phí vật tư cáp = Chiều dài * Đơn giá từ pricing.txt * Hệ số địa hình
    double currentUnitPrice = config.unitPrice.at(edge.mediaType);
    breakdown.material = edge.length * currentUnitPrice * edge.terrainFactor;
    
    // 4. Phí thiết bị cổng từ pricing.txt
    breakdown.equipment = config.equipmentCost.at(edge.mediaType);
    
    // 5. Phí bảo trì định kỳ từ pricing.txt
    breakdown.maintenance = config.maintenanceCost.at(edge.mediaType);
    
    // 6. Tổng tất cả các chi phí
    breakdown.total = breakdown.install + breakdown.material + breakdown.equipment + breakdown.maintenance;
    
    return breakdown;
}
