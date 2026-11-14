param(
    [Parameter(Mandatory)][string]$Dist,
    [Parameter(Mandatory)][string]$Package,
    [string]$Artifact = 'SSW-Dev'
)
$ErrorActionPreference = 'Stop'
$distRoot = (Resolve-Path -LiteralPath $Dist).Path
$gameRoot = (Resolve-Path -LiteralPath $Package).Path
$executable = 'StarshatterWars\Binaries\Win64\StarshatterWars.exe'
foreach ($path in @(
    (Join-Path $gameRoot $executable),
    (Join-Path $distRoot 'start_protected_game.exe'),
    (Join-Path $distRoot 'EasyAntiCheat/EasyAntiCheat_EOS_Setup.exe'),
    (Join-Path $distRoot 'EasyAntiCheat/Settings.json')
)) {
    if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw "Required file missing: $path" }
}
$project = Split-Path $PSScriptRoot -Parent
$pattern = 'ArtifactName\s*=\s*"?'+ [regex]::Escape($Artifact) + '"?\s*[,)]'
$row = Get-Content (Join-Path $project 'Config/UserEngine.ini') |
    Where-Object { $_ -match '^\s*\+Artifacts\s*=' -and $_ -match $pattern } |
    Select-Object -First 1
if (!$row) { throw "EOS artifact '$Artifact' was not found." }
$settings = Get-Content (Join-Path $distRoot 'EasyAntiCheat/Settings.json') -Raw | ConvertFrom-Json -AsHashtable
foreach ($field in 'productid','sandboxid','deploymentid') {
    $match = [regex]::Match($row, "(?i)\b$field\s*=\s*\x22?([^,\x22)\s]+)")
    if (!$match.Success) { throw "EOS artifact '$Artifact' has no $field." }
    $settings[$field] = $match.Groups[1].Value
}
$settings['title'] = 'Starshatter Wars'
$settings['executable'] = $executable
if ((Test-Path (Join-Path $gameRoot 'EasyAntiCheat')) -or (Test-Path (Join-Path $gameRoot 'start_protected_game.exe'))) {
    throw 'Choose a package without existing Easy Anti-Cheat files.'
}
Copy-Item -LiteralPath (Join-Path $distRoot 'EasyAntiCheat') -Destination $gameRoot -Recurse
Copy-Item -LiteralPath (Join-Path $distRoot 'start_protected_game.exe') -Destination $gameRoot
$settings | ConvertTo-Json -Depth 16 | Set-Content -LiteralPath (Join-Path $gameRoot 'EasyAntiCheat/Settings.json') -Encoding utf8NoBOM
Write-Output "Configured Easy Anti-Cheat for $Artifact in $gameRoot"
