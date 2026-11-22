@echo off
where pwsh.exe >nul 2>nul
if errorlevel 1 (
  echo PowerShell 7 is required.
  pause
  exit /b 1
)
pushd "%~dp0.."
pwsh.exe -NoLogo -NoProfile -File "%~dp0Build-Cook-Deploy.ps1" %*
set "buildResult=%ERRORLEVEL%"
popd
if not defined SSW_NO_PAUSE pause
exit /b %buildResult%
