@echo off
cd /d %~dp0

echo Running CMake...
echo/
cmake -B build -G "Visual Studio 17 2022"
echo/
echo Assuming CMake setup went well, the project sln is located at "build/reloaded2ps3.sln".
echo/
pause
