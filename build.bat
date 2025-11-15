@echo off
REM gleditor Build Script for Windows
REM Builds the application without installing

setlocal enabledelayedexpansion

REM Colors using ANSI escape codes (Windows 10+)
set "GREEN=[92m"
set "YELLOW=[93m"
set "RED=[91m"
set "BLUE=[94m"
set "NC=[0m"

echo.
echo ========================================
echo       gleditor Build Script
echo ========================================
echo.
echo Platform: Windows
echo.

REM Parse command line arguments
set BUILD_TYPE=Release
set CLEAN_BUILD=0
set USE_CMAKE=0
set USE_MAKE=0
set VERBOSE=0

:parse_args
if "%~1"=="" goto args_done
if /i "%~1"=="--debug" (
    set BUILD_TYPE=Debug
    shift
    goto parse_args
)
if /i "%~1"=="--clean" (
    set CLEAN_BUILD=1
    shift
    goto parse_args
)
if /i "%~1"=="--cmake" (
    set USE_CMAKE=1
    shift
    goto parse_args
)
if /i "%~1"=="--make" (
    set USE_MAKE=1
    shift
    goto parse_args
)
if /i "%~1"=="--verbose" (
    set VERBOSE=1
    shift
    goto parse_args
)
if /i "%~1"=="-v" (
    set VERBOSE=1
    shift
    goto parse_args
)
if /i "%~1"=="--help" goto show_help
if /i "%~1"=="-h" goto show_help
echo %RED%Unknown option: %~1%NC%
echo Use --help for usage information
exit /b 1

:show_help
echo Usage: build.bat [OPTIONS]
echo.
echo Options:
echo   --debug         Build with debug symbols (default: Release)
echo   --clean         Clean before building
echo   --cmake         Force use of CMake
echo   --make          Force use of Make
echo   --verbose, -v   Verbose output
echo   --help, -h      Show this help message
echo.
echo Examples:
echo   build.bat                  # Quick build (auto-detect best system)
echo   build.bat --debug          # Debug build
echo   build.bat --clean --cmake  # Clean CMake build
echo   build.bat --make -v        # Verbose Make build
exit /b 0

:args_done

REM Check for MSYS2/MinGW environment
where bash >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo %YELLOW%Note: For best results, run this in MSYS2 MinGW terminal%NC%
    echo %YELLOW%Or use: bash build.sh%NC%
    echo.
)

REM Auto-select build system if not forced
if %USE_CMAKE%==0 if %USE_MAKE%==0 (
    REM Windows: prefer CMake
    where cmake >nul 2>&1
    if !ERRORLEVEL! EQU 0 (
        set USE_CMAKE=1
    ) else (
        where make >nul 2>&1
        if !ERRORLEVEL! EQU 0 (
            set USE_MAKE=1
        )
    )
)

REM Build with Make
if %USE_MAKE%==1 (
    echo %BLUE%Building with Make...%NC%
    echo Build type: %BUILD_TYPE%
    echo.

    if not exist Makefile (
        echo %RED%Error: Makefile not found!%NC%
        exit /b 1
    )

    where make >nul 2>&1
    if %ERRORLEVEL% NEQ 0 (
        echo %RED%Error: make not found!%NC%
        echo Install MSYS2 and run: pacman -S make
        exit /b 1
    )

    REM Clean if requested
    if %CLEAN_BUILD%==1 (
        echo %BLUE%Cleaning previous build...%NC%
        make clean
    )

    REM Build
    if "%BUILD_TYPE%"=="Debug" (
        if %VERBOSE%==1 (
            make debug V=1
        ) else (
            make debug
        )
    ) else (
        if %VERBOSE%==1 (
            make V=1
        ) else (
            make
        )
    )

    if %ERRORLEVEL% EQU 0 (
        echo.
        echo %GREEN%Build successful!%NC%
        echo Binary: bin\gleditor.exe
        echo.
        echo Run with: bin\gleditor.exe
    ) else (
        echo %RED%Build failed!%NC%
        exit /b 1
    )
    goto end
)

REM Build with CMake
if %USE_CMAKE%==1 (
    echo %BLUE%Building with CMake...%NC%
    echo Build type: %BUILD_TYPE%
    echo.

    where cmake >nul 2>&1
    if %ERRORLEVEL% NEQ 0 (
        echo %RED%Error: CMake not found!%NC%
        echo.
        echo Install CMake:
        echo   - Download from: https://cmake.org/download/
        echo   - Or with MSYS2: pacman -S mingw-w64-x86_64-cmake
        exit /b 1
    )

    REM Clean if requested
    if %CLEAN_BUILD%==1 (
        echo %BLUE%Cleaning previous build...%NC%
        if exist build rmdir /s /q build
    )

    REM Create build directory
    if not exist build mkdir build
    cd build

    REM Detect generator
    where ninja >nul 2>&1
    if %ERRORLEVEL% EQU 0 (
        set CMAKE_GENERATOR=-G "Ninja"
    ) else (
        where mingw32-make >nul 2>&1
        if %ERRORLEVEL% EQU 0 (
            set CMAKE_GENERATOR=-G "MinGW Makefiles"
        ) else (
            set CMAKE_GENERATOR=-G "Unix Makefiles"
        )
    )

    REM Configure
    echo %BLUE%Configuring...%NC%
    if %VERBOSE%==1 (
        cmake %CMAKE_GENERATOR% -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DCMAKE_VERBOSE_MAKEFILE=ON ..
    ) else (
        cmake %CMAKE_GENERATOR% -DCMAKE_BUILD_TYPE=%BUILD_TYPE% .. >nul 2>&1
    )

    if %ERRORLEVEL% NEQ 0 (
        echo %RED%CMake configuration failed!%NC%
        cd ..
        exit /b 1
    )

    REM Build
    echo %BLUE%Building...%NC%
    if %VERBOSE%==1 (
        cmake --build . --config %BUILD_TYPE%
    ) else (
        cmake --build . --config %BUILD_TYPE% 2>&1 | findstr /i "error" || echo Building...
    )

    if %ERRORLEVEL% EQU 0 (
        cd ..
        echo.
        echo %GREEN%Build successful!%NC%

        REM Find the binary
        if exist build\gleditor.exe (
            echo Binary: build\gleditor.exe
            echo.
            echo Run with: build\gleditor.exe
        ) else if exist build\%BUILD_TYPE%\gleditor.exe (
            echo Binary: build\%BUILD_TYPE%\gleditor.exe
            echo.
            echo Run with: build\%BUILD_TYPE%\gleditor.exe
        ) else if exist bin\gleditor.exe (
            echo Binary: bin\gleditor.exe
            echo.
            echo Run with: bin\gleditor.exe
        ) else (
            echo Binary location may vary
        )
    ) else (
        cd ..
        echo %RED%Build failed!%NC%
        exit /b 1
    )
    goto end
)

REM No build system available
echo %RED%Error: No build system available!%NC%
echo.
echo Please install one of the following:
echo   - CMake: https://cmake.org/download/
echo   - Make (via MSYS2): pacman -S make
echo.
echo Recommended: Use MSYS2 MinGW terminal and run:
echo   bash build.sh
exit /b 1

:end
echo.
echo ========================================
echo      Build Complete!
echo ========================================
echo.

if "%BUILD_TYPE%"=="Debug" (
    echo %YELLOW%Note: Debug build includes symbols and is slower%NC%
    echo %YELLOW%For production use, build without --debug%NC%
    echo.
)

endlocal
