#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace util {

// Formats a time point as "MM/DD/YYYY hh:mm:ssAM".
std::string formatTimestamp(std::chrono::system_clock::time_point timePoint);

void clearScreen();

std::string trim(const std::string& text);

// Splits on whitespace, dropping empty tokens.
std::vector<std::string> split(const std::string& text);

// Uniform random integer in [low, high].
uint64_t randomRange(uint64_t low, uint64_t high);

}  // namespace util
