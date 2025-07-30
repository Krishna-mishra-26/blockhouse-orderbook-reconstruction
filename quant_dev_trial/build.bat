@echo off
echo Building OrderBook Reconstruction...

if "%1"=="clean" (
    echo Cleaning build artifacts...
    del reconstruction_blockhouse.exe 2>nul
    del test_suite.exe 2>nul
    del reconstructed_mbp.csv 2>nul
    del *.log 2>nul
    echo Clean complete.
    goto :end
)

if "%1"=="test" (
    echo Building and running tests...
    g++ -std=c++17 -O3 -Wall -Wextra -o test_suite.exe test_suite.cpp
    if errorlevel 1 (
        echo Test build failed!
        goto :end
    )
    test_suite.exe
    goto :end
)

if "%1"=="help" (
    echo Available commands:
    echo   build.bat          - Build reconstruction executable
    echo   build.bat clean    - Clean build artifacts
    echo   build.bat test     - Build and run test suite
    echo   build.bat run      - Build and run with mbo.csv
    echo   build.bat help     - Show this help
    goto :end
)

if "%1"=="run" (
    echo Building reconstruction executable...
    g++ -std=c++17 -O3 -Wall -Wextra -o reconstruction_blockhouse.exe reconstruction.cpp
    if errorlevel 1 (
        echo Build failed!
        goto :end
    )
    echo Running reconstruction on mbo.csv...
    reconstruction_blockhouse.exe mbo.csv
    goto :end
)

rem Default: just build
echo Building reconstruction executable...
g++ -std=c++17 -O3 -Wall -Wextra -o reconstruction_blockhouse.exe reconstruction.cpp
if errorlevel 1 (
    echo Build failed!
) else (
    echo Build successful! Use: reconstruction_blockhouse.exe mbo.csv
)

:end
