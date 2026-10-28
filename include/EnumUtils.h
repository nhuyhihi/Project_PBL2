#ifndef ENUMUTILS_H
#define ENUMUTILS_H

#include "Types.h"
#include <string>

namespace EnumUtils {
    // Chuyển đổi Enum sang chuỗi văn bản (dùng cho in ấn báo cáo / log)
    const char* toString(NodeType type);
    const char* toString(MediaType media);
    const char* toString(Protocol proto);
    const char* toString(PacketType type);
    const char* toString(EventType event);

    // Chuyển đổi chuỗi văn bản sang Enum (trả về true nếu hợp lệ, false nếu chuỗi lạ)
    bool parseNodeType(const std::string& str, NodeType& out);
    bool parseMediaType(const std::string& str, MediaType& out);
    bool parseProtocol(const std::string& str, Protocol& out);
    bool parsePacketType(const std::string& str, PacketType& out);
    bool parseEventType(const std::string& str, EventType& out);
}

#endif // ENUMUTILS_H
