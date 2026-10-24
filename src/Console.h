#pragma once

#include <memory>
#include <string>
#include <vector>

#include "EmulatedProcess.h"
#include "Scheduler.h"

// The main menu console and the screen process sessions.
class Console {
public:
    void run();

private:
    void printHeader() const;
    void initialize();
    void handleScreen(const std::vector<std::string>& args);
    void attachScreen(const std::shared_ptr<Process>& process);
    void printProcessInfo(const Process& process) const;
    void reportUtil() const;

    std::unique_ptr<Scheduler> scheduler_;
};
