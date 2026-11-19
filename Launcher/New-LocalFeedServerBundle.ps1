param(
    [string]$Archive = (Join-Path (Split-Path $PSScriptRoot -Parent) 'Saved/Research/Apache/2.4.71/httpd-2.4.71-261112-Win64-VS18.zip')
)
$ErrorActionPreference = 'Stop'
$expected = '44961A929DFF16B767B3ADBCEA55760A942A00239B2891DC55806BE34029E1FC'
if (!(Test-Path -LiteralPath $Archive -PathType Leaf) -or (Get-FileHash -LiteralPath $Archive -Algorithm SHA256).Hash -ne $expected) {
    throw 'Obtain the reviewed Apache Lounge httpd-2.4.71-261112-Win64-VS18.zip; its SHA-256 must match the publisher checksum.'
}
$root = Split-Path $PSScriptRoot -Parent
$temporary = Join-Path $root "Saved/LauncherApacheBundle/$([guid]::NewGuid())"
$destination = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot 'LocalFeedServer/Apache24'))
New-Item -ItemType Directory -Path $temporary,$destination -Force | Out-Null
Expand-Archive -LiteralPath $Archive -DestinationPath $temporary
$source = Join-Path $temporary 'Apache24'

# Follow PE imports without running downloaded executables or invoking a compiler.
function Get-PeImports([string]$Filename) {
    $bytes = [IO.File]::ReadAllBytes($Filename)
    $pe = [BitConverter]::ToInt32($bytes,0x3c)
    if ([BitConverter]::ToUInt32($bytes,$pe) -ne 0x4550) { throw "Not a PE image: $Filename" }
    $count = [BitConverter]::ToUInt16($bytes,$pe+6)
    $size = [BitConverter]::ToUInt16($bytes,$pe+20)
    $optional = $pe+24
    $magic = [BitConverter]::ToUInt16($bytes,$optional)
    $directory = $optional + $(if ($magic -eq 0x20b) {112} elseif ($magic -eq 0x10b) {96} else {throw 'Unsupported PE image'})
    $importRva = [BitConverter]::ToUInt32($bytes,$directory+8)
    if (!$importRva) { return }
    $sections = @()
    for ($i=0;$i -lt $count;$i++) {
        $offset = $optional+$size+40*$i
        $sections += [pscustomobject]@{Virtual=[BitConverter]::ToUInt32($bytes,$offset+12); Raw=[BitConverter]::ToUInt32($bytes,$offset+20);
            Size=[Math]::Max([BitConverter]::ToUInt32($bytes,$offset+8),[BitConverter]::ToUInt32($bytes,$offset+16))}
    }
    function Resolve-Rva([long]$Rva) {
        foreach ($section in $sections) { if ($Rva -ge $section.Virtual -and $Rva -lt $section.Virtual+$section.Size) { return [int]($section.Raw+$Rva-$section.Virtual) } }
        throw "Unmapped PE import address in $Filename"
    }
    $descriptor = Resolve-Rva $importRva
    while (($nameRva=[BitConverter]::ToUInt32($bytes,$descriptor+12)) -ne 0) {
        $nameOffset = Resolve-Rva $nameRva
        $end=$nameOffset;while ($end -lt $bytes.Length -and $bytes[$end] -ne 0) {$end++}
        if ($end -eq $bytes.Length) { throw 'Unterminated PE import name' }
        [Text.Encoding]::ASCII.GetString($bytes,$nameOffset,$end-$nameOffset)
        $descriptor+=20
    }
}

$queue = [Collections.Generic.Queue[string]]::new()
$queue.Enqueue('bin/httpd.exe')
foreach ($module in 'authz_core','authz_host','alias','mime','dir') { $queue.Enqueue("modules/mod_$module.so") }
$copied = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
$external = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
while ($queue.Count) {
    $relative=$queue.Dequeue()
    if (!$copied.Add($relative)) { continue }
    $file=Join-Path $source $relative
    if (!(Test-Path -LiteralPath $file -PathType Leaf)) { throw "Missing required Apache dependency $relative" }
    $target=Join-Path $destination $relative
    New-Item -ItemType Directory -Path (Split-Path $target -Parent) -Force | Out-Null
    Copy-Item -LiteralPath $file -Destination $target -Force
    foreach ($dependency in Get-PeImports $file) {
        if (Test-Path -LiteralPath (Join-Path $source "bin/$dependency") -PathType Leaf) { $queue.Enqueue("bin/$dependency") }
        else { [void]$external.Add($dependency) }
    }
}
foreach ($relative in 'conf/mime.types','LICENSE.txt','NOTICE.txt','ABOUT_APACHE.txt') {
    $target=Join-Path $destination $relative
    New-Item -ItemType Directory -Path (Split-Path $target -Parent) -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $source $relative) -Destination $target -Force
    [void]$copied.Add($relative)
}
foreach ($name in 'ReadMe.txt','Security.txt') {
    Copy-Item -LiteralPath (Join-Path $temporary $name) -Destination (Join-Path $destination "Distribution-$name") -Force
    [void]$copied.Add("Distribution-$name")
}
$files = @($copied | Sort-Object | ForEach-Object {
    $path=Join-Path $destination $_
    @{path=$_;size=(Get-Item -LiteralPath $path).Length;sha256=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash}
})
@{version='2.4.71';build='2026-11-12';source='https://www.apachelounge.com/download/VS18/binaries/httpd-2.4.71-261112-Win64-VS18.zip';
    archiveSha256=$expected;checksum='https://www.apachelounge.com/download/VS18/binaries/httpd-2.4.71-261112-Win64-VS18.zip.txt';
    verification='Publisher HTTPS SHA-256 matched; retained PGP signature was not verified';
    windowsPrerequisite='Visual C++ x64 runtime 14.52.37029 or later (per distributor)';
    externalWindowsImports=@($external | Sort-Object);files=$files} | ConvertTo-Json -Depth 6 |
    Set-Content -LiteralPath (Join-Path $destination 'bundle-manifest.json') -Encoding utf8
Write-Output "Staged minimal static-file Apache runtime ($($files.Count) files), licenses and provenance in $destination"
