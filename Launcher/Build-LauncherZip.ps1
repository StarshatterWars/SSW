#Requires -Version 7.0
[CmdletBinding()]
param(
    [string]$Package = (Join-Path $PSScriptRoot '../Build/Windows'),
    [string]$StarServerPackage = (Join-Path $PSScriptRoot '../Build/StarServer/WindowsServer'),
    [string]$Output = (Join-Path $PSScriptRoot '../Build/Windows.zip'),
    [string]$ProjectRoot = (Join-Path $PSScriptRoot '..')
)
$ErrorActionPreference = 'Stop'
$validated=& (Join-Path $PSScriptRoot 'Test-LauncherPackage.ps1') -Package $Package -StarServerPackage $StarServerPackage -ProjectRoot $ProjectRoot
$clientRoot = (Resolve-Path -LiteralPath $Package).Path
$serverRoot = (Resolve-Path -LiteralPath $StarServerPackage).Path
$zipPath = [IO.Path]::GetFullPath($Output)
foreach ($root in @($clientRoot,$serverRoot)) {
    if ($zipPath.StartsWith($root.TrimEnd('\','/')+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) {
        throw 'Place the ZIP outside the packaged client and server folders.'
    }
}
foreach ($required in @('StarshatterWars.exe','StarshatterWars/Binaries/Win64/StarshatterWars.exe','StarshatterWars/Content/Build/build-version.json')) {
    if (!(Test-Path -LiteralPath (Join-Path $clientRoot $required) -PathType Leaf)) { throw "Incomplete package: $required" }
}
if (!(Test-Path -LiteralPath (Join-Path $serverRoot 'StarshatterWars/Binaries/Win64/StarServer.exe') -PathType Leaf)) { throw 'Packaged StarServer is missing.' }
$version = (Get-Content -LiteralPath (Join-Path $clientRoot 'StarshatterWars/Content/Build/build-version.json') -Raw | ConvertFrom-Json).version
if ([string]::IsNullOrWhiteSpace($version)) { throw 'Packaged build version is empty.' }
$entries = [Collections.Generic.List[object]]::new()
function Add-PackageFiles([string]$Root,[string]$Prefix) {
    foreach ($file in Get-ChildItem -LiteralPath $Root -File -Recurse -Force) {
        $relative = [IO.Path]::GetRelativePath($Root,$file.FullName).Replace('\','/')
        if ($relative -like 'StarshatterWars/Saved/*' -or $relative -like 'StarshatterWars/Config/Custom/StarServer/*' -or $relative -like 'StarServer/*' -or $relative -eq '.ssw-version' -or $file.Extension -eq '.pdb') { continue }
        $entries.Add([pscustomobject]@{Source=$file.FullName;Name=($Prefix+$relative);Size=$file.Length;Modified=$file.LastWriteTimeUtc})
    }
}
Add-PackageFiles $clientRoot ''
Add-PackageFiles $serverRoot 'StarServer/WindowsServer/'
$notice=Get-Item -LiteralPath (Join-Path $PSScriptRoot 'SSW-Mods-Notice.txt')
$entries.Add([pscustomobject]@{Source=$notice.FullName;Name='StarshatterWars/Saved/Mods/SSW-Mods-Notice.txt';Size=$notice.Length;Modified=$notice.LastWriteTimeUtc})
[void][IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($zipPath))
$temporary=$zipPath+'.'+[Guid]::NewGuid().ToString('N')+'.incomplete'
$archive=[IO.Compression.ZipFile]::Open($temporary,[IO.Compression.ZipArchiveMode]::Create)
try {
    $index=0
    foreach ($entry in $entries) {
        Write-Progress -Activity "Packaging Starshatter Wars $version" -Status $entry.Name -PercentComplete (100*$index/$entries.Count)
        $current=Get-Item -LiteralPath $entry.Source
        if ($current.Length -ne $entry.Size -or $current.LastWriteTimeUtc -ne $entry.Modified) { throw 'Package changed. Finish cooking and packaging first.' }
        [void][IO.Compression.ZipFileExtensions]::CreateEntryFromFile($archive,$entry.Source,$entry.Name,[IO.Compression.CompressionLevel]::Fastest)
        $after=Get-Item -LiteralPath $entry.Source
        if ($after.Length -ne $entry.Size -or $after.LastWriteTimeUtc -ne $entry.Modified) { throw 'Package changed during compression; previous ZIP kept.' }
        $index++
    }
} finally { $archive.Dispose() }
$verify=[IO.Compression.ZipFile]::OpenRead($temporary)
try {
    if ($verify.Entries.Count -ne $entries.Count) { throw 'ZIP entry verification failed.' }
    foreach ($entry in $entries) { if ($verify.GetEntry($entry.Name).Length -ne $entry.Size) { throw "ZIP verification failed: $($entry.Name)" } }
} finally { $verify.Dispose() }
$hash=(Get-FileHash -LiteralPath $temporary -Algorithm SHA256).Hash.ToLowerInvariant()
[IO.File]::Move($temporary,$zipPath,$true)
[IO.File]::WriteAllText($zipPath+'.sha256',"$hash  $([IO.Path]::GetFileName($zipPath))`n")
Write-Progress -Activity 'Packaging Starshatter Wars' -Completed
Write-Host "Ready: $zipPath"
Write-Host "Build: $version; files: $($entries.Count); includes StarServer."
Write-Host 'ZIP and SHA-256 created. Launcher feed and signed manifest are unchanged.'
