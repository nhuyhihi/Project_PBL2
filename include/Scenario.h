#ifndef SCENARIO_H
#define SCENARIO_H

#include <string>
#include <vector>
#include "Types.h"

// Task B4.3: ScenarioEvent
struct ScenarioEvent {
    EventType eventType;
    int targetId;
    double value;
    double time;
};

// Cấu trúc gom nhóm toàn bộ các sự kiện của một kịch bản cụ thể
struct Scenario {
    std::string name;
    std::vector<ScenarioEvent> events;
};

#endif // SCENARIO_H
