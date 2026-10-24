@echo off
g++ -std=c++20 -O2 -Wall -Wextra -static -pthread src\*.cpp -o csopesy.exe
if %errorlevel% neq 0 (
    echo Build failed.
    exit /b %errorlevel%
)
echo Built csopesy.exe
