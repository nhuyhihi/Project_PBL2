#ifndef REPORTER_H
#define REPORTER_H

#include "Graph.h"

namespace Reporter {
    // In báo cáo chi tiết về kết quả cây khung nhỏ nhất (MST) và các cáp dự phòng.
    // So sánh chi phí tổng thể giữa: Xây tất cả, Chỉ xây MST, và Xây MST + Backup.
    void printMSTReport(const Graph& graph);
}

#endif
