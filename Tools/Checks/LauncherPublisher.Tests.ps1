#Requires -Version 7.0
[CmdletBinding()]
param([string]$ProjectRoot=(Join-Path $PSScriptRoot '../..'))
$ErrorActionPreference='Stop'
$projectPath=(Resolve-Path -LiteralPath $ProjectRoot).Path
$fixture=Join-Path $projectPath ('Saved/Automation/LauncherPublisher-'+[guid]::NewGuid().ToString('N'))
$source=Join-Path $fixture 'Project';$package=Join-Path $fixture 'Package';$server=Join-Path $fixture 'Server';$feed=Join-Path $fixture 'Feed'
function Write-FixtureFile([string]$Path,[string]$Text) {
    [void][IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($Path))
    [IO.File]::WriteAllText($Path,$Text)
}
function Assert-True([bool]$Value,[string]$Message) {if(!$Value){throw "FAILED: $Message"};Write-Host "PASS: $Message"}
function Assert-Rejected([scriptblock]$Action,[string]$Expected) {
    $rejected=$false
    try {& $Action | Out-Null} catch {
        if($_.Exception.Message -notlike "*$Expected*"){throw}
        $rejected=$true
    }
    Assert-True $rejected "Rejects $Expected"
}
$version='20261122-230000Z';$old='20261121-230553Z'
$record=@{version=$version;channel='Alpha 5';builtUtc='2026-11-22T23:00:00Z'} | ConvertTo-Json
$buildRelative='StarshatterWars/Content/Build/build-version.json'
Write-FixtureFile (Join-Path $source 'Content/Build/build-version.json') $record
Write-FixtureFile (Join-Path $source 'Binaries/Win64/StarshatterWars.exe') 'fixture compiled client'
Write-FixtureFile (Join-Path $package 'StarshatterWars.exe') 'fixture bootstrap'
Write-FixtureFile (Join-Path $package 'StarshatterWars/Binaries/Win64/StarshatterWars.exe') 'fixture compiled client'
Write-FixtureFile (Join-Path $package $buildRelative) $record
Write-FixtureFile (Join-Path $package 'StarshatterWars/Content/Paks/StarshatterWars-Windows.pak') 'fixture pak'
Write-FixtureFile (Join-Path $package 'StarshatterWars/Content/Paks/StarshatterWars-Windows.ucas') 'fixture ucas'
Write-FixtureFile (Join-Path $package 'StarshatterWars/Content/Paks/StarshatterWars-Windows.utoc') 'fixture utoc'
Write-FixtureFile (Join-Path $package 'StarshatterWars/Saved/SaveGames/private.sav') 'must not ship'
Write-FixtureFile (Join-Path $server 'StarshatterWars/Binaries/Win64/StarServer.exe') 'fixture server'
Write-FixtureFile (Join-Path $server 'StarshatterWars/Saved/private.sav') 'must not ship'
Write-FixtureFile (Join-Path $feed 'current/manifest.json') (@{version=$old}|ConvertTo-Json)
Write-FixtureFile (Join-Path $feed 'current/previous-release.txt') 'keep this on failure'
$keyPath=Join-Path $fixture 'test-release-key.pem';$configPath=Join-Path $fixture 'launcher.test.json'
$key=[Security.Cryptography.RSA]::Create(2048)
try {
    Write-FixtureFile $keyPath $key.ExportPkcs8PrivateKeyPem()
    Write-FixtureFile $configPath (@{patchPublicKeyPem=$key.ExportSubjectPublicKeyInfoPem();localFeedServer=@{enabled=$true;documentRoot=$feed}}|ConvertTo-Json -Depth 5)
} finally {$key.Dispose()}
$publish=Join-Path $projectPath 'Launcher/Publish-LauncherBuild.ps1'
$zipPath=Join-Path $fixture 'Windows.zip'
$params=@{Package=$package;StarServerPackage=$server;ProjectRoot=$source;FeedRoot=$feed;PrivateKey=$keyPath;LauncherConfig=$configPath;Output=$zipPath;About='Fixture deployment: finite warehouse stock and pilot UI fixes.'}
& $publish @params -ValidateOnly
Assert-True (!(Test-Path -LiteralPath $zipPath)) 'Validation alone does not create a ZIP'
$oldManifestHash=(Get-FileHash -LiteralPath (Join-Path $feed 'current/manifest.json')).Hash
Write-FixtureFile (Join-Path $package $buildRelative) (@{version=$old;channel='Alpha 5'}|ConvertTo-Json)
Assert-Rejected {& $publish @params -ValidateOnly} 'package is not the latest build'
Write-FixtureFile (Join-Path $package $buildRelative) $record
Write-FixtureFile (Join-Path $package 'StarshatterWars/Binaries/Win64/StarshatterWars.exe') 'stale executable'
Assert-Rejected {& $publish @params -ValidateOnly} 'does not match a compiled client'
Write-FixtureFile (Join-Path $package 'StarshatterWars/Binaries/Win64/StarshatterWars.exe') 'fixture compiled client'
Remove-Item -LiteralPath (Join-Path $package 'StarshatterWars/Content/Paks/StarshatterWars-Windows.utoc')
Assert-Rejected {& $publish @params -ValidateOnly} 'no matching .utoc'
Write-FixtureFile (Join-Path $package 'StarshatterWars/Content/Paks/StarshatterWars-Windows.utoc') 'fixture utoc'
$wrongFeed=$params.Clone();$wrongFeed.FeedRoot=Join-Path $fixture 'WrongFeed'
Assert-Rejected {& $publish @wrongFeed -ValidateOnly} 'Wrong feed directory'
$wrongKey=[Security.Cryptography.RSA]::Create(2048)
try {
    $wrongPath=Join-Path $fixture 'wrong-test-key.pem';Write-FixtureFile $wrongPath $wrongKey.ExportPkcs8PrivateKeyPem()
    $wrongParams=$params.Clone();$wrongParams.PrivateKey=$wrongPath
    Assert-Rejected {& $publish @wrongParams -ValidateOnly} 'signing key does not match'
} finally {$wrongKey.Dispose()}
Write-FixtureFile (Join-Path $feed 'current/manifest.json') (@{version='20261123-230000Z'}|ConvertTo-Json)
Assert-Rejected {& $publish @params -ValidateOnly} 'already has newer build'
Write-FixtureFile (Join-Path $feed 'current/manifest.json') (@{version=$old}|ConvertTo-Json)
Assert-True ((Get-FileHash -LiteralPath (Join-Path $feed 'current/manifest.json')).Hash -eq $oldManifestHash) 'Rejected preflights preserve the live manifest'
Assert-True (!(Test-Path -LiteralPath $zipPath)) 'Rejected preflights never create a stale ZIP'
& $publish @params
$manifest=Get-Content -LiteralPath (Join-Path $feed 'current/manifest.json') -Raw | ConvertFrom-Json
Assert-True ($manifest.version -ceq $version) 'The selected package version reaches the live feed'
$archive=[IO.Compression.ZipFile]::OpenRead((Join-Path $feed 'current/StarshatterWars.zip'))
try {
    Assert-True ($null -ne $archive.GetEntry('StarshatterWars/Binaries/Win64/StarshatterWars.exe')) 'ZIP contains the current client'
    Assert-True ($null -ne $archive.GetEntry('StarServer/WindowsServer/StarshatterWars/Binaries/Win64/StarServer.exe')) 'ZIP includes StarServer under the SSW installation'
    Assert-True ($null -eq $archive.GetEntry('StarshatterWars/Saved/SaveGames/private.sav')) 'ZIP excludes pilot saves'
    Assert-True ($null -eq $archive.GetEntry('StarServer/WindowsServer/StarshatterWars/Saved/private.sav')) 'ZIP excludes server saves'
    Assert-True ($archive.Entries.Count -eq $manifest.files.Count) 'ZIP and signed manifest list the same files'
} finally {$archive.Dispose()}
Assert-True ($null -ne (Get-ChildItem -LiteralPath $feed -Directory -Filter 'previous-*')) 'Previous release remains available for recovery'
Assert-True ((Get-Content -LiteralPath ($zipPath+'.release.json') -Raw | ConvertFrom-Json).version -ceq $version) 'Deployment receipt records the published version'
$lock=[IO.File]::Open((Join-Path $feed '.publish.lock'),[IO.FileMode]::OpenOrCreate,[IO.FileAccess]::ReadWrite,[IO.FileShare]::None)
try {Assert-Rejected {& $publish @params} 'Another deployer is publishing'} finally {$lock.Dispose()}
$tests=@{passed=$true;fixture=$fixture;version=$version;cases=18}
[IO.File]::WriteAllText((Join-Path $fixture 'results.json'),($tests|ConvertTo-Json))
Write-Host "DEPLOYER TESTS PASSED: $fixture"
