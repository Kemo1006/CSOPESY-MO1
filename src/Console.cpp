#include "Console.h"

#include <filesystem>
#include <fstream>
#include <iostream>

#include "Utils.h"

namespace {

const std::string PROMPT = "root:\\> ";
const std::string CONFIG_FILE = "config.txt";
const std::string LOG_FILE = "csopesy-log.txt";
const std::string LAST_UPDATED = "10-24-2026";

const std::vector<std::string> DEVELOPERS = {
    "Colcol, Llandro Massimo",
    "Ong, Kyle Edward",
    "Tayzon, Hannah Moriah"
};

const std::vector<std::vector<std::string>> BANNER_GLYPHS = {
    {R"(  _____ )", R"( / ____|)", R"(| |     )", R"(| |     )", R"(| |____ )", R"( \_____|)"},
    {R"(  _____ )", R"( / ____|)", R"(| (___  )", R"( \___ \ )", R"( ____) |)", R"(|_____/ )"},
    {R"(  ____  )", R"( / __ \ )", R"(| |  | |)", R"(| |  | |)", R"(| |__| |)", R"( \____/ )"},
    {R"( _____  )", R"(|  __ \ )", R"(| |__) |)", R"(|  ___/ )", R"(| |     )", R"(|_|     )"},
    {R"( ______ )", R"(|  ____|)", R"(| |__   )", R"(|  __|  )", R"(| |____ )", R"(|______|)"},
    {R"(  _____ )", R"( / ____|)", R"(| (___  )", R"( \___ \ )", R"( ____) |)", R"(|_____/ )"},
    {R"(__     __)", R"(\ \   / /)", R"( \ \_/ / )", R"(  \   /  )", R"(   | |   )", R"(   |_|   )"},
};

const std::string DIVIDER = "-----------------------------------------------------";

}  // namespace

void Console::run() {
    util::clearScreen();
    printHeader();

    std::string line;
    while (true) {
        std::cout << PROMPT << std::flush;
        if (!std::getline(std::cin, line)) {
            break;
        }
        std::vector<std::string> args = util::split(line);
        if (args.empty()) {
            continue;
        }
        const std::string& command = args[0];

        if (command == "exit") {
            break;
        }
        if (command == "initialize") {
            initialize();
            continue;
        }
        if (!scheduler_) {
            std::cout << "Please run \"initialize\" first.\n";
            continue;
        }

        if (command == "screen") {
            handleScreen(args);
        } else if (command == "scheduler-start") {
            if (scheduler_->startGenerating()) {
                std::cout << "Scheduler started. Generating dummy processes...\n";
            } else {
                std::cout << "Scheduler is already running.\n";
            }
        } else if (command == "scheduler-stop") {
            if (scheduler_->stopGenerating()) {
                std::cout << "Scheduler stopped. No longer generating dummy processes.\n";
            } else {
                std::cout << "Scheduler is not running.\n";
            }
        } else if (command == "report-util") {
            reportUtil();
        } else {
            std::cout << "Command not recognized: " << command << "\n";
        }
    }

    if (scheduler_) {
        scheduler_->shutdown();
    }
}

void Console::printHeader() const {
    for (size_t row = 0; row < BANNER_GLYPHS.front().size(); ++row) {
        std::string bannerLine;
        for (const auto& glyph : BANNER_GLYPHS) {
            bannerLine += glyph[row];
        }
        std::cout << bannerLine << "\n";
    }
    std::cout << DIVIDER << "\n";
    std::cout << "Welcome to CSOPESY Emulator!\n\n";
    std::cout << "Developers:\n";
    for (const std::string& developer : DEVELOPERS) {
        std::cout << developer << "\n";
    }
    std::cout << "\nLast updated: " << LAST_UPDATED << "\n";
    std::cout << DIVIDER << "\n\n";
}

void Console::initialize() {
    if (scheduler_) {
        std::cout << "The emulator is already initialized.\n";
        return;
    }

    Config config;
    std::string error;
    if (!Config::load(CONFIG_FILE, config, error)) {
        std::cout << "Initialization failed: " << error << "\n";
        return;
    }

    scheduler_ = std::make_unique<Scheduler>(config);
    scheduler_->start();

    std::cout << "Initialized from " << CONFIG_FILE << ":\n"
              << "  num-cpu " << config.numCpu << "\n"
              << "  scheduler " << config.scheduler << "\n"
              << "  quantum-cycles " << config.quantumCycles << "\n"
              << "  batch-process-freq " << config.batchProcessFreq << "\n"
              << "  min-ins " << config.minIns << "\n"
              << "  max-ins " << config.maxIns << "\n"
              << "  delay-per-exec " << config.delayPerExec << "\n";
}

void Console::handleScreen(const std::vector<std::string>& args) {
    if (args.size() == 2 && args[1] == "-ls") {
        std::cout << scheduler_->utilizationReport() << "\n";
        return;
    }

    if (args.size() == 3 && args[1] == "-s") {
        std::shared_ptr<Process> process = scheduler_->createProcess(args[2]);
        if (!process) {
            std::cout << "Process " << args[2] << " already exists.\n";
            return;
        }
        attachScreen(process);
        return;
    }

    if (args.size() == 3 && args[1] == "-r") {
        std::shared_ptr<Process> process = scheduler_->findProcess(args[2]);
        if (!process || process->isFinished()) {
            std::cout << "Process " << args[2] << " not found.\n";
            return;
        }
        attachScreen(process);
        return;
    }

    std::cout << "Usage:\n"
              << "  screen -s <process name>   Create a new process and attach to its screen\n"
              << "  screen -r <process name>   Reattach to a running process's screen\n"
              << "  screen -ls                 List CPU utilization and running/finished processes\n";
}

void Console::attachScreen(const std::shared_ptr<Process>& process) {
    util::clearScreen();
    printProcessInfo(*process);

    std::string line;
    while (true) {
        std::cout << "\n" << PROMPT << std::flush;
        if (!std::getline(std::cin, line)) {
            return;
        }
        std::vector<std::string> args = util::split(line);
        if (args.empty()) {
            continue;
        }

        if (args[0] == "process-smi") {
            std::cout << "\n";
            printProcessInfo(*process);
        } else if (args[0] == "exit") {
            break;
        } else {
            std::cout << "Command not recognized: " << args[0] << "\n";
        }
    }

    util::clearScreen();
    printHeader();
}

void Console::printProcessInfo(const Process& process) const {
    // Read progress before the logs so a finished process never shows logs missing.
    uint64_t executed = process.executedInstructions();
    std::vector<std::string> logs = process.logs();

    std::cout << "Process name: " << process.name() << "\n";
    std::cout << "ID: " << process.id() << "\n";
    std::cout << "Logs:\n";
    for (const std::string& log : logs) {
        std::cout << log << "\n";
    }
    std::cout << "\n";

    if (executed >= process.totalInstructions()) {
        std::cout << "Finished!\n";
    } else {
        std::cout << "Current instruction line: " << executed << "\n";
        std::cout << "Lines of code: " << process.totalInstructions() << "\n";
    }
}

void Console::reportUtil() const {
    std::ofstream file(LOG_FILE);
    if (!file) {
        std::cout << "Could not write " << LOG_FILE << ".\n";
        return;
    }
    file << scheduler_->utilizationReport();
    file.close();
    std::cout << "Report generated at " << std::filesystem::absolute(LOG_FILE).generic_string() << "!\n";
}
