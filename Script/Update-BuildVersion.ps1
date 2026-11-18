param([Parameter(Mandatory)][string]$ProjectRoot)
$ErrorActionPreference = 'Stop'
$now = [DateTime]::UtcNow
$record = @{ version=$now.ToString("yyyyMMdd-HHmmss'Z'",[Globalization.CultureInfo]::InvariantCulture); builtUtc=$now.ToString('o'); channel='Alpha' }
$path = Join-Path $ProjectRoot 'Content/Build/build-version.json'
[void][IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($path))
$temporary = $path + '.tmp'
[IO.File]::WriteAllText($temporary,($record | ConvertTo-Json),[Text.UTF8Encoding]::new($false))
Move-Item -LiteralPath $temporary -Destination $path -Force
