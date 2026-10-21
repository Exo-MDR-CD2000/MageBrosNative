@echo off
rem Builds MageBrosNative in Release and packages build\MageBrosNative-<version>-windows-x64.zip.
rem Needs Visual Studio 2022 or newer with the "Desktop development with C++" workload.
setlocal
cd /d "%~dp0"

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
  echo Visual Studio was not found. Install it with the "Desktop development with C++" workload.
  exit /b 1
)
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSDIR=%%i"
if not defined VSDIR (
  echo Visual Studio has no C++ tools. Add the "Desktop development with C++" workload.
  exit /b 1
)
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1

cmake -S . -B build -A x64 || exit /b 1
cmake --build build --config Release || exit /b 1
ctest --test-dir build -C Release --output-on-failure || exit /b 1
cd build && cpack -C Release || exit /b 1
echo.
echo Done: the zip is in the build folder.
