#Requires -Version 7.0
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Package,
    [Parameter(Mandatory)][string]$StarServerPackage,
    [string]$ProjectRoot = (Join-Path $PSScriptRoot '..'),
    [string]$FeedRoot = '',
    [string]$PrivateKey = '',
    [string]$LauncherConfig = ''
)
$ErrorActionPreference='Stop'
$projectPath=(Resolve-Path -LiteralPath $ProjectRoot).Path
$clientPath=(Resolve-Path -LiteralPath $Package).Path
$serverPath=(Resolve-Path -LiteralPath $StarServerPackage).Path
$buildRelative='StarshatterWars/Content/Build/build-version.json'
foreach($file in @('StarshatterWars.exe','StarshatterWars/Binaries/Win64/StarshatterWars.exe',$buildRelative)) {
    if(!(Test-Path -LiteralPath (Join-Path $clientPath $file) -PathType Leaf)){throw "Incomplete Windows package: $file. Finish cooking and packaging first."}
}
$record=Get-Content -LiteralPath (Join-Path $clientPath $buildRelative) -Raw | ConvertFrom-Json
$sourceRecord=Get-Content -LiteralPath (Join-Path $projectPath 'Content/Build/build-version.json') -Raw | ConvertFrom-Json
$version=[string]$record.version
if($version -notmatch '^\d{8}-\d{6}Z$'){throw 'The Windows package has no valid UTC build identifier.'}
if($version -cne [string]$sourceRecord.version) {
    throw "The package is not the latest build: Build/Windows has $version; the project has $($sourceRecord.version). Cook and package to Build/Windows before deploying. No release was changed."
}
$nativeGame=Join-Path $clientPath 'StarshatterWars/Binaries/Win64/StarshatterWars.exe'
$clientHash=(Get-FileHash -LiteralPath $nativeGame -Algorithm SHA256).Hash
$matches=@('StarshatterWars.exe','StarshatterWars-Win64-Shipping.exe','StarshatterWars-Win64-Test.exe') | Where-Object {
    $candidate=Join-Path $projectPath "Binaries/Win64/$_"
    (Test-Path -LiteralPath $candidate -PathType Leaf) -and (Get-FileHash -LiteralPath $candidate -Algorithm SHA256).Hash -eq $clientHash
}
if(!$matches){throw 'The packaged game executable does not match a compiled client in this project. Rebuild, cook and package again; no release was changed.'}
$paks=Join-Path $clientPath 'StarshatterWars/Content/Paks'
if(!(Test-Path -LiteralPath $paks -PathType Container) -or !(Get-ChildItem -LiteralPath $paks -Filter '*.pak' -File)) {
    throw 'Cooked game content is missing from Build/Windows. Finish packaging first.'
}
foreach($container in Get-ChildItem -LiteralPath $paks -Filter '*.ucas' -File) {
    if(!(Test-Path -LiteralPath ([IO.Path]::ChangeExtension($container.FullName,'.utoc')) -PathType Leaf)) {throw "Incomplete cooked content: $($container.Name) has no matching .utoc file."}
}
foreach($container in Get-ChildItem -LiteralPath $paks -Filter '*.utoc' -File) {
    if(!(Test-Path -LiteralPath ([IO.Path]::ChangeExtension($container.FullName,'.ucas')) -PathType Leaf)) {throw "Incomplete cooked content: $($container.Name) has no matching .ucas file."}
}
if(!(Test-Path -LiteralPath (Join-Path $serverPath 'StarshatterWars/Binaries/Win64/StarServer.exe') -PathType Leaf)) {throw 'The packaged StarServer is missing. Select the existing Build/StarServer/WindowsServer package.'}
$feedPath=if($FeedRoot){[IO.Path]::GetFullPath($FeedRoot)}else{''}
if($feedPath) {
    foreach($root in @($clientPath,$serverPath)) {
        if($feedPath.Equals($root,[StringComparison]::OrdinalIgnoreCase) -or $feedPath.StartsWith($root.TrimEnd('\','/')+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) {throw 'The launcher feed must be outside the packaged client and server directories.'}
    }
    $liveManifest=Join-Path $feedPath 'current/manifest.json'
    if(Test-Path -LiteralPath $liveManifest -PathType Leaf) {
        $live=Get-Content -LiteralPath $liveManifest -Raw | ConvertFrom-Json
        if([string]::CompareOrdinal([string]$live.version,$version) -gt 0) {throw "The launcher already has newer build $($live.version). Refusing to replace it with $version."}
    }
}
if($PrivateKey -or $LauncherConfig) {
    if(!(Test-Path -LiteralPath $PrivateKey -PathType Leaf)){throw 'The local release signing key is missing. It must not be included in the package.'}
    $config=Get-Content -LiteralPath $LauncherConfig -Raw | ConvertFrom-Json
    if($feedPath -and $config.localFeedServer.enabled -and $config.localFeedServer.documentRoot) {
        $configured=[IO.Path]::GetFullPath([string]$config.localFeedServer.documentRoot).TrimEnd('\','/')
        if(!$configured.Equals($feedPath.TrimEnd('\','/'),[StringComparison]::OrdinalIgnoreCase)) {throw "Wrong feed directory. This launcher serves $configured, but the deployer was pointed at $feedPath."}
    }
    $private=[Security.Cryptography.RSA]::Create();$public=[Security.Cryptography.RSA]::Create()
    try {
        $private.ImportFromPem([IO.File]::ReadAllText((Resolve-Path -LiteralPath $PrivateKey).Path))
        $public.ImportFromPem([string]$config.patchPublicKeyPem)
        if($private.KeySize -lt 2048 -or [Convert]::ToBase64String($private.ExportSubjectPublicKeyInfo()) -cne [Convert]::ToBase64String($public.ExportSubjectPublicKeyInfo())) {throw 'The release signing key does not match this launcher. No release was changed.'}
    } finally {$private.Dispose();$public.Dispose()}
}
[pscustomobject]@{Version=$version;Channel=[string]$record.channel;ClientRoot=$clientPath;ServerRoot=$serverPath;ProjectRoot=$projectPath;FeedRoot=$feedPath;ClientSha256=$clientHash}
