#ifndef IOMANAGER_H
#define IOMANAGER_H

#include <string>
#include <vector>
#include "Graph.h"
#include "Packet.h"
#include "Types.h"

class IOManager {
public:
    // Đọc danh sách Node từ file
    static bool loadNodes(const std::string& filepath, Graph& graph);
    
    // Đọc danh sách Edge từ file
    static bool loadEdges(const std::string& filepath, Graph& graph);
    
    // Đọc danh sách Packet từ file
    static bool loadPackets(const std::string& filepath, std::vector<Packet>& packets);
    
    // Kiểm tra xem dòng có nên bỏ qua hay không (dòng trống hoặc bắt đầu bằng '#')
    static bool isSkippableLine(const std::string& line);
    
    // Chuyển đổi String -> Enum
    static NodeType stringToNodeType(const std::string& str);
    static MediaType stringToMediaType(const std::string& str);
    static Protocol stringToProtocol(const std::string& str);
    static PacketType stringToPacketType(const std::string& str);
    static EventType stringToEventType(const std::string& str);
};

#endif 
