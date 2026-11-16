@echo off
rem Double-click this file, or run it from any terminal. It takes no arguments.
rem It deletes the build\ folder (solution, compiled files, vcpkg packages)
rem and the benchmark output images. Source files are not touched.
setlocal
cd /d "%~dp0"

if not exist "build" goto clean_outputs
echo [clean] Deleting build\ ...
rmdir /s /q "build"
if exist "build" goto locked

:clean_outputs
if exist "bench_*.png" del /q "bench_*.png"
echo [clean] Done.
exit /b 0

:locked
echo [clean] Could not delete all of build\.
echo         Close Visual Studio and any running program from this project, then try again.
if not defined SEAM_NO_PAUSE pause
exit /b 1
