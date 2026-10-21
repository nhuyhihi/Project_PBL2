#ifndef SCENARIO_H
#define SCENARIO_H

#include <string>
#include <vector>
#include "Types.h"

struct ScenarioEvent {
    EventType eventType;
    int targetId;
    double value;
    double time;
};

struct Scenario {
    std::string name;
    std::vector<ScenarioEvent> events;
};

#endif 
