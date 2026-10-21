#ifndef IOMANAGER_H
#define IOMANAGER_H

#include <string>
#include <vector>
#include "Graph.h"
#include "Packet.h"
#include "Scenario.h"
#include "Types.h"

class IOManager {
public:
    // Đọc danh sách Node từ file
    static bool loadNodes(const std::string& filepath, Graph& graph);
    
    // Đọc danh sách Edge từ file
    static bool loadEdges(const std::string& filepath, Graph& graph);
    
    // Đọc danh sách Packet từ file (có thể truyền Graph để kiểm tra node nguồn/đích)
    static bool loadPackets(const std::string& filepath, std::vector<Packet>& packets, const Graph* graph = nullptr);
    
    // Đọc kịch bản sự kiện (Scenario) từ file (Task B4.3, B4.7)
    static bool loadScenario(const std::string& filepath, Scenario& scenario);
    
    // Kiểm tra xem dòng có nên bỏ qua hay không (dòng trống hoặc bắt đầu bằng '#')
    static bool isSkippableLine(const std::string& line);
};

#endif 
