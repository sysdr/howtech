@echo off
REM Load the echo driver - Run as Administrator

set DRIVER_PATH=%~dp0echosample\x64\Debug\echosample.sys
set SERVICE_NAME=EchoSample

echo Stopping existing service if running...
sc stop %SERVICE_NAME% >nul 2>&1
timeout /t 2 /nobreak >nul
sc delete %SERVICE_NAME% >nul 2>&1

echo Creating service...
sc create %SERVICE_NAME% type=kernel binPath=%DRIVER_PATH%

if %ERRORLEVEL% NEQ 0 (
    echo Failed to create service!
    exit /b 1
)

echo Starting service...
sc start %SERVICE_NAME%

if %ERRORLEVEL% EQU 0 (
    echo.
    echo Driver loaded successfully!
    echo.
    echo Run test_echo.exe to test the driver
) else (
    echo.
    echo Warning: Service created but may have failed to start
    echo Check Event Viewer for details
)
