#Requires -Version 7.0
param(
 [Parameter(Mandatory)][string]$Phase,
 [string]$Commit='HEAD',
 [Parameter(Mandatory)][string]$Verification,
 [string]$Limitations='Manual gameplay verification and cooking/packaging have not been performed.',
 [string]$ProjectRoot=(Join-Path $PSScriptRoot '..')
)
$ErrorActionPreference='Stop'
$root=(Resolve-Path -LiteralPath $ProjectRoot).Path
$sha=(& git -C $root rev-parse --verify "$Commit^{commit}").Trim()
if($LASTEXITCODE -ne 0){throw 'Commit not found'}
if($Phase -notmatch '^[a-zA-Z0-9_-]+$'){throw 'Use a simple phase name'}
$paths=@(& git -C $root diff-tree --root --no-commit-id --name-only --diff-filter=ACMR -r $sha)
if(!$paths.Count -or $paths|Where-Object {$_ -match '(^|/)(Binaries|Intermediate|Saved|bin|obj)/|\.def$'}){throw 'Commit contains excluded generated or legacy DEF files'}
$folder=Join-Path $root 'Build/Sprint/2026-11-26'
[void][IO.Directory]::CreateDirectory($folder)
$zip=Join-Path $folder ($Phase+'-'+$sha.Substring(0,9)+'.zip')
if(Test-Path -LiteralPath $zip){throw "Bundle already exists: $zip"}
& git -C $root archive --format=zip "--output=$zip" $sha -- @paths
if($LASTEXITCODE -ne 0){throw 'Source archive failed'}
$archive=[IO.Compression.ZipFile]::Open($zip,[IO.Compression.ZipArchiveMode]::Update)
try{
 $manifest=[ordered]@{phase=$Phase;commit=$sha;createdUtc=[DateTime]::UtcNow.ToString('o');files=$paths;verification=$Verification;limitations=$Limitations}
 $entry=$archive.CreateEntry('SPRINT-MANIFEST.json');$writer=[IO.StreamWriter]::new($entry.Open());try{$writer.Write(($manifest|ConvertTo-Json -Depth 5))}finally{$writer.Dispose()}
 $entry=$archive.CreateEntry('BUILD-AND-VERIFY.txt');$writer=[IO.StreamWriter]::new($entry.Open())
 try{$writer.Write("Complete repository files, not patch fragments. Base: SSW $sha.`r`nApply to the matching repository preserving paths. Close UE Editor. Build with Launcher/Build-Cook-Deploy.cmd (tests, cook, package and signed launcher publication), or use the installed UE 5.7 Build.bat for StarshatterWarsEditor / StarshatterWars Win64 Development.`r`nValidation: $Verification`r`nLimitations: $Limitations`r`n")}finally{$writer.Dispose()}
}finally{$archive.Dispose()}
Get-FileHash -LiteralPath $zip -Algorithm SHA256|Select-Object Path,Hash
