#include "Scheduler.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>

#include "Utils.h"

namespace {

// Wall-clock length of one CPU tick.
constexpr std::chrono::milliseconds TICK_DURATION{100};

// Longest "Waiting processes" list printed by screen -ls / report-util.
constexpr size_t MAX_WAITING_LISTED = 20;

const std::string DIVIDER = "--------------------------------------";

std::string batchProcessName(uint64_t number) {
    return (number < 10 ? "p0" : "p") + std::to_string(number);
}

std::string formatUtilization(size_t used, size_t total) {
    double percent = 100.0 * static_cast<double>(used) / static_cast<double>(total);
    std::ostringstream out;
    if (used * 100 % total == 0) {
        out << used * 100 / total;
    } else {
        out << std::fixed << std::setprecision(2) << percent;
    }
    return out.str() + "%";
}

}  // namespace

Scheduler::Scheduler(const Config& config)
    : config_(config), cores_(config.numCpu), tickBarrier_(static_cast<std::ptrdiff_t>(config.numCpu) + 1) {}

Scheduler::~Scheduler() {
    shutdown();
}

void Scheduler::start() {
    for (uint32_t i = 0; i < config_.numCpu; ++i) {
        coreThreads_.emplace_back(&Scheduler::coreLoop, this, i);
    }
    clockThread_ = std::thread(&Scheduler::clockLoop, this);
}

void Scheduler::shutdown() {
    stopRequested_ = true;
    if (clockThread_.joinable()) {
        clockThread_.join();
    }
    for (std::thread& thread : coreThreads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    coreThreads_.clear();
}

std::shared_ptr<Process> Scheduler::createProcess(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (processes_.count(name) > 0) {
        return nullptr;
    }
    return admitLocked(name);
}

std::shared_ptr<Process> Scheduler::findProcess(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = processes_.find(name);
    return it == processes_.end() ? nullptr : it->second;
}

bool Scheduler::startGenerating() {
    return !generating_.exchange(true);
}

bool Scheduler::stopGenerating() {
    return generating_.exchange(false);
}

std::string Scheduler::utilizationReport() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::shared_ptr<Process>> running;
    for (const Core& core : cores_) {
        if (core.process) {
            running.push_back(core.process);
        }
    }

    // Processes that are not on a core and not finished: queued (Ready) or Sleeping.
    // Only the first MAX_WAITING_LISTED are listed so the report stays readable.
    std::vector<std::shared_ptr<Process>> waiting;
    waiting.insert(waiting.end(), readyQueue_.begin(), readyQueue_.end());
    waiting.insert(waiting.end(), sleeping_.begin(), sleeping_.end());
    size_t waitingShown = std::min(waiting.size(), MAX_WAITING_LISTED);

    size_t nameWidth = 10;
    for (size_t i = 0; i < waitingShown; ++i) nameWidth = std::max(nameWidth, waiting[i]->name().size() + 2);
    for (const auto& process : running) nameWidth = std::max(nameWidth, process->name().size() + 2);
    for (const auto& process : finished_) nameWidth = std::max(nameWidth, process->name().size() + 2);

    std::ostringstream out;
    out << "CPU utilization: " << formatUtilization(running.size(), cores_.size()) << "\n";
    out << "Cores used: " << running.size() << "\n";
    out << "Cores available: " << cores_.size() - running.size() << "\n";
    out << "\n" << DIVIDER << "\n";

    out << "Running processes:\n";
    for (const auto& process : running) {
        out << std::left << std::setw(static_cast<int>(nameWidth)) << process->name() << "("
            << util::formatTimestamp(process->createdAt()) << ")   Core: " << std::setw(4) << process->coreId
            << "   " << process->executedInstructions() << " / " << process->totalInstructions() << "\n";
    }

    out << "\nWaiting processes:\n";
    for (size_t i = 0; i < waitingShown; ++i) {
        const auto& process = waiting[i];
        out << std::left << std::setw(static_cast<int>(nameWidth)) << process->name() << "("
            << util::formatTimestamp(process->createdAt()) << ")   "
            << std::setw(13) << (process->state == ProcessState::Sleeping ? "Sleeping" : "Ready")
            << process->executedInstructions() << " / " << process->totalInstructions() << "\n";
    }
    if (waiting.size() > waitingShown) {
        out << "... and " << waiting.size() - waitingShown << " more waiting\n";
    }

    out << "\nFinished processes:\n";
    for (const auto& process : finished_) {
        out << std::left << std::setw(static_cast<int>(nameWidth)) << process->name() << "("
            << util::formatTimestamp(process->finishedAt) << ")   Finished     " << process->totalInstructions()
            << " / " << process->totalInstructions() << "\n";
    }
    out << DIVIDER << "\n";
    return out.str();
}

void Scheduler::clockLoop() {
    auto nextTick = std::chrono::steady_clock::now();
    while (true) {
        nextTick += TICK_DURATION;
        std::this_thread::sleep_until(nextTick);

        quitting_ = stopRequested_.load();
        if (quitting_) {
            tickBarrier_.arrive_and_wait();  // releases the cores so they can see quitting_
            return;
        }

        ++cpuTicks_;
        tickBarrier_.arrive_and_wait();  // cores execute this tick
        tickBarrier_.arrive_and_wait();  // all cores are done

        std::lock_guard<std::mutex> lock(mutex_);
        collectCoreResultsLocked();
        // With a frequency of 1, a new process is generated at the end of every CPU cycle.
        if (generating_ && cpuTicks_ % config_.batchProcessFreq == 0) {
            generateBatchProcessLocked();
        }
        wakeSleepersLocked();
        dispatchLocked();
    }
}

void Scheduler::coreLoop(uint32_t coreId) {
    while (true) {
        tickBarrier_.arrive_and_wait();
        if (quitting_) {
            return;
        }
        Core& core = cores_[coreId];
        if (core.process) {
            core.result = core.process->executeTick(static_cast<int>(coreId), config_.delayPerExec);
        }
        tickBarrier_.arrive_and_wait();
    }
}

std::shared_ptr<Process> Scheduler::admitLocked(const std::string& name) {
    uint64_t length = util::randomRange(config_.minIns, config_.maxIns);
    auto process = std::make_shared<Process>(nextPid_++, name, generateProgram(name, length));
    processes_.emplace(name, process);
    readyQueue_.push_back(process);
    return process;
}

void Scheduler::generateBatchProcessLocked() {
    std::string name;
    do {
        name = batchProcessName(nextBatchNumber_++);
    } while (processes_.count(name) > 0);
    admitLocked(name);
}

void Scheduler::collectCoreResultsLocked() {
    for (Core& core : cores_) {
        if (!core.process) {
            continue;
        }
        std::shared_ptr<Process> process = core.process;
        ++process->ticksOnCore;

        switch (core.result.kind) {
            case TickResult::Kind::Finished:
                process->state = ProcessState::Finished;
                process->finishedAt = Process::Clock::now();
                finished_.push_back(process);
                break;
            case TickResult::Kind::Sleep:
                // Executed SLEEP(X) this tick; it sits out the next X ticks.
                process->state = ProcessState::Sleeping;
                process->wakeTick = cpuTicks_ + core.result.sleepTicks + 1;
                sleeping_.push_back(process);
                break;
            case TickResult::Kind::Continue:
                if (config_.isRoundRobin() && process->ticksOnCore >= config_.quantumCycles) {
                    process->state = ProcessState::Ready;
                    readyQueue_.push_back(process);
                    break;
                }
                continue;  // keeps the core
        }
        process->coreId = -1;
        core.process.reset();
    }
}

void Scheduler::wakeSleepersLocked() {
    // Processes due on the next tick rejoin the ready queue now so they can be dispatched for it.
    auto stillSleeping = std::stable_partition(sleeping_.begin(), sleeping_.end(), [this](const auto& process) {
        return process->wakeTick > cpuTicks_ + 1;
    });
    for (auto it = stillSleeping; it != sleeping_.end(); ++it) {
        (*it)->state = ProcessState::Ready;
        readyQueue_.push_back(*it);
    }
    sleeping_.erase(stillSleeping, sleeping_.end());
}

void Scheduler::dispatchLocked() {
    for (size_t i = 0; i < cores_.size() && !readyQueue_.empty(); ++i) {
        if (cores_[i].process) {
            continue;
        }
        std::shared_ptr<Process> process = readyQueue_.front();
        readyQueue_.pop_front();
        process->state = ProcessState::Running;
        process->coreId = static_cast<int>(i);
        process->ticksOnCore = 0;
        cores_[i].process = process;
        cores_[i].result = {};
    }
}
