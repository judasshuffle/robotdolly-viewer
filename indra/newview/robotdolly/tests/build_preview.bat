@echo off
setlocal
cd /d "%~dp0"
where cl >nul 2>nul
if errorlevel 1 (
  echo ERROR: Run this from "x64 Native Tools Command Prompt for VS 2022".
  pause
  exit /b 1
)
echo Building ROBOT DOLLY Workspace Preview...
cl /nologo /std:c++17 /EHsc /W4 workspace_preview.cpp /link user32.lib gdi32.lib /SUBSYSTEM:WINDOWS /OUT:workspace_preview.exe
if errorlevel 1 (
  echo Build failed. Copy the error messages into ChatGPT.
  pause
  exit /b 1
)
echo Build succeeded. Starting workspace preview...
start "" "%~dp0workspace_preview.exe"
endlocal
