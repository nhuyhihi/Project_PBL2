#include "CostModel.h"
#include "Config.h"

CostBreakdown CostModel::calculateCablingCost(const Edge& edge) {
    CostBreakdown breakdown;
    
    // Ràng buộc: "dùng INF cho cạnh không đi được"
    // Nếu trạng thái của cáp là đang bị đứt (isUp = false),
    // chi phí đi qua cáp này sẽ là vô cùng lớn (INF).
    if (!edge.isUp) {
        breakdown.install = 0.0;
        breakdown.material = 0.0;
        breakdown.equipment = 0.0;
        breakdown.maintenance = 0.0;
        breakdown.total = Config::INF;
        return breakdown;
    }

    // 1. Chi phí lắp đặt cơ bản (installationCost) cho mỗi đường cáp
    breakdown.install = Config::INSTALLATION_COST;
    
    // 2. Tính chi phí vật tư: Dài (m) * Đơn giá (ĐV/m) * Hệ số địa hình
    breakdown.material = edge.length * edge.unitPrice * edge.terrainFactor;
    
    // 3. Lấy trực tiếp chi phí thiết bị từ Edge
    breakdown.equipment = edge.equipmentCost;
    
    // 4. Lấy trực tiếp chi phí bảo trì từ Edge
    breakdown.maintenance = edge.maintenanceCost;
    
    // 5. Tổng tất cả các chi phí tạo nên Cabling Cost
    breakdown.total = breakdown.install + breakdown.material + breakdown.equipment + breakdown.maintenance;
    
    return breakdown;
}
