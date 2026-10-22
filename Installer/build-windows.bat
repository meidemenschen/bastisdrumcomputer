@echo off
rem ---------------------------------------------------------------------------
rem Bastis Drumcomputer: baut VST3 + Standalone und erzeugt die Setup-Datei.
rem Voraussetzungen: Visual Studio 2022 (C++), CMake, Git, Inno Setup 6
rem Aufruf: Doppelklick oder im Projektordner  Installer\build-windows.bat
rem ---------------------------------------------------------------------------
setlocal
cd /d "%~dp0.."

cmake -B build -G "Visual Studio 17 2022" -A x64 || goto :error
cmake --build build --config Release || goto :error

set "ISCC=%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe"
if not exist "%ISCC%" set "ISCC=%ProgramFiles%\Inno Setup 6\ISCC.exe"
if not exist "%ISCC%" set "ISCC=%LocalAppData%\Programs\Inno Setup 6\ISCC.exe"
if not exist "%ISCC%" (
    echo Inno Setup 6 nicht gefunden. Download: https://jrsoftware.org/isdl.php
    goto :error
)

"%ISCC%" Installer\BastisDrumcomputer.iss || goto :error

echo.
echo Fertig! Die Setup-Datei liegt in Installer\Output\
pause
exit /b 0

:error
echo.
echo Es ist ein Fehler aufgetreten.
pause
exit /b 1
