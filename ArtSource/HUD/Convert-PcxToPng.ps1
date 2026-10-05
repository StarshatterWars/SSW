<#
.SYNOPSIS
Converts PCX files to PNG with black made transparent using ImageMagick 7.
.EXAMPLE
.\Convert-PcxToPng.ps1 -Directory 'D:\Textures'
.EXAMPLE
.\Convert-PcxToPng.ps1 -Directory 'D:\Textures' -Recurse -OutputDirectory 'D:\PNG'
.NOTES
Requires ImageMagick 7 (magick.exe) on PATH, or supply -MagickPath.
Existing PNGs are skipped unless -Overwrite is supplied. Source PCX files are preserved.
#>
[CmdletBinding(SupportsShouldProcess)]
param(
    [Parameter(Position = 0)]
    [ValidateScript({ Test-Path -LiteralPath $_ -PathType Container })]
    [string]$Directory = (Get-Location).Path,
    [string]$OutputDirectory,
    [switch]$Recurse,
    [switch]$Overwrite,
    [ValidateRange(0, 100)]
    [double]$FuzzPercent = 0,
    [string]$MagickPath = 'magick.exe'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$converter = Get-Command -Name $MagickPath -CommandType Application -ErrorAction SilentlyContinue |
    Select-Object -First 1
if (-not $converter) {
    throw 'ImageMagick 7 was not found. Install it with magick.exe on PATH, or provide -MagickPath with its full path.'
}
$sourceRoot = (Get-Item -LiteralPath $Directory).FullName
$sourcePrefix = $sourceRoot.TrimEnd([char[]]'\/') + [IO.Path]::DirectorySeparatorChar
$destinationRoot = if ($OutputDirectory) {
    $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutputDirectory)
} else { $sourceRoot }
$files = @(Get-ChildItem -LiteralPath $sourceRoot -File -Recurse:$Recurse |
    Where-Object { $_.Extension -ieq '.pcx' })
$converted = 0
$skipped = 0
$failed = 0
$fuzz = $FuzzPercent.ToString([Globalization.CultureInfo]::InvariantCulture) + '%'

foreach ($file in $files) {
    $relative = $file.FullName.Substring($sourcePrefix.Length)
    $destination = Join-Path $destinationRoot ([IO.Path]::ChangeExtension($relative, '.png'))
    if ((Test-Path -LiteralPath $destination) -and -not $Overwrite) {
        Write-Verbose "Skipping existing PNG: $destination"
        $skipped++
        continue
    }
    if (-not $PSCmdlet.ShouldProcess($destination, "Convert '$($file.FullName)' and make black transparent")) {
        continue
    }
    $temporary = $null
    try {
        $parent = Split-Path -Parent $destination
        [IO.Directory]::CreateDirectory($parent) | Out-Null
        # Convert to a temporary PNG so a failed conversion cannot damage an existing output.
        $temporary = Join-Path $parent (([Guid]::NewGuid().ToString('N')) + '.png')
        $arguments = @(
            $file.FullName,
            '-alpha', 'on',
            '-fuzz', $fuzz,
            '-transparent', 'black',
            "PNG32:$temporary"
        )
        & $converter.Source @arguments
        if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $temporary -PathType Leaf)) {
            throw "ImageMagick failed with exit code $LASTEXITCODE."
        }
        if ($Overwrite) {
            Move-Item -LiteralPath $temporary -Destination $destination -Force
        } else {
            [IO.File]::Move($temporary, $destination)
        }
        $converted++
        Write-Host "Converted: $destination"
    }
    catch {
        $failed++
        Write-Warning "Failed '$($file.FullName)': $($_.Exception.Message)"
    }
    finally {
        if ($temporary -and (Test-Path -LiteralPath $temporary)) {
            Remove-Item -LiteralPath $temporary -Force
        }
    }
}
Write-Host "Finished: $converted converted, $skipped skipped, $failed failed; $($files.Count) PCX files found."
if ($failed -gt 0) { throw "$failed conversion(s) failed. See warnings above." }

