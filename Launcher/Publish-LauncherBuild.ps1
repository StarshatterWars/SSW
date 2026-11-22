#Requires -Version 7.0
[CmdletBinding()]
param(
    [string]$Package = (Join-Path $PSScriptRoot '../Build/Windows'),
    [string]$StarServerPackage = (Join-Path $PSScriptRoot '../Build/StarServer/WindowsServer'),
    [string]$Output = (Join-Path $PSScriptRoot '../Build/Windows.zip'),
    [string]$FeedRoot = (Join-Path $PSScriptRoot '../Build/LauncherFeed'),
    [string]$PrivateKey = (Join-Path $PSScriptRoot '../Saved/SSWLocalReleaseKey.pem'),
    [ValidateLength(0,500)][string]$About = '',
    [string]$ProjectRoot = (Join-Path $PSScriptRoot '..'),
    [string]$LauncherConfig = (Join-Path $PSScriptRoot 'launcher.localhost.json'),
    [switch]$ValidateOnly
)
$ErrorActionPreference='Stop'
$validated=& (Join-Path $PSScriptRoot 'Test-LauncherPackage.ps1') -Package $Package -StarServerPackage $StarServerPackage -ProjectRoot $ProjectRoot -FeedRoot $FeedRoot -PrivateKey $PrivateKey -LauncherConfig $LauncherConfig
$packageRoot=$validated.ClientRoot
$StarServerPackage=$validated.ServerRoot
$version=$validated.Version
$feedPath=$validated.FeedRoot
if([string]::IsNullOrWhiteSpace($About)) {
    $notes=Join-Path $PSScriptRoot 'next-release-notes.txt'
    $About=if(Test-Path -LiteralPath $notes){(Get-Content -LiteralPath $notes -Raw).Trim()}else{'Updated game build. Existing pilot saves are preserved. Includes StarServer.'}
}
if($About.Length -gt 500){throw 'Keep the plain-English release notes to 500 characters or fewer.'}
Write-Host "PACKAGE: $packageRoot"
Write-Host "BUILD:   $version"
Write-Host "FEED:    $feedPath"
if($ValidateOnly){Write-Host 'VALIDATED. The package matches the compiled client and launcher. No ZIP or release files were changed.';return}
[void][IO.Directory]::CreateDirectory($feedPath)
try {$publishLock=[IO.File]::Open((Join-Path $feedPath '.publish.lock'),[IO.FileMode]::OpenOrCreate,[IO.FileAccess]::ReadWrite,[IO.FileShare]::None)}
catch {throw 'Another deployer is publishing this feed. Wait for it to finish before trying again.'}
try {
$stage=Join-Path $feedPath ('staging-'+[Guid]::NewGuid().ToString('N'))
$current=Join-Path $feedPath 'current'
$previous=Join-Path $feedPath ('previous-'+[Guid]::NewGuid().ToString('N'))

Write-Host "Creating and publishing packaged build $version"
& (Join-Path $PSScriptRoot 'Build-LauncherZip.ps1') -Package $packageRoot -StarServerPackage $StarServerPackage -Output $Output -ProjectRoot $ProjectRoot
& (Join-Path $PSScriptRoot 'New-PatchRelease.ps1') -Package $packageRoot -StarServerPackage $StarServerPackage -Output $stage -PrivateKey $PrivateKey -About $About -Version $version | Out-Null
$manifest=Get-Content -LiteralPath (Join-Path $stage 'manifest.json') -Raw | ConvertFrom-Json
if ($manifest.version -ne $version) { throw 'Package changed during publishing; existing feed kept.' }
$configData=Get-Content -LiteralPath $LauncherConfig -Raw | ConvertFrom-Json
$publicKey=[Security.Cryptography.RSA]::Create()
try {
    $publicKey.ImportFromPem($configData.patchPublicKeyPem)
    if (!$publicKey.VerifyData([IO.File]::ReadAllBytes((Join-Path $stage 'manifest.json')),[IO.File]::ReadAllBytes((Join-Path $stage 'manifest.json.sig')),[Security.Cryptography.HashAlgorithmName]::SHA256,[Security.Cryptography.RSASignaturePadding]::Pkcs1)) {
        throw 'Signing key does not match the localhost launcher. Existing feed kept.'
    }
} finally { $publicKey.Dispose() }
$zip=[IO.Compression.ZipFile]::OpenRead([IO.Path]::GetFullPath($Output))
try {
    if ($zip.Entries.Count -ne $manifest.files.Count) { throw 'ZIP and patch manifest file counts differ; existing feed kept.' }
    foreach ($file in $manifest.files) {
        $entry=$zip.GetEntry($file.path)
        if (!$entry -or $entry.Length -ne $file.size) { throw "ZIP/manifest mismatch: $($file.path)" }
        $stream=$entry.Open()
        try { $digest=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($stream)) } finally { $stream.Dispose() }
        if ($digest -ne $file.sha256) { throw "ZIP/manifest hash mismatch: $($file.path)" }
    }
} finally { $zip.Dispose() }
Copy-Item -LiteralPath ([IO.Path]::GetFullPath($Output)) -Destination (Join-Path $stage 'StarshatterWars.zip')
$news=Get-Content -LiteralPath (Join-Path $PSScriptRoot 'feed.json') -Raw | ConvertFrom-Json
$entry=[pscustomobject]@{category='Latest install build';title="Alpha 5 - Build $version ready to install";date=[DateTime]::UtcNow.ToString('yyyy-MM-dd');body=$About}
$news.entries=@($entry)+@($news.entries | Where-Object {$_.category -ne 'Latest install build'})
[IO.File]::WriteAllText((Join-Path $stage 'feed.json'),($news | ConvertTo-Json -Depth 12))

# Rename only these explicitly named children of the resolved local feed root.
# Keep the previous complete release so interrupted preparation cannot replace it.
$rechecked=& (Join-Path $PSScriptRoot 'Test-LauncherPackage.ps1') -Package $packageRoot -StarServerPackage $StarServerPackage -ProjectRoot $ProjectRoot -FeedRoot $FeedRoot -PrivateKey $PrivateKey -LauncherConfig $LauncherConfig
if($rechecked.Version -ne $version){throw 'The package changed during publishing. The previous feed was kept.'}
$hadCurrent=Test-Path -LiteralPath $current
if ($hadCurrent) { Rename-Item -LiteralPath $current -NewName ([IO.Path]::GetFileName($previous)) }
try { Rename-Item -LiteralPath $stage -NewName 'current' }
catch {
    if ($hadCurrent -and !(Test-Path -LiteralPath $current)) { Rename-Item -LiteralPath $previous -NewName 'current' }
    throw
}
$published=Get-Content -LiteralPath (Join-Path $current 'manifest.json') -Raw | ConvertFrom-Json
if ($published.version -ne $version) { throw 'Published version verification failed.' }
Write-Host "PUBLISHED: $version"
Write-Host "Launcher ZIP, patch files, signed manifest and news: $current"
Write-Host 'In the launcher, choose Check for Updates. No installed game or pilot saves were modified.'

$receipt=[ordered]@{version=$version;package=$packageRoot;server=$StarServerPackage;feed=$current;zip=[IO.Path]::GetFullPath($Output);clientSha256=$validated.ClientSha256;publishedUtc=[DateTime]::UtcNow.ToString('o');about=$About}
[IO.File]::WriteAllText(([IO.Path]::GetFullPath($Output)+'.release.json'),($receipt|ConvertTo-Json -Depth 5))
Write-Host "Deployment receipt: $([IO.Path]::GetFullPath($Output)).release.json"
} finally {$publishLock.Dispose()}
