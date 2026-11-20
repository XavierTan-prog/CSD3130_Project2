@echo off
rem Builds the results harness in Release and runs it on this PC.
rem Double-click it. Output is saved to images\output\results\windows_output.txt.
setlocal
cd /d "%~dp0..\.."
if not exist images\output\results mkdir images\output\results
set "LOG=%CD%\images\output\results\windows_output.txt"

set "VSDIR="
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -property installationPath`) do set "VSDIR=%%i"
set "CMAKE=cmake"
if defined VSDIR if exist "%VSDIR%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" set "CMAKE=%VSDIR%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if not exist build\CMakeCache.txt (
  echo Run run.bat once first so build\ exists. > "%LOG%"
  type "%LOG%"
  pause
  exit /b 1
)

echo [results] Building in Release (a few minutes) ...
echo === results harness, %DATE% %TIME% === > "%LOG%"
"%CMAKE%" -B build -S . >> "%LOG%" 2>&1
if errorlevel 1 goto failed
"%CMAKE%" --build build --config Release --target tests results_eval results_gcbig results_grid >> "%LOG%" 2>&1
if errorlevel 1 goto failed
"%CMAKE%" --build build --config Release --target results_omp >> "%LOG%" 2>&1
if errorlevel 1 echo [results] results_omp did not build (OpenMP), skipped. >> "%LOG%"

rem The results go to a separate file first so they can be shown in this
rem window at the end without the build messages; then they join the log.
set "RES=%CD%\images\output\results\results_only.tmp"
echo [results] Running tests ...
echo ===== tests ===== > "%RES%"
build\Release\tests.exe >> "%RES%" 2>&1
echo [results] Running results_grid ...
echo ===== results_grid ===== >> "%RES%"
build\Release\results_grid.exe >> "%RES%" 2>&1
echo [results] Running results_eval (1-2 minutes) ...
echo ===== results_eval ===== >> "%RES%"
build\Release\results_eval.exe >> "%RES%" 2>&1
if exist build\Release\results_omp.exe (
  echo [results] Running results_omp ...
  echo ===== results_omp ===== >> "%RES%"
  build\Release\results_omp.exe >> "%RES%" 2>&1
)
echo [results] Running results_gcbig (about a minute) ...
echo ===== results_gcbig ===== >> "%RES%"
build\Release\results_gcbig.exe >> "%RES%" 2>&1
echo ===== done %TIME% ===== >> "%RES%"

echo. >> "%LOG%"
type "%RES%" >> "%LOG%"
echo.
type "%RES%"
del "%RES%"
echo.
echo [results] Done. Also saved to images\output\results\windows_output.txt
echo [results] Figures: images\output\results\
pause
exit /b 0

:failed
echo ===== BUILD FAILED ===== >> "%LOG%"
echo [results] Build failed. See images\output\results\windows_output.txt
pause
exit /b 1
