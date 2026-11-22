param(
    [string]$EngineRoot = 'D:\UnrealEngine\UnrealEngine-5.7.4',
    [string]$Cooker = 'C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
)
$ErrorActionPreference = 'Stop'
$ProjectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$ProjectFile = Join-Path $ProjectRoot 'StarshatterWars.uproject'
$ArchiveRoot = Join-Path $ProjectRoot 'Build/FractalVerse'
$CookRoot = Join-Path $ProjectRoot 'Saved/Cooked/FractalVerse/WindowsServer'
$StageRoot = Join-Path $ProjectRoot 'Saved/StagedBuilds/FractalVerse'
$LogPath = Join-Path $ProjectRoot ('Saved/Logs/FractalVersePackage-' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '.log')
# Use the installed editor for cooking so the owner's installed-engine client
# editor module is not overwritten by a source-engine editor build.
$Options = @(
    'BuildCookRun', ('-project=' + $ProjectFile), '-target=FractalVerse',
    '-server', '-noclient', '-serverplatform=Win64', '-serverconfig=Development',
    '-skipbuild', '-nocompileeditor', '-cook', '-stage', '-pak', '-archive',
    ('-archivedirectory=' + $ArchiveRoot), ('-stagingdirectory=' + $StageRoot),
    ('-CookOutputDir=' + $CookRoot), ('-unrealexe=' + $Cooker),
    '-map=/Game/Maps/Transition', '-SkipCookingEditorContent', '-nodebuginfo',
    '-nobootstrapexe', '-unattended', '-utf8output', '-nop4',
    '-AdditionalCookerOptions=-NullRHI -CookProcessCount=1 -CustomConfig=FractalVerse -DisablePlugins=Landmass -SkipSoftReferences'
)
Push-Location $ProjectRoot
try {
    & (Join-Path $EngineRoot 'Engine/Build/BatchFiles/Build.bat') FractalVerse Win64 Development $ProjectFile -WaitMutex -MaxParallelActions=4 -NoHotReloadFromIDE
    if ($LASTEXITCODE -ne 0) { throw 'FractalVerse server compilation failed.' }
    & (Join-Path $EngineRoot 'Engine/Build/BatchFiles/RunUAT.bat') @Options 2>&1 | Tee-Object -FilePath $LogPath
    if ($LASTEXITCODE -ne 0) { throw ('Server cooking or packaging failed. See ' + $LogPath) }
    $Executable = Join-Path $ArchiveRoot 'WindowsServer/StarshatterWars/Binaries/Win64/FractalVerse.exe'
    if (!(Test-Path -LiteralPath $Executable)) { throw ('Expected server executable missing: ' + $Executable) }
    Write-Output ('FractalVerse server package ready: ' + $ArchiveRoot)
} finally { Pop-Location }
