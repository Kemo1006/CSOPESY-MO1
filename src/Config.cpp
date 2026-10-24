#include "Config.h"

#include <algorithm>
#include <cctype>
#include <fstream>

namespace {

constexpr uint64_t TWO_POW_32 = 4294967296ULL;

bool parseNumber(const std::string& key, const std::string& value, uint64_t low, uint64_t high,
                 uint64_t& out, std::string& error) {
    bool allDigits = !value.empty() &&
                     std::all_of(value.begin(), value.end(), [](unsigned char c) { return std::isdigit(c); });
    if (!allDigits || value.size() > 19) {
        error = "\"" + key + "\" must be a whole number, got \"" + value + "\".";
        return false;
    }
    uint64_t number = std::stoull(value);
    if (number < low || number > high) {
        error = "\"" + key + "\" must be in the range [" + std::to_string(low) + ", " + std::to_string(high) +
                "], got " + value + ".";
        return false;
    }
    out = number;
    return true;
}

std::string stripQuotes(const std::string& value) {
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
        return value.substr(1, value.size() - 2);
    }
    return value;
}

}  // namespace

bool Config::load(const std::string& path, Config& out, std::string& error) {
    std::ifstream file(path);
    if (!file) {
        error = "Could not open \"" + path + "\".";
        return false;
    }

    Config config;
    std::string key;
    std::string rawValue;
    while (file >> key) {
        if (!(file >> rawValue)) {
            error = "Missing value for \"" + key + "\".";
            return false;
        }
        std::string value = stripQuotes(rawValue);
        uint64_t number = 0;

        if (key == "num-cpu") {
            if (!parseNumber(key, value, 1, 128, number, error)) return false;
            config.numCpu = static_cast<uint32_t>(number);
        } else if (key == "scheduler") {
            std::transform(value.begin(), value.end(), value.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            if (value != "fcfs" && value != "rr") {
                error = "\"scheduler\" must be \"fcfs\" or \"rr\", got \"" + value + "\".";
                return false;
            }
            config.scheduler = value;
        } else if (key == "quantum-cycles") {
            if (!parseNumber(key, value, 1, TWO_POW_32, config.quantumCycles, error)) return false;
        } else if (key == "batch-process-freq") {
            if (!parseNumber(key, value, 1, TWO_POW_32, config.batchProcessFreq, error)) return false;
        } else if (key == "min-ins") {
            if (!parseNumber(key, value, 1, TWO_POW_32, config.minIns, error)) return false;
        } else if (key == "max-ins") {
            if (!parseNumber(key, value, 1, TWO_POW_32, config.maxIns, error)) return false;
        } else if (key == "delay-per-exec" || key == "delays-per-exec") {
            // The specs' table says "delays-per-exec"; its sample file says "delay-per-exec".
            if (!parseNumber(key, value, 0, TWO_POW_32, config.delayPerExec, error)) return false;
        } else {
            error = "Unknown parameter \"" + key + "\".";
            return false;
        }
    }

    if (config.minIns > config.maxIns) {
        error = "\"min-ins\" (" + std::to_string(config.minIns) + ") cannot be greater than \"max-ins\" (" +
                std::to_string(config.maxIns) + ").";
        return false;
    }

    out = config;
    return true;
}
