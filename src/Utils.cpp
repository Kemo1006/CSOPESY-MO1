#include "Utils.h"

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <random>
#include <sstream>

namespace util {

std::string formatTimestamp(std::chrono::system_clock::time_point timePoint) {
    std::time_t time = std::chrono::system_clock::to_time_t(timePoint);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &time);
#else
    localtime_r(&time, &local);
#endif
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%m/%d/%Y %I:%M:%S%p", &local);
    return buffer;
}

void clearScreen() {
    std::cout << std::flush;
#ifdef _WIN32
    std::system("cls");
#else
    std::system("clear");
#endif
}

std::string trim(const std::string& text) {
    const char* whitespace = " \t\r\n";
    size_t start = text.find_first_not_of(whitespace);
    if (start == std::string::npos) {
        return "";
    }
    size_t end = text.find_last_not_of(whitespace);
    return text.substr(start, end - start + 1);
}

std::vector<std::string> split(const std::string& text) {
    std::vector<std::string> tokens;
    std::istringstream stream(text);
    std::string token;
    while (stream >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

uint64_t randomRange(uint64_t low, uint64_t high) {
    thread_local std::mt19937_64 engine{std::random_device{}()};
    return std::uniform_int_distribution<uint64_t>(low, high)(engine);
}

}  // namespace util
