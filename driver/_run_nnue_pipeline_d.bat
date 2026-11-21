@echo off
setlocal EnableExtensions DisableDelayedExpansion

rem Run the directory pipeline with existing executables; change Release to use another build.
set "BIN_DIR=%~dp0..\build\bin\Release"

rem Resolve relative data paths from the repository root, wherever this is run.
pushd "%~dp0.."
if errorlevel 1 (
    >&2 echo ERROR: Could not change to the repository root.
    exit /b 1
)

set "STAGE=append_fens_from_dir.exe"
echo(
echo [1/3] Appending FENs from the PGN directory...
"%BIN_DIR%\%STAGE%"
set "EXIT_CODE=%ERRORLEVEL%"
rem Exit code 2 means some files were skipped, but usable FENs were appended.
if "%EXIT_CODE%"=="2" (
    echo WARNING: Some PGN files failed and were skipped; see the collector's list above.
    echo Continuing with FENs from successfully collected files.
) else (
    if not "%EXIT_CODE%"=="0" goto :failed
)

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
