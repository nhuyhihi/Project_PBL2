#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <vector>
#include <iostream>
#include "Graph.h"
#include "Packet.h"
#include "Scenario.h"

namespace Logger {
    struct SimulationLogEntry {
        double timestamp;         
        int packetId;              
        int source;                
        int destination;           
        std::string packetType;   
        std::string path;        
        double delay;             
        int hopCount;             
        int fragmentCount;        
        std::string status;        
        std::string dropReason;   
        std::string failureEvent;  
        bool isRerouted;          
    };

    bool exportSimulationLog(const std::string& filepath,
                             const std::string& scenarioName,
                             const std::vector<SimulationLogEntry>& logs);

} 

#endif 
