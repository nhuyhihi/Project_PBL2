#ifndef EDGE_H
#define EDGE_H

#include "Types.h"

struct Edge {
    int id;
    int u;
    int v;
    MediaType mediaType;
    double length;            // m
    double bandwidthMbps;     // Mbps
    double maxSegmentLength;  // m
    double terrainFactor;
    int mtu;                  // bytes
    
    double currentLoad;       // 0.0 to 1.0
    double queueDelay;        // seconds
    bool isUp;
    bool isBackup;
    bool isBuilt;

    Edge() : id(-1), u(-1), v(-1), mediaType(MediaType::COPPER), length(0.0), 
             bandwidthMbps(0.0), maxSegmentLength(0.0), 
             terrainFactor(1.0), 
             mtu(1500), currentLoad(0.0), queueDelay(0.0), 
             isUp(true), isBackup(false), isBuilt(false) {}
};

#endif // EDGE_H
