#Requires -Version 7.0
[CmdletBinding()]
param(
    [string]$ProjectRoot = (Join-Path $PSScriptRoot '..'),
    [string]$EngineRoot = 'C:/Program Files/Epic Games/UE_5.7',
    [string]$EditorCommand = '',
    [string]$StarServerPackage = '',
    [string]$EconomyServerPackage = '',
    [string]$FeedRoot = '',
    [string]$PrivateKey = '',
    [string]$LauncherConfig = (Join-Path $PSScriptRoot 'launcher.localhost.json'),
    [ValidateLength(0,500)][string]$About = '',
    [switch]$PlanOnly,
    [switch]$KeepStaging
)
$ErrorActionPreference='Stop'
$PSNativeCommandUseErrorActionPreference=$false
$projectPath=(Resolve-Path -LiteralPath $ProjectRoot).Path
$projectFile=Join-Path $projectPath 'StarshatterWars.uproject'
$buildTool=Join-Path $EngineRoot 'Engine/Build/BatchFiles/Build.bat'
$uatTool=Join-Path $EngineRoot 'Engine/Build/BatchFiles/RunUAT.bat'
if(!$EditorCommand){$EditorCommand=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'}
if(!$StarServerPackage){$StarServerPackage=Join-Path $projectPath 'Build/StarServer/WindowsServer'}
if(!$FeedRoot){$FeedRoot=Join-Path $projectPath 'Build/LauncherFeed'}
if(!$PrivateKey){$PrivateKey=Join-Path $projectPath 'Saved/SSWLocalReleaseKey.pem'}
foreach($required in @($projectFile,$buildTool,$uatTool,$EditorCommand,$PrivateKey,$LauncherConfig,(Join-Path $StarServerPackage 'StarshatterWars/Binaries/Win64/StarServer.exe'))) {
    if(!(Test-Path -LiteralPath $required -PathType Leaf)){throw "Required file is missing: $required"}
}
$repo=(& git -C $projectPath rev-parse --show-toplevel 2>$null)
if($LASTEXITCODE -ne 0 -or ![IO.Path]::GetFullPath($repo).TrimEnd('\','/').Equals($projectPath.TrimEnd('\','/'),[StringComparison]::OrdinalIgnoreCase)) {throw 'ProjectRoot must be the SSW Git repository root so the build can be recorded safely.'}
$sourceCommit=(& git -C $projectPath rev-parse HEAD).Trim()
$buildRoot=[IO.Path]::GetFullPath((Join-Path $projectPath 'Build'))
$package=Join-Path $buildRoot 'Windows'
$sourceIcon=Join-Path $projectPath 'Application.ico'
if(!(Test-Path -LiteralPath $sourceIcon -PathType Leaf)){throw 'The project Application.ico is missing.'}
Write-Host 'STARSHATTER WARS - BUILD / TEST / COOK / PACKAGE / DEPLOY'
Write-Host "Project: $projectPath"
Write-Host "Output:  $package"
Write-Host 'Uses the installed client engine and the existing packaged StarServer.'
Write-Host 'Close Unreal Editor before starting. Any failed step stops publication.'
if($PlanOnly){Write-Host 'PLAN ONLY: build Editor; run Starshatter tests; build/cook/package Development client; validate; record build; ZIP and publish. Nothing was built, committed or deployed.';return}
[void][IO.Directory]::CreateDirectory($buildRoot)
try {$pipelineLock=[IO.File]::Open((Join-Path $buildRoot '.build-cook-deploy.lock'),[IO.FileMode]::OpenOrCreate,[IO.FileAccess]::ReadWrite,[IO.FileShare]::None)}
catch {throw 'Another Build-Cook-Deploy run is already active.'}
$runStartedUtc=[DateTime]::UtcNow
$runId=[DateTime]::UtcNow.ToString('yyyyMMdd-HHmmssZ')+'-'+[guid]::NewGuid().ToString('N').Substring(0,8)
$runRoot=Join-Path $buildRoot "ReleaseStaging/$runId"
$logRoot=Join-Path $projectPath "Saved/Logs/BuildCookDeploy-$runId"
$report=Join-Path $logRoot 'Tests'
$archive=Join-Path $runRoot 'Archive'
$stage=Join-Path $runRoot 'Stage'
$previous=Join-Path $buildRoot "PreviousWindows/$runId"
function Assert-BuildChild([string]$Path) {
    $resolved=[IO.Path]::GetFullPath($Path)
    if(!$resolved.StartsWith($buildRoot.TrimEnd('\','/')+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw "Unsafe build destination: $resolved"}
    return $resolved
}
function Run-NativeStep([string]$Name,[string]$Command,[string[]]$Arguments) {
    Write-Host "`n=== $Name ==="
    $stepLog=Join-Path $logRoot ($Name.Replace(' ','-')+'.log')
    & $Command @Arguments 2>&1 | Tee-Object -FilePath $stepLog | Out-Host
    if($LASTEXITCODE -ne 0){throw "$Name failed (exit $LASTEXITCODE). See $stepLog. Nothing was deployed."}
}
try {
    [void][IO.Directory]::CreateDirectory((Assert-BuildChild $runRoot))
    [void][IO.Directory]::CreateDirectory($logRoot)
    [IO.File]::WriteAllText((Join-Path $runRoot '.pipeline-run'),$runId)
    Push-Location $projectPath
    try {
        Run-NativeStep '1 Build Editor' $buildTool @('StarshatterWarsEditor','Win64','Development',('-Project='+$projectFile),'-WaitMutex','-NoHotReloadFromIDE','-DisableAdaptiveUnity')
        Run-NativeStep '2 Automated tests' $EditorCommand @($projectFile,'-unattended','-nop4','-nosplash','-nosound','-RenderOffscreen','-ddc=InstalledNoZenLocalFallback','-ini:EditorSettings:[/Script/UnrealEd.AnalyticsPrivacySettings]:bSendUsageData=False','-ExecCmds=Automation RunTests Starshatter','-TestExit=Automation Test Queue Empty',('-ReportExportPath='+$report),('-abslog='+(Join-Path $logRoot 'GameTests.log')))
        $testFile=Join-Path $report 'index.json'
        if(!(Test-Path -LiteralPath $testFile)){throw 'Tests did not produce a report. Cooking and deployment were stopped.'}
        $tests=Get-Content -LiteralPath $testFile -Raw|ConvertFrom-Json
        foreach($field in @('failed','succeededWithWarnings','notRun','succeeded')) {
            $value=$tests.PSObject.Properties[$field]
            if(!$value -or $value.Value -isnot [ValueType] -or $value.Value -lt 0){throw "Invalid automated test report: missing or invalid $field. Nothing was deployed."}
        }
        if($tests.failed -gt 0 -or $tests.succeededWithWarnings -gt 0 -or $tests.notRun -gt 0 -or $tests.succeeded -lt 1){throw 'Tests failed, had warnings, or were incomplete. See the test report; nothing was deployed.'}
        if(!$EconomyServerPackage){
            $wire=Join-Path $projectPath 'Saved/Automation/EconomyWireFixture.json'
            if(!(Test-Path -LiteralPath $wire -PathType Leaf) -or (Get-Item -LiteralPath $wire).LastWriteTimeUtc -lt $runStartedUtc){throw 'The current Unreal tests did not produce a fresh economy snapshot. Nothing was deployed.'}
            $nativeOutput=@(& (Join-Path $projectPath 'EconomyServer/Build-Server.ps1') -ProjectRoot $projectPath)
            $EconomyServerPackage=[string]($nativeOutput|Select-Object -Last 1)
            $parityReport=Join-Path $logRoot 'NativeProjectionParity'
            Run-NativeStep '2b Native projection parity' $EditorCommand @($projectFile,'-unattended','-nop4','-nosplash','-nosound','-RenderOffscreen','-ddc=InstalledNoZenLocalFallback','-ini:EditorSettings:[/Script/UnrealEd.AnalyticsPrivacySettings]:bSendUsageData=False','-ExecCmds=Automation RunTests Starshatter.Economy.Service.ProjectionPreservesPlayer','-TestExit=Automation Test Queue Empty',('-SSWEconomyProjectionFixture='+(Join-Path $projectPath 'Saved/Automation/EconomyWireProjection.json')),('-ReportExportPath='+$parityReport),('-abslog='+(Join-Path $logRoot 'NativeProjectionParity.log')))
            $parityFile=Join-Path $parityReport 'index.json'
            if(!(Test-Path -LiteralPath $parityFile -PathType Leaf)){throw 'Native projection parity did not report results.'}
            $parity=Get-Content -LiteralPath $parityFile -Raw|ConvertFrom-Json
            if($parity.succeeded -ne 1 -or $parity.failed -ne 0 -or $parity.succeededWithWarnings -ne 0 -or $parity.notRun -ne 0){throw 'Native projection parity failed. Nothing was deployed.'}
        }
        $nativeManifest=Join-Path $EconomyServerPackage 'server-manifest.json'
        if(!(Test-Path -LiteralPath $nativeManifest -PathType Leaf)){throw 'Verified economy server package is missing. Publication stopped.'}
        $native=Get-Content -LiteralPath $nativeManifest -Raw|ConvertFrom-Json
        if(!$native.liveClientCutoverReady -or !$native.standbyAutostartReady -or $native.protocol -ne 1){throw 'Economy package has not passed live handoff tests.'}
        foreach($entry in $native.files){
            $base=[IO.Path]::GetFullPath($EconomyServerPackage).TrimEnd('\','/')+[IO.Path]::DirectorySeparatorChar
            $file=[IO.Path]::GetFullPath((Join-Path $base $entry.path))
            if(!$file.StartsWith($base,[StringComparison]::OrdinalIgnoreCase) -or !(Test-Path -LiteralPath $file -PathType Leaf) -or (Get-Item -LiteralPath $file).Length -ne $entry.length -or (Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash -ine $entry.sha256){throw 'Economy package integrity check failed.'}
        }
        # Unreal reads this path for both the compiled client and the Windows bootstrap.
        [void][IO.Directory]::CreateDirectory($package)
        Copy-Item -LiteralPath $sourceIcon -Destination (Join-Path $package 'Application.ico') -Force
        $cookArgs=@('BuildCookRun',('-project='+$projectFile),'-target=StarshatterWars','-platform=Win64','-clientconfig=Development','-build','-nocompileeditor','-cook','-stage','-pak','-iostore','-prereqs','-nodebuginfo','-archive',('-archivedirectory='+$archive),('-stagingdirectory='+$stage),('-unrealexe='+$EditorCommand),'-AdditionalCookerOptions=-ddc=InstalledNoZenLocalFallback','-unattended','-utf8output','-nop4')
        Run-NativeStep '3 Build cook and package' $uatTool $cookArgs
        $fresh=Assert-BuildChild (Join-Path $archive 'Windows')
        Copy-Item -LiteralPath $sourceIcon -Destination (Join-Path $fresh 'Application.ico')
        Copy-Item -LiteralPath $EconomyServerPackage -Destination (Join-Path $fresh 'EconomyServer') -Recurse
        $validation=@{Package=$fresh;StarServerPackage=$StarServerPackage;ProjectRoot=$projectPath;FeedRoot=$FeedRoot;PrivateKey=$PrivateKey;LauncherConfig=$LauncherConfig}
        $validated=& (Join-Path $PSScriptRoot 'Test-LauncherPackage.ps1') @validation
        & (Join-Path $PSScriptRoot 'Test-PackagedStartup.ps1') -Package $fresh -ReportDirectory (Join-Path $logRoot 'PackagedStartup') | Out-Host
        Write-Host "`n=== 4 Install fresh package at Build/Windows ==="
        [void](Assert-BuildChild $package);[void](Assert-BuildChild $previous)
        $hadPackage=Test-Path -LiteralPath $package
        if($hadPackage){[void][IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($previous));Move-Item -LiteralPath $package -Destination $previous}
        try {Move-Item -LiteralPath $fresh -Destination $package}
        catch {if($hadPackage -and !(Test-Path -LiteralPath $package)){Move-Item -LiteralPath $previous -Destination $package};throw}
        Write-Host "`n=== 5 Record verified build ==="
        $message=Join-Path $logRoot 'build-commit.txt'
        [IO.File]::WriteAllText($message,"Record verified Windows build $($validated.Version)`n`nSource commit: $sourceCommit`nEditor build, $($tests.succeeded) automated tests, client build, cook, package and isolated packaged-startup check passed. Includes packaged StarServer and the cross-process verified native economy service. Only build metadata is recorded automatically; unrelated staged and unstaged changes are preserved.`n")
        & git -C $projectPath diff --quiet HEAD -- Content/Build/build-version.json
        if($LASTEXITCODE -eq 1){& git -C $projectPath commit --only -F $message -- Content/Build/build-version.json;if($LASTEXITCODE -ne 0){throw 'The build passed but its metadata commit failed. Publication was stopped.'}}
        elseif($LASTEXITCODE -ne 0){throw 'Could not check the build metadata for its commit.'}
        Write-Host "`n=== 6 ZIP and deploy to launcher ==="
        $publish=@{Package=$package;StarServerPackage=$StarServerPackage;Output=(Join-Path $buildRoot 'Windows.zip');FeedRoot=$FeedRoot;ProjectRoot=$projectPath;PrivateKey=$PrivateKey;LauncherConfig=$LauncherConfig;About=$About}
        & (Join-Path $PSScriptRoot 'Publish-LauncherBuild.ps1') @publish *>&1 | Tee-Object -FilePath (Join-Path $logRoot 'Publish.log') | Out-Host
        $summary=@{version=$validated.Version;sourceCommit=$sourceCommit;buildCommit=(& git -C $projectPath rev-parse HEAD).Trim();testsPassed=$tests.succeeded;package=$package;previousPackage=$(if($hadPackage){$previous}else{$null});feed=[IO.Path]::GetFullPath($FeedRoot);completedUtc=[DateTime]::UtcNow.ToString('o')}
        [IO.File]::WriteAllText((Join-Path $logRoot 'build-summary.json'),($summary|ConvertTo-Json -Depth 5))
        if(!$KeepStaging -and (Test-Path -LiteralPath $runRoot)) {
            $cleanup=Assert-BuildChild $runRoot
            if([IO.File]::ReadAllText((Join-Path $cleanup '.pipeline-run')) -cne $runId){throw 'Staging ownership marker changed; automatic cleanup stopped.'}
            Remove-Item -LiteralPath $cleanup -Recurse -Force
        }
        Write-Host "`nCOMPLETE: $($validated.Version) is available in the launcher. Choose Check for Updates."
        Write-Host "Logs and build record: $logRoot"
    } finally {Pop-Location}
} catch {Write-Host "`nSTOPPED: $($_.Exception.Message)";Write-Host "Logs: $logRoot";throw}
finally {$pipelineLock.Dispose()}
