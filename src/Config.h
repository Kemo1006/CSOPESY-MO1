#pragma once

#include <cstdint>
#include <string>

// Parameters read from config.txt by the "initialize" command.
// Defaults match the sample config.txt in the project specs.
struct Config {
    uint32_t numCpu = 4;
    std::string scheduler = "rr";  // "fcfs" or "rr"
    uint64_t quantumCycles = 5;
    uint64_t batchProcessFreq = 1;
    uint64_t minIns = 1000;
    uint64_t maxIns = 2000;
    uint64_t delayPerExec = 0;

    bool isRoundRobin() const { return scheduler == "rr"; }

    // Reads a space-separated "key value" file. Returns false and fills
    // `error` if the file is missing or a parameter is invalid.
    static bool load(const std::string& path, Config& out, std::string& error);
};
