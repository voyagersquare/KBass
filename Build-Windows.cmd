@echo off
setlocal
cd /d "%~dp0"
if /i "%~1"=="--no-pause" set "KBASS_NO_PAUSE=1"
set "KBASS_VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%KBASS_VSWHERE%" goto :missingvs
for /f "usebackq tokens=*" %%I in (`"%KBASS_VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "KBASS_VS=%%I"
if not defined KBASS_VS goto :missingvs
call "%KBASS_VS%\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
if errorlevel 1 goto :failed
set "PATH=%KBASS_VS%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;%KBASS_VS%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;%PATH%"
where cmake >nul 2>&1
if errorlevel 1 goto :missingtools
where ninja >nul 2>&1
if errorlevel 1 goto :missingtools
cmake --preset windows-x64
if errorlevel 1 goto :failed
cmake --build --preset windows-release --target KBass_VST3 KBassDspTests KBassProcessorTests KBassUserPresetTests --parallel 2
if errorlevel 1 goto :failed
ctest --preset windows-release
if errorlevel 1 goto :failed
if not exist "build\windows-x64\KBass_artefacts\Release\VST3\KBass.vst3\Contents\x86_64-win\KBass.vst3" goto :failed
echo.
echo BUILD SUCCEEDED: build\windows-x64\KBass_artefacts\Release\VST3\KBass.vst3
if not defined KBASS_NO_PAUSE pause
exit /b 0
:missingvs
echo Install Visual Studio with Desktop development with C++.
goto :failed
:missingtools
echo Install the C++ CMake tools for Windows component in Visual Studio Installer.
goto :failed
:failed
echo Build failed. See the error above.
if not defined KBASS_NO_PAUSE pause
exit /b 1
