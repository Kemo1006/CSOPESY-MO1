#pragma once

#include <atomic>
#include <barrier>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "Config.h"
#include "EmulatedProcess.h"

// Real-time CPU scheduler. A clock thread advances the CPU tick counter and
// one worker thread per core executes its assigned process once per tick.
// Each tick runs in lock step:
//   1. every core executes one tick of its process (in parallel),
//   2. the clock thread finishes/sleeps/preempts processes, generates a batch
//      process if due, wakes sleepers, and dispatches the ready queue to free cores.
class Scheduler {
public:
    explicit Scheduler(const Config& config);
    ~Scheduler();
    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;

    void start();
    void shutdown();

    const Config& config() const { return config_; }

    // Creates a process with random instructions and puts it in the ready
    // queue. Returns nullptr if the name is already taken.
    std::shared_ptr<Process> createProcess(const std::string& name);
    std::shared_ptr<Process> findProcess(const std::string& name) const;

    // Return false if batch generation was already in the requested state.
    bool startGenerating();
    bool stopGenerating();

    // CPU utilization, cores used/available, running and finished processes.
    std::string utilizationReport() const;

private:
    struct Core {
        std::shared_ptr<Process> process;
        TickResult result;
    };

    void clockLoop();
    void coreLoop(uint32_t coreId);

    std::shared_ptr<Process> admitLocked(const std::string& name);
    void generateBatchProcessLocked();
    void collectCoreResultsLocked();
    void wakeSleepersLocked();
    void dispatchLocked();

    const Config config_;

    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<Process>> processes_;
    std::deque<std::shared_ptr<Process>> readyQueue_;
    std::vector<std::shared_ptr<Process>> sleeping_;
    std::vector<std::shared_ptr<Process>> finished_;
    // Written by the clock thread (under mutex_) only between ticks; read by
    // each core thread only while the tick executes.
    std::vector<Core> cores_;
    int nextPid_ = 1;
    uint64_t nextBatchNumber_ = 1;

    uint64_t cpuTicks_ = 0;  // only touched by the clock thread
    std::barrier<> tickBarrier_;
    std::atomic<bool> generating_{false};
    std::atomic<bool> stopRequested_{false};
    std::atomic<bool> quitting_{false};
    std::thread clockThread_;
    std::vector<std::thread> coreThreads_;
};
