@echo off
REM Build script for Windows - Run this on Windows with WDK installed

echo Building Echo Driver with MSBuild...
msbuild echosample\echosample.vcxproj /p:Configuration=Debug /p:Platform=x64 /verbosity:minimal

if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    exit /b 1
)

echo.
echo Build successful!
echo Driver: echosample\x64\Debug\echosample.sys
echo PDB:    echosample\x64\Debug\echosample.pdb
echo.
echo Next steps:
echo 1. Enable test signing: bcdedit /set testsigning on
echo 2. Enable debug mode:   bcdedit /debug on
echo 3. Reboot
echo 4. Run load-driver.cmd
