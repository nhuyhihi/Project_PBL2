#ifndef COSTMODEL_H
#define COSTMODEL_H

#include "Edge.h"

struct CostBreakdown {
    double install;     
    double material;      // Chi phí vật tư (chiều dài * đơn giá * hệ số địa hình)
    double equipment;     
    double maintenance;   
    double total;         
};

class CostModel {
public:
    static CostBreakdown calculateCablingCost(const Edge& edge);
};

#endif 
