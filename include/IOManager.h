#ifndef IOMANAGER_H
#define IOMANAGER_H

#include <string>
#include <vector>
#include "Graph.h"
#include "Packet.h"
#include "Scenario.h"
#include "Types.h"

// Đọc và kiểm tra dữ liệu đầu vào từ các tệp văn bản
class IOManager {
public:
    static bool loadNodes(const std::string& filepath, Graph& graph);
    static bool loadEdges(const std::string& filepath, Graph& graph);
    static bool loadPackets(const std::string& filepath, std::vector<Packet>& packets, const Graph* graph = nullptr);
    static bool loadScenario(const std::string& filepath, Scenario& scenario);
    static bool isSkippableLine(const std::string& line);
};

#endif 
