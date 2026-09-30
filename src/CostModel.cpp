#include "CostModel.h"
#include "Config.h"
#include <fstream>
#include <sstream>
#include <iostream>

MediaType CostModel::stringToMediaType(const std::string& str) const {
    if (str == "FIBER") return MediaType::FIBER;
    if (str == "COPPER") return MediaType::COPPER;
    return MediaType::WIRELESS;
}

bool CostModel::loadPricing(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "[CostModel] Canh bao: Khong the mo " << filepath << ". Su dung gia mac dinh.\n";
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::stringstream ss(line);
        std::string key;
        ss >> key;

        if (key == "INSTALLATION_COST") {
            ss >> config.installationCost;
        } 
        else if (key == "UNIT_PRICE") {
            std::string typeStr;
            double val;
            if (ss >> typeStr >> val) {
                config.unitPrice[stringToMediaType(typeStr)] = val;
            }
        } 
        else if (key == "EQUIPMENT_COST") {
            std::string typeStr;
            double val;
            if (ss >> typeStr >> val) {
                config.equipmentCost[stringToMediaType(typeStr)] = val;
            }
        }
        else if (key == "MAINTENANCE_COST") {
            std::string typeStr;
            double val;
            if (ss >> typeStr >> val) {
                config.maintenanceCost[stringToMediaType(typeStr)] = val;
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
