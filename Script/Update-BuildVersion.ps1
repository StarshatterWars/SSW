param([Parameter(Mandatory)][string]$ProjectRoot)
$ErrorActionPreference = 'Stop'
$now = [DateTime]::UtcNow
$releaseInfo = Get-Content -LiteralPath (Join-Path $ProjectRoot 'Content/Build/release-info.json') -Raw | ConvertFrom-Json
if ([string]::IsNullOrWhiteSpace($releaseInfo.stage)) { throw 'Release stage is missing from release-info.json' }
$record = @{ version=$now.ToString("yyyyMMdd-HHmmss'Z'",[Globalization.CultureInfo]::InvariantCulture); builtUtc=$now.ToString('o'); channel=$releaseInfo.stage }
$path = Join-Path $ProjectRoot 'Content/Build/build-version.json'
[void][IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($path))
$temporary = $path + '.tmp'
[IO.File]::WriteAllText($temporary,($record | ConvertTo-Json),[Text.UTF8Encoding]::new($false))
Move-Item -LiteralPath $temporary -Destination $path -Force
