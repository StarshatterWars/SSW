param(
    [Parameter(Mandatory)][string]$Package,
    [Parameter(Mandatory)][string]$Output,
    [Parameter(Mandatory)][string]$Version,
    [Parameter(Mandatory)][string]$PrivateKey
)
$ErrorActionPreference = 'Stop'
$source = (Resolve-Path -LiteralPath $Package).Path
$target = [IO.Path]::GetFullPath($Output)
if ($target.StartsWith($source.TrimEnd('\') + '\', [StringComparison]::OrdinalIgnoreCase) -or
    (Test-Path -LiteralPath $target)) { throw 'Choose a new release directory outside the package.' }
$key = [Security.Cryptography.RSA]::Create()
try {
    $key.ImportFromPem([IO.File]::ReadAllText((Resolve-Path -LiteralPath $PrivateKey).Path))
    if ($key.KeySize -lt 2048) { throw 'Use an RSA key of at least 2048 bits.' }
    $files = @(Get-ChildItem -LiteralPath $source -File -Force -Recurse | ForEach-Object {
        $relative = [IO.Path]::GetRelativePath($source, $_.FullName).Replace('\','/')
        if ($relative -notlike 'StarshatterWars/Saved/*' -and $relative -ne '.ssw-version' -and $_.Extension -ne '.pdb') {
            $destination = Join-Path "$target/files" $relative
            [void][IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($destination))
            Copy-Item -LiteralPath $_.FullName -Destination $destination
            @{path=$relative; size=$_.Length; sha256=(Get-FileHash -LiteralPath $destination).Hash}
        }
    })
    if ($files.path -notcontains 'StarshatterWars.exe' -or
        $files.path -notcontains 'StarshatterWars/Binaries/Win64/StarshatterWars.exe') { throw 'Package executables missing.' }
    $manifest = [Text.Encoding]::UTF8.GetBytes((@{version=$Version; files=$files} | ConvertTo-Json -Depth 4))
    [IO.File]::WriteAllBytes("$target/manifest.json", $manifest)
    [IO.File]::WriteAllBytes("$target/manifest.json.sig", $key.SignData($manifest,
        [Security.Cryptography.HashAlgorithmName]::SHA256, [Security.Cryptography.RSASignaturePadding]::Pkcs1))
    Write-Output $key.ExportSubjectPublicKeyInfoPem()
} finally { $key.Dispose() }
