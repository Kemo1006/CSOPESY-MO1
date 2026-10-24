#include "EmulatedProcess.h"

#include <limits>

#include "Utils.h"

namespace {

uint16_t clampUint16(int64_t value) {
    if (value < 0) return 0;
    if (value > std::numeric_limits<uint16_t>::max()) return std::numeric_limits<uint16_t>::max();
    return static_cast<uint16_t>(value);
}

}  // namespace

Process::Process(int id, std::string name, std::vector<Instruction> program)
    : id_(id),
      name_(std::move(name)),
      program_(std::move(program)),
      total_(countInstructions(program_)),
      createdAt_(Clock::now()) {
    frames_.push_back({&program_, 0, 1});
}

TickResult Process::executeTick(int coreId, uint64_t delayPerExec) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (executed_ >= total_) {
        return {TickResult::Kind::Finished};
    }
    if (busyWait_ > 0) {
        --busyWait_;
        return {};
    }

    const Instruction* instruction = nextInstruction();
    if (instruction == nullptr) {
        executed_ = total_;
        return {TickResult::Kind::Finished};
    }
    execute(*instruction, coreId);
    ++executed_;

    if (executed_ >= total_) {
        return {TickResult::Kind::Finished};
    }
    if (instruction->type == InstructionType::Sleep && instruction->sleepTicks > 0) {
        return {TickResult::Kind::Sleep, instruction->sleepTicks};
    }
    busyWait_ = delayPerExec;
    return {};
}

uint64_t Process::executedInstructions() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return executed_;
}

bool Process::isFinished() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return executed_ >= total_;
}

std::vector<std::string> Process::logs() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return logs_;
}

const Instruction* Process::nextInstruction() {
    while (!frames_.empty()) {
        Frame& frame = frames_.back();
        if (frame.pc >= frame.instructions->size()) {
            if (--frame.iterationsLeft > 0) {
                frame.pc = 0;
            } else {
                frames_.pop_back();
            }
            continue;
        }

        const Instruction& instruction = (*frame.instructions)[frame.pc++];
        if (instruction.type != InstructionType::For) {
            return &instruction;
        }
        if (instruction.repeats > 0 && !instruction.body.empty()) {
            frames_.push_back({&instruction.body, 0, instruction.repeats});
        }
    }
    return nullptr;
}

void Process::execute(const Instruction& instruction, int coreId) {
    switch (instruction.type) {
        case InstructionType::Print: {
            std::string message = instruction.message;
            if (!instruction.printVariable.empty()) {
                message += std::to_string(variable(instruction.printVariable));
            }
            logs_.push_back("(" + util::formatTimestamp(Clock::now()) + ") Core:" + std::to_string(coreId) + " \"" +
                            message + "\"");
            break;
        }
        case InstructionType::Declare:
            variable(instruction.target) = instruction.lhs.value;
            break;
        case InstructionType::Add: {
            int64_t sum = static_cast<int64_t>(resolve(instruction.lhs)) + resolve(instruction.rhs);
            variable(instruction.target) = clampUint16(sum);
            break;
        }
        case InstructionType::Subtract: {
            int64_t difference = static_cast<int64_t>(resolve(instruction.lhs)) - resolve(instruction.rhs);
            variable(instruction.target) = clampUint16(difference);
            break;
        }
        case InstructionType::Sleep:
            // The scheduler moves the process off the CPU; see executeTick.
            break;
        case InstructionType::For:
            // Expanded by nextInstruction, never executed directly.
            break;
    }
}

uint16_t& Process::variable(const std::string& name) {
    // Undeclared variables are automatically declared with a value of 0.
    return variables_.try_emplace(name, 0).first->second;
}

uint16_t Process::resolve(const Operand& operand) {
    return operand.isVariable ? variable(operand.variable) : operand.value;
}
