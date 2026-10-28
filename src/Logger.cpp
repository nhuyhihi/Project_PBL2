#include "Logger.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>

namespace Logger {

bool exportSimulationLog(const std::string& filepath,
                         const std::string& scenarioName,
                         const std::vector<SimulationLogEntry>& logs) {
    std::ofstream outFile(filepath);
    if (!outFile.is_open()) {
        std::cerr << "Loi: Khong the mo hoac tao file log: " << filepath << "\n";
        return false;
    }

    outFile << "========================================================================================================\n";
    outFile << "                               NHAT KY MO PHONG MANG (SIMULATION LOG)                                   \n";
    outFile << "Kich ban (Scenario) : " << scenarioName << "\n";
    outFile << "Tong so ban ghi log : " << logs.size() << "\n";
    outFile << "========================================================================================================\n\n";

    outFile << std::left
            << std::setw(8)  << "Time(s)"
            << std::setw(8)  << "Packet"
            << std::setw(6)  << "Src"
            << std::setw(6)  << "Dst"
            << std::setw(6)  << "Hop"
            << std::setw(8)  << "Frag"
            << std::setw(12) << "Delay(s)"
            << std::setw(10) << "Status"
            << std::setw(20) << "Path"
            << std::setw(28) << "Failure / Note"
            << "\n";
    outFile << "--------------------------------------------------------------------------------------------------------\n";

    for (const auto& entry : logs) {
        std::string delayStr = (entry.status == "SUCCESS") ? std::to_string(entry.delay) : "-";
        if (entry.status == "SUCCESS") {
            // Định dạng 6 chữ số thập phân cho delay
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%.6f", entry.delay);
            delayStr = buf;
        }

        std::string note = "";
        if (!entry.failureEvent.empty()) {
            note = entry.failureEvent;
        }
        if (entry.status != "SUCCESS" && !entry.dropReason.empty()) {
            if (!note.empty()) note += " | ";
            note += "DROP: " + entry.dropReason;
        } else if (entry.isRerouted) {
            if (!note.empty()) note += " | ";
            note += "[REROUTED]";
        }

        outFile << std::left
                << std::setw(8)  << std::fixed << std::setprecision(1) << entry.timestamp
                << std::setw(8)  << ("#" + std::to_string(entry.packetId))
                << std::setw(6)  << entry.source
                << std::setw(6)  << entry.destination
                << std::setw(6)  << (entry.status == "SUCCESS" ? std::to_string(entry.hopCount) : "-")
                << std::setw(8)  << entry.fragmentCount
                << std::setw(12) << delayStr
                << std::setw(10) << entry.status
                << std::setw(20) << entry.path
                << note
                << "\n";
    }

    outFile << "--------------------------------------------------------------------------------------------------------\n";
    outFile << "Ket thuc nhat ky mo phong.\n";
    outFile.close();
    return true;
}

} 
