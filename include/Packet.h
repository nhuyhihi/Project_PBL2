#ifndef PACKET_H
#define PACKET_H

#include "Types.h"

struct Packet {
    int id;
    int source;
    int destination;
    int sizeBytes;
    PacketType type;
    Protocol protocol;
    bool canFragment;
    bool newConnection;

    Packet() : id(-1), source(-1), destination(-1), sizeBytes(0), 
               type(PacketType::BULK_DATA), protocol(Protocol::UDP), 
               canFragment(true), newConnection(false) {}
};

#endif // PACKET_H
