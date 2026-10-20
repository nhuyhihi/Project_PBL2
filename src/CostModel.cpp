#include "CostModel.h"
#include "Config.h"
#include "EnumUtils.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>

MediaType CostModel::stringToMediaType(const std::string& str) const {
    MediaType media;
    if (EnumUtils::parseMediaType(str, media)) {
        return media;
    }
    throw std::invalid_argument("Unknown MediaType: " + str);
}

bool CostModel::loadPricing(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "[CostModel] Canh bao: Khong the mo " << filepath << ". Su dung gia mac dinh.\n";
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

    // 1. Chi phí lắp đặt cơ bản
    breakdown.install = config.installationCost;
    
    // 2. Tính chi phí vật tư: Tra cứu trong map dựa vào loại cáp.
    // Fallback: Nếu không có trong map thì dùng edge.unitPrice
    double currentUnitPrice = config.unitPrice.count(edge.mediaType) 
                              ? config.unitPrice.at(edge.mediaType) 
                              : edge.unitPrice;
    breakdown.material = edge.length * currentUnitPrice * edge.terrainFactor;
    
    // 3. Phí thiết bị
    double currentEquipCost = config.equipmentCost.count(edge.mediaType)
                              ? config.equipmentCost.at(edge.mediaType)
                              : edge.equipmentCost;
    breakdown.equipment = currentEquipCost;
    
    // 4. Phí bảo trì
    double currentMaintCost = config.maintenanceCost.count(edge.mediaType)
                              ? config.maintenanceCost.at(edge.mediaType)
                              : edge.maintenanceCost;
    breakdown.maintenance = currentMaintCost;
    
    // 5. Tổng tất cả các chi phí
    breakdown.total = breakdown.install + breakdown.material + breakdown.equipment + breakdown.maintenance;
    
    return breakdown;
}
