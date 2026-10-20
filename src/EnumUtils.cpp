#include "EnumUtils.h"

namespace EnumUtils {

const char* toString(NodeType type) {
    switch (type) {
        case NodeType::ROUTER: return "ROUTER";
        case NodeType::HOST:   return "HOST";
        case NodeType::SWITCH: return "SWITCH";
    }
    return "UNKNOWN";
}

const char* toString(MediaType media) {
    switch (media) {
        case MediaType::FIBER:    return "FIBER";
        case MediaType::COPPER:   return "COPPER";
        case MediaType::WIRELESS: return "WIRELESS";
    }
    return "UNKNOWN";
}

const char* toString(Protocol proto) {
    switch (proto) {
        case Protocol::TCP: return "TCP";
        case Protocol::UDP: return "UDP";
    }
    return "UNKNOWN";
}

const char* toString(PacketType type) {
    switch (type) {
        case PacketType::REAL_TIME: return "REAL_TIME";
        case PacketType::BULK_DATA: return "BULK_DATA";
        case PacketType::CONTROL:   return "CONTROL";
    }
    return "UNKNOWN";
}

const char* toString(EventType event) {
    switch (event) {
        case EventType::LINK_DOWN:    return "LINK_DOWN";
        case EventType::LINK_UP:      return "LINK_UP";
        case EventType::LOAD_UPDATE:  return "LOAD_UPDATE";
        case EventType::DELAY_UPDATE: return "DELAY_UPDATE";
    }
    return "UNKNOWN";
}

bool parseNodeType(const std::string& str, NodeType& out) {
    if (str == "ROUTER") { out = NodeType::ROUTER; return true; }
    if (str == "HOST")   { out = NodeType::HOST;   return true; }
    if (str == "SWITCH") { out = NodeType::SWITCH; return true; }
    return false;
}

bool parseMediaType(const std::string& str, MediaType& out) {
    if (str == "FIBER")    { out = MediaType::FIBER;    return true; }
    if (str == "COPPER")   { out = MediaType::COPPER;   return true; }
    if (str == "WIRELESS") { out = MediaType::WIRELESS; return true; }
    return false;
}

bool parseProtocol(const std::string& str, Protocol& out) {
    if (str == "TCP") { out = Protocol::TCP; return true; }
    if (str == "UDP") { out = Protocol::UDP; return true; }
    return false;
}

bool parsePacketType(const std::string& str, PacketType& out) {
    if (str == "REAL_TIME") { out = PacketType::REAL_TIME; return true; }
    if (str == "BULK_DATA") { out = PacketType::BULK_DATA; return true; }
    if (str == "CONTROL")   { out = PacketType::CONTROL;   return true; }
    return false;
}

bool parseEventType(const std::string& str, EventType& out) {
    if (str == "LINK_DOWN")    { out = EventType::LINK_DOWN;    return true; }
    if (str == "LINK_UP")      { out = EventType::LINK_UP;      return true; }
    if (str == "LOAD_UPDATE")  { out = EventType::LOAD_UPDATE;  return true; }
    if (str == "DELAY_UPDATE") { out = EventType::DELAY_UPDATE; return true; }
    return false;
}

} // namespace EnumUtils
