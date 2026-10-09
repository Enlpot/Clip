@echo off
setlocal
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0run-clip-draw-engine.ps1" %*
set "exit_code=%ERRORLEVEL%"
endlocal & exit /b %exit_code%
