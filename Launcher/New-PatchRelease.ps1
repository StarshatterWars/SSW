param(
    [Parameter(Mandatory)][string]$Package,
    [Parameter(Mandatory)][string]$Output,
    [string]$Version = [DateTime]::UtcNow.ToString("yyyyMMdd-HHmmss'Z'", [Globalization.CultureInfo]::InvariantCulture),
    [Parameter(Mandatory)][ValidateLength(1,500)][string]$About,
    [Parameter(Mandatory)][string]$PrivateKey,
    [string]$StarServerPackage = (Join-Path $PSScriptRoot '../Build/StarServer/WindowsServer')
)
$ErrorActionPreference = 'Stop'
if ([string]::IsNullOrWhiteSpace($About)) { throw 'Describe the update in plain English.' }
$source = (Resolve-Path -LiteralPath $Package).Path
$server = (Resolve-Path -LiteralPath $StarServerPackage).Path
$target = [IO.Path]::GetFullPath($Output)
if ($target.StartsWith($source.TrimEnd('\') + '\', [StringComparison]::OrdinalIgnoreCase) -or
    $target.StartsWith($server.TrimEnd('\') + '\', [StringComparison]::OrdinalIgnoreCase) -or
    (Test-Path -LiteralPath $target)) { throw 'Choose a new release directory outside the package.' }
if (!(Test-Path -LiteralPath (Join-Path $server 'StarshatterWars/Binaries/Win64/StarServer.exe') -PathType Leaf)) {
    throw 'StarServer package executable missing.'
}

function Copy-ReleaseFiles([string]$Root, [string]$Prefix) {
    Get-ChildItem -LiteralPath $Root -File -Force -Recurse | ForEach-Object {
        $relative = [IO.Path]::GetRelativePath($Root, $_.FullName).Replace('\','/')
        if ($relative -like 'StarshatterWars/Saved/*' -or $relative -like 'StarshatterWars/Config/Custom/StarServer/*' -or
            $relative -like 'StarServer/*' -or $relative -eq '.ssw-version' -or $_.Extension -eq '.pdb') { return }
        $path = $Prefix + $relative
        $destination = Join-Path "$target/files" $path
        [void][IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($destination))
        Copy-Item -LiteralPath $_.FullName -Destination $destination
        @{path=$path; size=$_.Length; sha256=(Get-FileHash -LiteralPath $destination).Hash}
    }
}

$noticePath = 'StarshatterWars/Saved/Mods/SSW-Mods-Notice.txt'
$key = [Security.Cryptography.RSA]::Create()
try {
    $key.ImportFromPem([IO.File]::ReadAllText((Resolve-Path -LiteralPath $PrivateKey).Path))
    if ($key.KeySize -lt 2048) { throw 'Use an RSA key of at least 2048 bits.' }
    $files = @(
        Copy-ReleaseFiles $source ''
        Copy-ReleaseFiles $server 'StarServer/WindowsServer/'
        $notice = Join-Path $target "files/$noticePath"
        [void][IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($notice))
        Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'SSW-Mods-Notice.txt') -Destination $notice
        @{path=$noticePath; size=(Get-Item -LiteralPath $notice).Length; sha256=(Get-FileHash -LiteralPath $notice).Hash}
    )
    if ($files.path -notcontains 'StarshatterWars.exe' -or
        $files.path -notcontains 'StarshatterWars/Binaries/Win64/StarshatterWars.exe') { throw 'Package executables missing.' }
    $manifest = [Text.Encoding]::UTF8.GetBytes((@{version=$Version; about=$About.Trim(); files=$files} | ConvertTo-Json -Depth 4))
    [IO.File]::WriteAllBytes("$target/manifest.json", $manifest)
    [IO.File]::WriteAllBytes("$target/manifest.json.sig", $key.SignData($manifest,
        [Security.Cryptography.HashAlgorithmName]::SHA256, [Security.Cryptography.RSASignaturePadding]::Pkcs1))
    Write-Output $key.ExportSubjectPublicKeyInfoPem()
} finally { $key.Dispose() }
