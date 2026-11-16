@echo off
rem Double-click this file, or run it from any terminal. It takes no arguments.
rem It runs clean.bat, then run.bat: the build\ folder is deleted, the
rem Visual Studio solution is generated again from scratch and opened.
setlocal
cd /d "%~dp0"

set "SEAM_NO_PAUSE=1"
call "%~dp0clean.bat"
if errorlevel 1 goto failed
set "SEAM_NO_PAUSE="

call "%~dp0run.bat"
exit /b %errorlevel%

:failed
echo [rebuild] Clean failed, so nothing was regenerated.
pause
exit /b 1
