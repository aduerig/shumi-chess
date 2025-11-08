param(
    [string]$Config = "Release"
)

cmake --build build --config $Config

$exePath = ".\build\bin\$Config\shumi_driver.exe"

# forward ALL extra args exactly as typed
& $exePath @args
