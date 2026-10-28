# CSOPESY Major Output 1: Process Scheduler and CLI

| Name | Role |
|---|---|
| Colcol, Llandro Massimo | Developer |
| Ong, Kyle Edward | Developer |
| Tayzon, Hannah Moriah | Developer |

## Entry point
src/main.cpp  (int main() creates a Console and calls Console::run())

## Building
Requires a C++20 compiler (tested with MSYS2 UCRT64 g++ 16.1).

  Windows:  build.bat
  Manual:   g++ -std=c++20 -O2 -static -pthread src/*.cpp -o csopesy.exe

## Running
Run csopesy.exe from the folder that contains config.txt. The "initialize"
command reads config.txt from the current working directory, and
"report-util" writes csopesy-log.txt to the same directory.

  csopesy.exe

## Commands
### Main menu (only "initialize" and "exit" work before initializing):
| Command | What it does |
|---|---|
| initialize | Read config.txt and start the CPU scheduler |
| exit | Terminate the console |
| screen -s <process name> | Create a process and attach to its screen |
| screen -r <process name> | Reattach to an unfinished process's screen |
| screen -ls | Show CPU utilization, cores used/available, running and finished     processes |
| scheduler-start | Generate a dummy process every batch-process-freq CPU ticks (p01, p02, ...) |
| scheduler-stop | Stop generating dummy processes |
| report-util | Save the "screen -ls" report to csopesy-log.txt |

### Inside a process screen:
| Command | What it does |
|---|---|
| process-smi | Show process name, ID, logs, and progress ("Finished!" once it is done) |
| exit | Return to the main menu |

## config.txt
Space-separated "parameter value" pairs:
  - num-cpu             [1, 128]
  - scheduler           "fcfs" or "rr"
  - quantum-cycles      [1, 2^32]  (rr only)
  - batch-process-freq  [1, 2^32]
  - min-ins             [1, 2^32]
  - max-ins             [1, 2^32]
  - delay-per-exec      [0, 2^32]  ("delays-per-exec" is also accepted)

## Implementation notes
- One CPU tick lasts 100 ms of wall-clock time (TICK_DURATION in
  src/Scheduler.cpp). A clock thread advances the tick counter; one worker
  thread per core executes its process once per tick.
- Instruction counts ("Lines of code", "x / y" in reports) are the number of
  instructions a process executes, with FOR bodies expanded by their repeat
  counts. Each process executes a random count in [min-ins, max-ins].
- Generated programs use PRINT, DECLARE, ADD, SUBTRACT, SLEEP and FOR, with FOR
  loops nested at most 3 levels. PRINT outputs "Hello world from <name>!".
- SLEEP(X) moves the process off the CPU for X ticks; generated sleeps use
  X in [1, 10].
- delay-per-exec keeps the process on its core, busy-waiting, for that many
  ticks between instructions.
- "Running processes" lists the processes currently on a core. "Waiting
  processes" lists processes that are queued (Ready) or Sleeping, and shows
  at most the first 20, followed by "... and N more waiting" if there are
  more. "Finished processes" lists the processes that completed.

## Source layout
  - src/main.cpp             Entry point
  - src/Console.*            Main menu, command recognition, screen sessions
  - src/Config.*             config.txt parsing and validation
  - src/Scheduler.*          CPU clock, cores, FCFS/RR scheduling, batch generation
  - src/EmulatedProcess.*    Process state, variables, logs, instruction execution
  - src/Instruction.*        Instruction representation and random generation
  - src/Utils.*              Timestamps, console clearing, string helpers