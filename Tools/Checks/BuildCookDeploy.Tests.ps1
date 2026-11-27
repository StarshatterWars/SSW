#Requires -Version 7.0
$ErrorActionPreference='Stop'
$root=(Resolve-Path "$PSScriptRoot/../..").Path
$fixture=Join-Path $root ('Saved/Automation/BuildCookDeploy-'+[guid]::NewGuid().ToString('N'))
$source=Join-Path $fixture 'Project With Spaces';$engine=Join-Path $fixture 'Engine With Spaces'
$prepared=Join-Path $fixture 'Prepared';$server=Join-Path $fixture 'Server';$feed=Join-Path $fixture 'Feed'
function Put($p,$s){[void][IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($p));[IO.File]::WriteAllText($p,$s)}
$checks=0
function Check($ok,$s){if(!$ok){throw "FAILED: $s"};$script:checks++;Write-Host "PASS $s"}
$old='20261121-230553Z';$new='20261123-230000Z'
$record=@{version=$new;channel='Alpha 5';builtUtc='2026-11-23T23:00:00Z'}|ConvertTo-Json
Put "$source/StarshatterWars.uproject" '{}'
Put "$source/Content/Build/build-version.json" (@{version=$old;channel='Alpha 5'}|ConvertTo-Json)
Put "$source/Binaries/Win64/StarshatterWars.exe" 'compiled client'
Put "$source/notes.txt" 'baseline'
Put "$source/Build/Windows/previous.txt" 'old package'
Put "$prepared/StarshatterWars.exe" 'bootstrap'
Put "$prepared/StarshatterWars/Binaries/Win64/StarshatterWars.exe" 'compiled client'
Put "$prepared/StarshatterWars/Content/Build/build-version.json" $record
foreach($ext in @('pak','ucas','utoc')){Put "$prepared/StarshatterWars/Content/Paks/game.$ext" $ext}
# The isolated startup gate must execute a real PE, not the old text placeholder.
Put "$fixture/Startup.cs" 'using System;using System.IO;class Startup{static int Main(string[] args){foreach(string a in args)if(a.StartsWith("-abslog="))File.WriteAllText(a.Substring(8),"Fixture startup completed");return 0;}}'
$probeOutput=[IO.Path]::GetFullPath("$prepared/StarshatterWars/Binaries/Win64/StarshatterWars.exe");$probeSource=[IO.Path]::GetFullPath("$fixture/Startup.cs")
Put "$fixture/Startup.rsp" ('/nologo /target:exe /out:"'+$probeOutput+'" "'+$probeSource+'"')
& "$env:WINDIR/Microsoft.NET/Framework64/v4.0.30319/csc.exe" ('@'+[IO.Path]::GetFullPath("$fixture/Startup.rsp"))
if($LASTEXITCODE -ne 0){throw 'Could not build isolated startup probe.'}
Copy-Item -LiteralPath "$prepared/StarshatterWars/Binaries/Win64/StarshatterWars.exe" -Destination "$source/Binaries/Win64/StarshatterWars.exe" -Force
Put "$server/StarshatterWars/Binaries/Win64/StarServer.exe" 'server'
Put "$feed/current/manifest.json" (@{version=$old}|ConvertTo-Json)
$key=[Security.Cryptography.RSA]::Create(2048)
try{Put "$fixture/key.pem" $key.ExportPkcs8PrivateKeyPem();Put "$fixture/config.json" (@{patchPublicKeyPem=$key.ExportSubjectPublicKeyInfoPem();localFeedServer=@{enabled=$true;documentRoot=$feed}}|ConvertTo-Json -Depth 5)}finally{$key.Dispose()}
& git -C $source init -q
& git -C $source config user.name 'Pipeline Fixture'
& git -C $source config user.email 'fixture@example.invalid'
& git -C $source add StarshatterWars.uproject Content/Build/build-version.json notes.txt
& git -C $source commit -qm baseline
$runner="$fixture/fake-stage.ps1"
Put $runner @'
$ErrorActionPreference='Stop'
$mode=$args[0]
$toolArgs=@([regex]::Matches($env:SSW_FIXTURE_ARGS,'"([^"]*)"|(\S+)')|ForEach-Object {if($_.Groups[1].Success){$_.Groups[1].Value}else{$_.Groups[2].Value}})
Add-Content -LiteralPath $env:SSW_FIXTURE_TRACE -Value $mode
if($env:SSW_FIXTURE_FAIL -eq $mode -and $mode -ne 'tests'){exit 7}
if($mode -eq 'tests'){
 $report=($toolArgs|Where-Object {$_ -like '-ReportExportPath=*'}) -replace '^-ReportExportPath=',''
 [void][IO.Directory]::CreateDirectory($report)
 @{succeeded=370;succeededWithWarnings=$(if($env:SSW_FIXTURE_FAIL -eq 'warnings'){1}else{0});failed=$(if($env:SSW_FIXTURE_FAIL -eq 'tests'){1}else{0});notRun=0}|ConvertTo-Json|Set-Content "$report/index.json"
}
if($mode -eq 'cook'){
 $archive=($toolArgs|Where-Object {$_ -like '-archivedirectory=*'}) -replace '^-archivedirectory=',''
 [void][IO.Directory]::CreateDirectory($archive)
 Copy-Item -LiteralPath $env:SSW_FIXTURE_PREPARED -Destination "$archive/Windows" -Recurse
 Copy-Item "$archive/Windows/StarshatterWars/Content/Build/build-version.json" "$env:SSW_FIXTURE_SOURCE/Content/Build/build-version.json" -Force
 if($env:SSW_FIXTURE_FAIL -eq 'stale'){Set-Content "$archive/Windows/StarshatterWars/Content/Build/build-version.json" '{"version":"20261121-230553Z","channel":"Alpha 5"}'}
}
exit 0
'@
foreach($step in @(@('Engine/Build/BatchFiles/Build.bat','build'),@('Engine/Build/BatchFiles/RunUAT.bat','cook'),@('Editor.cmd','tests'))){
 Put (Join-Path $engine $step[0]) ("@echo off`r`nset SSW_FIXTURE_ARGS=%*`r`npwsh.exe -NoLogo -NoProfile -File `"$runner`" $($step[1])`r`nexit /b %ERRORLEVEL%`r`n")
}
$economy=Join-Path $fixture 'Economy';Put "$economy/FractalEconomy.exe" 'fixture native service'
$nativeFile=Get-Item "$economy/FractalEconomy.exe"
Put "$economy/server-manifest.json" (@{server='FractalEconomy';protocol=1;liveClientCutoverReady=$true;standbyAutostartReady=$true;files=@(@{path='FractalEconomy.exe';length=$nativeFile.Length;sha256=(Get-FileHash $nativeFile.FullName -Algorithm SHA256).Hash})}|ConvertTo-Json -Depth 5)
$params=@{EconomyServerPackage=$economy;ProjectRoot=$source;EngineRoot=$engine;EditorCommand="$engine/Editor.cmd";StarServerPackage=$server;FeedRoot=$feed;PrivateKey="$fixture/key.pem";LauncherConfig="$fixture/config.json";About='Pipeline fixture'}
$pipeline="$root/Launcher/Build-Cook-Deploy.ps1"
$env:SSW_FIXTURE_TRACE="$fixture/trace.txt";$env:SSW_FIXTURE_PREPARED=$prepared;$env:SSW_FIXTURE_SOURCE=$source
try{
 & $pipeline @params -PlanOnly
 Check (!(Test-Path $env:SSW_FIXTURE_TRACE)) 'Plan does not run tools'
 foreach($failure in @('build','tests','warnings','cook','stale')){
  $env:SSW_FIXTURE_FAIL=$failure
  if(Test-Path $env:SSW_FIXTURE_TRACE){Remove-Item -LiteralPath $env:SSW_FIXTURE_TRACE}
  $rejected=$false;try{& $pipeline @params}catch{$rejected=$true}
  Check $rejected "$failure failure stops pipeline"
  $expected=if($failure -in @('cook','stale')){'build,tests,cook'}elseif($failure -eq 'build'){'build'}else{'build,tests'}
  Check (((Get-Content $env:SSW_FIXTURE_TRACE)-join ',') -eq $expected) "$failure stops at the expected step"
  Check ((Get-Content "$feed/current/manifest.json" -Raw|ConvertFrom-Json).version -eq $old) "$failure failure preserves feed"
  Check (Test-Path "$source/Build/Windows/previous.txt") "$failure failure preserves previous package"
 }
 $env:SSW_FIXTURE_FAIL=''
 Remove-Item -LiteralPath $env:SSW_FIXTURE_TRACE
 Put "$source/notes.txt" 'owner staged work'
 & git -C $source add notes.txt
 & $pipeline @params
 Check (((Get-Content $env:SSW_FIXTURE_TRACE)-join ',') -eq 'build,tests,cook') 'Steps execute in order with spaced paths'
 Check ((Get-Content "$feed/current/manifest.json" -Raw|ConvertFrom-Json).version -eq $new) 'Latest version published'
 Check (Test-Path "$source/Build/Windows.zip") 'ZIP created'
 Check (Test-Path "$source/Build/Windows/EconomyServer/server-manifest.json") 'Verified native economy accompanies the client'
 $zip=[IO.Compression.ZipFile]::OpenRead("$source/Build/Windows.zip")
 try{Check (@($zip.Entries|Where-Object {$_.FullName -match '(^|/)EconomyServer/FractalEconomy.exe$'}).Count -eq 1) 'Launcher ZIP contains exactly one native economy executable'}finally{$zip.Dispose()}

 Check ((& git -C $source diff --cached --name-only) -eq 'notes.txt') 'Owner staged work preserved'
 Check ((& git -C $source show --pretty= --name-only HEAD).Trim() -eq 'Content/Build/build-version.json') 'Build commit records only metadata'
 Check (@(Get-ChildItem "$source/Build/PreviousWindows" -Recurse -Filter previous.txt).Count -eq 1) 'Previous package retained'
 Check (@(Get-ChildItem "$source/Build/ReleaseStaging" -Directory|Where-Object {Test-Path (Join-Path $_.FullName 'Archive/Windows')}).Count -eq 1) 'Successful staging removed; rejected stale staging retained for diagnosis'
 Check (@(Get-ChildItem "$source/Saved/Logs" -Recurse -Filter build-summary.json).Count -eq 1) 'Build receipt retained'
 $env:SSW_NO_PAUSE='1'
 & "$root/Launcher/Build-Cook-Deploy.cmd" -PlanOnly -ProjectRoot $source -EngineRoot $engine -EditorCommand "$engine/Editor.cmd" -StarServerPackage $server -FeedRoot $feed -PrivateKey "$fixture/key.pem" -LauncherConfig "$fixture/config.json"
 Check ($LASTEXITCODE -eq 0) 'CMD wrapper forwards spaced paths and success exit code'
 @{passed=$true;checks=$checks;fixture=$fixture}|ConvertTo-Json|Set-Content "$fixture/results.json"
 Write-Host "ALL $checks PIPELINE CHECKS PASSED: $fixture"
}finally{foreach($name in @('SSW_NO_PAUSE','SSW_FIXTURE_TRACE','SSW_FIXTURE_PREPARED','SSW_FIXTURE_SOURCE','SSW_FIXTURE_FAIL')){[Environment]::SetEnvironmentVariable($name,$null,'Process')}}
