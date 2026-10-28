@echo off
setlocal
if exist "C:\msys64\ucrt64\bin\g++.exe" (
    set "CXX=C:\msys64\ucrt64\bin\g++.exe"
) else (
    set "CXX=g++"
)
%CXX% -std=c++20 -O2 -Wall -Wextra -static -pthread src\*.cpp -o csopesy.exe
if %errorlevel% neq 0 (
    echo Build failed.
    exit /b %errorlevel%
)
echo Built csopesy.exe
