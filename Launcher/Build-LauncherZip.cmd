@echo off
where pwsh.exe >nul 2>nul
if errorlevel 1 (
  echo PowerShell 7 is required to deploy Starshatter Wars.
  pause
  exit /b 1
)
pushd "%~dp0.."
pwsh.exe -NoLogo -NoProfile -File "%~dp0Publish-LauncherBuild.ps1" %*
set "publishResult=%ERRORLEVEL%"
popd
if not "%publishResult%"=="0" pause
exit /b %publishResult%
