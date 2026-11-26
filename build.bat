@echo off
rem Configure + build with MSVC (VS 2022 Build Tools) and Ninja.
rem Usage: build.bat [Debug|Release]   (default Release)
setlocal
set CONFIG=%1
if "%CONFIG%"=="" set CONFIG=Release

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set VSDIR=
for /f "usebackq tokens=*" %%i in (`call "%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSDIR=%%i"
if not defined VSDIR (echo Visual Studio C++ tools not found & exit /b 1)
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1

cmake -S . -B build\%CONFIG% -G Ninja -DCMAKE_BUILD_TYPE=%CONFIG% || exit /b 1
cmake --build build\%CONFIG% || exit /b 1
echo.
echo Built build\%CONFIG%\Solitaire.exe
