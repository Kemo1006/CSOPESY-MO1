#pragma once

#include <cstdint>
#include <string>
#include <vector>

enum class InstructionType { Print, Declare, Add, Subtract, Sleep, For };

// Either a reference to a uint16 variable or a uint16 literal.
struct Operand {
    bool isVariable = false;
    std::string variable;
    uint16_t value = 0;
};

struct Instruction {
    InstructionType type = InstructionType::Print;
    std::string message;            // PRINT: text to output
    std::string printVariable;      // PRINT: optional variable appended to the message, e.g. "Value from: " + x
    std::string target;             // DECLARE / ADD / SUBTRACT: destination variable
    Operand lhs;                    // DECLARE: default value; ADD / SUBTRACT: first operand
    Operand rhs;                    // ADD / SUBTRACT: second operand
    uint8_t sleepTicks = 0;         // SLEEP
    uint64_t repeats = 0;           // FOR
    std::vector<Instruction> body;  // FOR
};

constexpr int MAX_FOR_NESTING = 3;

// Number of instructions the program executes, with FOR bodies expanded
// by their repeat counts. FOR itself is control flow and is not counted.
uint64_t countInstructions(const std::vector<Instruction>& program);

// Builds a random program that executes exactly `length` instructions.
std::vector<Instruction> generateProgram(const std::string& processName, uint64_t length);
