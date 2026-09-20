#ifndef TYPES_H
#define TYPES_H

#include <string>

enum class NodeType {
    ROUTER,
    HOST,
    SWITCH
};

enum class MediaType {
    FIBER,
    COPPER,
    WIRELESS
};

enum class Protocol {
    TCP,
    UDP
};

enum class PacketType {
    REAL_TIME,
    BULK_DATA,
    CONTROL
};

enum class EventType {
    LINK_DOWN,
    LINK_UP,
    LOAD_UPDATE,
    DELAY_UPDATE
};

#endif // TYPES_H
