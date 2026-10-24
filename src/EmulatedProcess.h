#pragma once

#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "Instruction.h"

enum class ProcessState { Ready, Running, Sleeping, Finished };

// Outcome of giving a process one CPU tick.
struct TickResult {
    enum class Kind { Continue, Sleep, Finished };
    Kind kind = Kind::Continue;
    uint8_t sleepTicks = 0;
};

class Process {
public:
    using Clock = std::chrono::system_clock;

    Process(int id, std::string name, std::vector<Instruction> program);
    Process(const Process&) = delete;
    Process& operator=(const Process&) = delete;

    int id() const { return id_; }
    const std::string& name() const { return name_; }
    Clock::time_point createdAt() const { return createdAt_; }
    uint64_t totalInstructions() const { return total_; }

    // Runs one CPU tick on `coreId`: either one busy-wait cycle of
    // delay-per-exec or the next instruction.
    TickResult executeTick(int coreId, uint64_t delayPerExec);

    uint64_t executedInstructions() const;
    bool isFinished() const;
    std::vector<std::string> logs() const;

    // Scheduling bookkeeping, guarded by the Scheduler's mutex.
    ProcessState state = ProcessState::Ready;
    int coreId = -1;
    uint64_t ticksOnCore = 0;
    uint64_t wakeTick = 0;
    Clock::time_point finishedAt{};

private:
    // One level of the FOR-loop stack; the bottom frame is the program itself.
    struct Frame {
        const std::vector<Instruction>* instructions;
        size_t pc;
        uint64_t iterationsLeft;
    };

    const Instruction* nextInstruction();
    void execute(const Instruction& instruction, int coreId);
    uint16_t& variable(const std::string& name);
    uint16_t resolve(const Operand& operand);

    const int id_;
    const std::string name_;
    const std::vector<Instruction> program_;
    const uint64_t total_;
    const Clock::time_point createdAt_;

    mutable std::mutex mutex_;
    std::vector<Frame> frames_;
    std::unordered_map<std::string, uint16_t> variables_;
    std::vector<std::string> logs_;
    uint64_t executed_ = 0;
    uint64_t busyWait_ = 0;
};
