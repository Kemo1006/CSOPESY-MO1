#include "Instruction.h"

#include <algorithm>
#include <limits>

#include "Utils.h"

namespace {

const std::vector<std::string> VARIABLE_NAMES = {"x", "y", "z", "a", "b", "c"};
constexpr uint64_t MAX_FOR_REPEATS = 5;
constexpr uint64_t MAX_FOR_BODY = 5;
constexpr uint8_t MAX_GENERATED_SLEEP = 10;

std::string randomVariable() {
    return VARIABLE_NAMES[util::randomRange(0, VARIABLE_NAMES.size() - 1)];
}

uint16_t randomUint16() {
    return static_cast<uint16_t>(util::randomRange(0, std::numeric_limits<uint16_t>::max()));
}

Operand randomOperand() {
    Operand operand;
    if (util::randomRange(0, 1) == 0) {
        operand.isVariable = true;
        operand.variable = randomVariable();
    } else {
        operand.value = randomUint16();
    }
    return operand;
}

Instruction randomLeaf(const std::string& processName) {
    Instruction instruction;
    switch (util::randomRange(0, 4)) {
        case 0:
            instruction.type = InstructionType::Print;
            instruction.message = "Hello world from " + processName + "!";
            break;
        case 1:
            instruction.type = InstructionType::Declare;
            instruction.target = randomVariable();
            instruction.lhs.value = randomUint16();
            break;
        case 2:
            instruction.type = InstructionType::Add;
            instruction.target = randomVariable();
            instruction.lhs = randomOperand();
            instruction.rhs = randomOperand();
            break;
        case 3:
            instruction.type = InstructionType::Subtract;
            instruction.target = randomVariable();
            instruction.lhs = randomOperand();
            instruction.rhs = randomOperand();
            break;
        default:
            instruction.type = InstructionType::Sleep;
            instruction.sleepTicks = static_cast<uint8_t>(util::randomRange(1, MAX_GENERATED_SLEEP));
            break;
    }
    return instruction;
}

std::vector<Instruction> generateBlock(const std::string& processName, uint64_t length, int depth) {
    std::vector<Instruction> block;
    uint64_t remaining = length;
    while (remaining > 0) {
        bool canLoop = depth < MAX_FOR_NESTING && remaining >= 2;
        // FOR is one of the six instruction types, so it is picked 1 in 6 times when allowed.
        if (canLoop && util::randomRange(0, 5) == 0) {
            Instruction loop;
            loop.type = InstructionType::For;
            loop.repeats = util::randomRange(2, std::min(remaining, MAX_FOR_REPEATS));
            uint64_t bodyLength = util::randomRange(1, std::min(remaining / loop.repeats, MAX_FOR_BODY));
            loop.body = generateBlock(processName, bodyLength, depth + 1);
            remaining -= bodyLength * loop.repeats;
            block.push_back(std::move(loop));
        } else {
            block.push_back(randomLeaf(processName));
            --remaining;
        }
    }
    return block;
}

}  // namespace

uint64_t countInstructions(const std::vector<Instruction>& program) {
    uint64_t count = 0;
    for (const Instruction& instruction : program) {
        if (instruction.type == InstructionType::For) {
            count += instruction.repeats * countInstructions(instruction.body);
        } else {
            ++count;
        }
    }
    return count;
}

std::vector<Instruction> generateProgram(const std::string& processName, uint64_t length) {
    return generateBlock(processName, length, 0);
}
