@echo off
REM Compile the test application - Run on Windows

echo Compiling test application...
cl.exe /nologo /Fe:test_echo.exe test_echo.c

if %ERRORLEVEL% EQU 0 (
    echo.
    echo Test application compiled: test_echo.exe
    echo Run it to test the driver
) else (
    echo.
    echo Compilation failed!
    echo Make sure Visual Studio command prompt environment is set
)
