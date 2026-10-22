@echo off
rem Builds MageBrosNative in Release and packages build\MageBrosNative-<version>-windows-x64.zip.
rem Needs Visual Studio 2022 or newer with the "Desktop development with C++" workload.
rem
rem Each run first raises the version in CMakeLists.txt (the single source of the version number):
rem   make_release.bat          patch: 0.1.0 -> 0.1.1 (also when double-clicked)
rem   make_release.bat minor    0.1.3 -> 0.2.0
rem   make_release.bat major    0.2.0 -> 1.0.0
rem   make_release.bat none     rebuild at the current version
rem If the build or a test fails, the version is put back. Commit CMakeLists.txt with the release.
rem When ..\MageBros.exe is found, the map and hero tests run on all 40 levels too.
setlocal
cd /d "%~dp0"
set "RESULT=0"
set "OLDVER="

set "BUMP=%~1"
if "%BUMP%"=="" set "BUMP=patch"
if /i not "%BUMP%"=="patch" if /i not "%BUMP%"=="minor" if /i not "%BUMP%"=="major" if /i not "%BUMP%"=="none" (
  echo Unknown option "%BUMP%". Use patch, minor, major or none.
  goto :error
)

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
  echo Visual Studio was not found. Install it with the "Desktop development with C++" workload.
  goto :error
)
set "VSDIR="
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSDIR=%%i"
if not defined VSDIR (
  echo Visual Studio has no C++ tools. Add the "Desktop development with C++" workload.
  goto :error
)
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul || goto :error

rem Read the version from project(MageBrosNative VERSION x.y.z ...) and raise it.
for /f "usebackq delims=" %%v in (`powershell -NoProfile -Command "if ((Get-Content -Raw CMakeLists.txt) -match 'project\(MageBrosNative VERSION (\d+\.\d+\.\d+)') { $matches[1] }"`) do set "OLDVER=%%v"
if not defined OLDVER (
  echo Could not find "project(MageBrosNative VERSION x.y.z" in CMakeLists.txt.
  goto :error
)
for /f "tokens=1-3 delims=." %%a in ("%OLDVER%") do (
  set /a "MAJ=%%a, MIN=%%b, PAT=%%c"
)
if /i "%BUMP%"=="patch" set /a "PAT+=1"
if /i "%BUMP%"=="minor" set /a "MIN+=1, PAT=0"
if /i "%BUMP%"=="major" set /a "MAJ+=1, MIN=0, PAT=0"
set "NEWVER=%MAJ%.%MIN%.%PAT%"
if /i "%BUMP%"=="none" (
  echo Version %OLDVER% ^(unchanged^)
  set "OLDVER="
) else (
  call :setversion %NEWVER% || goto :error
  echo Version %OLDVER% -^> %NEWVER%
)

rem The game-file tests need the user's MageBros.exe, which sits next to the repository.
set "GAME="
if exist "..\MageBros.exe" for %%f in ("..\MageBros.exe") do set "GAME=%%~ff"
if defined GAME (
  set "GAME=%GAME:\=/%"
  echo Found MageBros.exe: the map and hero tests will run too.
) else (
  echo ..\MageBros.exe not found: running only the tests that need no game files.
)

cmake -S . -B build -A x64 "-DMBN_GAME_EXE=%GAME%" || goto :failed
cmake --build build --config Release || goto :failed
ctest --test-dir build -C Release --output-on-failure || goto :failed
pushd build
cpack -C Release
if errorlevel 1 (popd & goto :failed)
popd
echo.
echo Done: build\MageBrosNative-%NEWVER%-windows-x64.zip
goto :done

:failed
if defined OLDVER (
  call :setversion %OLDVER%
  echo Version put back to %OLDVER%.
)
:error
echo Release failed.
set "RESULT=1"
:done
pause
exit /b %RESULT%

rem Writes the given x.y.z into the project() line, keeping the file's encoding and line endings.
:setversion
powershell -NoProfile -Command "$t = [IO.File]::ReadAllText('CMakeLists.txt'); $t = [regex]::Replace($t, '(project\(MageBrosNative VERSION )\d+\.\d+\.\d+', '${1}%1'); [IO.File]::WriteAllText('CMakeLists.txt', $t, (New-Object Text.UTF8Encoding $false))"
exit /b %errorlevel%
