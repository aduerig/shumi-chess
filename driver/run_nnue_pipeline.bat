@echo off
setlocal EnableExtensions DisableDelayedExpansion

rem Run the existing executables; change Release here to use another build.
set "BIN_DIR=%~dp0..\build\bin\Release"

rem Resolve relative data paths from the repository root, wherever this is run.
pushd "%~dp0.."
if errorlevel 1 (
    >&2 echo ERROR: Could not change to the repository root.
    exit /b 1
)

set "STAGE=append_fens_from_pgn_file.exe"
echo(
echo [1/3] Appending FENs from PGN...
"%BIN_DIR%\%STAGE%"
set "EXIT_CODE=%ERRORLEVEL%"
if not "%EXIT_CODE%"=="0" goto :failed

set "STAGE=evaluate_fens.exe"
echo(
echo [2/3] Evaluating collected FENs...
"%BIN_DIR%\%STAGE%"
set "EXIT_CODE=%ERRORLEVEL%"
if not "%EXIT_CODE%"=="0" goto :failed

set "STAGE=FENScores_to_model.exe"
echo(
echo [3/3] Training the NNUE model...
"%BIN_DIR%\%STAGE%"
set "EXIT_CODE=%ERRORLEVEL%"
if not "%EXIT_CODE%"=="0" goto :failed

echo Pipeline complete: NNUE\models\Model_temp.txt
popd
exit /b 0

:failed
>&2 echo ERROR: %STAGE% failed with exit code %EXIT_CODE%.
>&2 echo Pipeline stopped.
popd
exit /b %EXIT_CODE%
