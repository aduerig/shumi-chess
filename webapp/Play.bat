@echo off
setlocal
title Shumi Chess
pushd "%~dp0"
if errorlevel 1 (
  echo Could not open the Shumi Chess application folder.
  pause
  exit /b 1
)

rem Double-click to play. Extra command-line options override these defaults.
call npm.cmd start -- -d9 -t5000 -r2 %*
set "shumiExitCode=%ERRORLEVEL%"
if not "%shumiExitCode%"=="0" (
  echo.
  echo Shumi Chess could not start. See the error above.
  echo If npm is not recognized, install Node.js and reopen this launcher.
  pause
)
popd
exit /b %shumiExitCode%
