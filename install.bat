@echo off
REM gleditor Installation Script for Windows
REM Requires MSYS2/MinGW or similar environment

setlocal enabledelayedexpansion

echo.
echo ====================================
echo   gleditor Installation for Windows
echo ====================================
echo.

REM Check if running in MSYS2/MinGW environment
where bash >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: MSYS2/MinGW environment not detected!
    echo.
    echo gleditor requires MSYS2 for building on Windows.
    echo Please install MSYS2 from: https://www.msys2.org/
    echo.
    echo After installing MSYS2:
    echo   1. Open MSYS2 MinGW 64-bit terminal
    echo   2. Install dependencies:
    echo      pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-make
    echo      pacman -S mingw-w64-x86_64-gtk3 mingw-w64-x86_64-gtksourceview4
    echo      pacman -S mingw-w64-x86_64-pkg-config
    echo   3. Navigate to gleditor directory
    echo   4. Run: ./install.sh
    echo.
    pause
    exit /b 1
)

echo Detected MSYS2/MinGW environment
echo.

REM Check for required tools
echo Checking for required tools...

where gcc >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: gcc not found!
    echo Please install mingw-w64-x86_64-gcc
    exit /b 1
)
echo   [OK] gcc found

where make >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: make not found!
    echo Please install mingw-w64-x86_64-make
    exit /b 1
)
echo   [OK] make found

where pkg-config >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: pkg-config not found!
    echo Please install mingw-w64-x86_64-pkg-config
    exit /b 1
)
echo   [OK] pkg-config found

echo.
echo Checking for GTK and dependencies...

pkg-config --exists gtk+-3.0
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: GTK+3 not found!
    echo Please install: pacman -S mingw-w64-x86_64-gtk3
    exit /b 1
)
for /f "tokens=*" %%i in ('pkg-config --modversion gtk+-3.0') do set GTK_VERSION=%%i
echo   [OK] GTK+3 version: %GTK_VERSION%

pkg-config --exists gtksourceview-4
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: GTKSourceView 4 not found!
    echo Please install: pacman -S mingw-w64-x86_64-gtksourceview4
    exit /b 1
)
echo   [OK] GTKSourceView 4 found

echo.
echo Building gleditor...
echo.

REM Clean previous build
if exist bin\gleditor.exe del /q bin\gleditor.exe
if exist build rmdir /s /q build

REM Build using CMake if available, otherwise use Make
where cmake >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo Using CMake build system...
    mkdir build
    cd build
    cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..
    cmake --build . --config Release
    cd ..

    if exist build\gleditor.exe (
        echo.
        echo [SUCCESS] Build completed!
        echo.

        REM Copy executable to bin directory
        if not exist bin mkdir bin
        copy /y build\gleditor.exe bin\gleditor.exe

        REM Copy required DLLs
        echo Copying required DLLs...
        for %%f in (
            libgcc_s_seh-1.dll
            libstdc++-6.dll
            libwinpthread-1.dll
            libgtk-3-0.dll
            libgdk-3-0.dll
            libglib-2.0-0.dll
            libgobject-2.0-0.dll
            libgio-2.0-0.dll
            libcairo-2.0.dll
            libpango-1.0-0.dll
            libgtksourceview-4-0.dll
        ) do (
            where %%f >nul 2>&1
            if !ERRORLEVEL! EQU 0 (
                for /f "tokens=*" %%p in ('where %%f') do (
                    copy /y "%%p" bin\ >nul 2>&1
                )
            )
        )
    ) else (
        echo [ERROR] Build failed!
        exit /b 1
    )
) else (
    echo CMake not found, using Makefile...
    make clean
    make

    if exist bin\gleditor.exe (
        echo.
        echo [SUCCESS] Build completed!
        echo.
    ) else (
        echo [ERROR] Build failed!
        exit /b 1
    )
)

echo.
echo ====================================
echo   Installation Complete!
echo ====================================
echo.
echo To run gleditor:
echo   cd bin
echo   gleditor.exe
echo.
echo Or add the bin directory to your PATH.
echo.
echo Configuration files will be saved to:
echo   %%APPDATA%%\gleditor
echo.
echo Happy shader coding! ^_^
echo.

pause
