@echo off
rem Double-click this file, or run it from any terminal. It takes no arguments.
rem It generates the Visual Studio solution in build\ and opens it.
rem Running the resizer, the benchmark and the tests is done from Visual
rem Studio: pick the startup project and press F5 (see README.md).
setlocal
cd /d "%~dp0"

rem --- Locate Visual Studio (used to find its bundled CMake and vcpkg). ---
set "VSDIR="
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" goto find_cmake
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -property installationPath`) do set "VSDIR=%%i"

:find_cmake
rem --- CMake: the copy bundled with Visual Studio first (it always knows
rem --- that Visual Studio version), then whatever is on PATH.
if not defined VSDIR goto path_cmake
set "CMAKE=%VSDIR%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if exist "%CMAKE%" goto find_vcpkg
:path_cmake
set "CMAKE=cmake"
where cmake >nul 2>nul
if errorlevel 1 goto no_cmake

:find_vcpkg
rem --- vcpkg: VCPKG_ROOT, then C:\dev\vcpkg, then the Visual Studio copy. ---
set "TOOLCHAIN="
if defined VCPKG_ROOT set "TOOLCHAIN=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake"
if defined TOOLCHAIN if exist "%TOOLCHAIN%" goto configure
set "TOOLCHAIN=C:\dev\vcpkg\scripts\buildsystems\vcpkg.cmake"
if exist "%TOOLCHAIN%" goto configure
if not defined VSDIR goto no_vcpkg
set "TOOLCHAIN=%VSDIR%\VC\vcpkg\scripts\buildsystems\vcpkg.cmake"
if not exist "%TOOLCHAIN%" goto no_vcpkg

:configure
echo [run] Generating the Visual Studio solution in build\ ...
echo [run] The first run compiles OpenCV and can take a long time.
"%CMAKE%" -B build -S . "-DCMAKE_TOOLCHAIN_FILE=%TOOLCHAIN%"
if errorlevel 1 goto failed

set "SLN=build\seam_carving.sln"
if not exist "%SLN%" set "SLN=build\seam_carving.slnx"
if not exist "%SLN%" goto no_sln
echo [run] Opening %SLN% ...
start "" "%SLN%"
exit /b 0

:no_cmake
echo [run] CMake was not found.
echo       Install Visual Studio 2022 with the "Desktop development with C++" workload.
goto fail_exit

:no_vcpkg
echo [run] vcpkg was not found.
echo       Install it as described in README.md, or set VCPKG_ROOT to its folder.
goto fail_exit

:no_sln
echo [run] CMake finished but no solution file was created in build\.
echo       Delete the build folder and run this again with Visual Studio installed.
goto fail_exit

:failed
echo [run] CMake failed. See the messages above.

:fail_exit
pause
exit /b 1
