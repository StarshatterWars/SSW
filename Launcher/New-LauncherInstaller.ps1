param(
    [Parameter(Mandatory)][string]$Wix,
    [ValidatePattern('^\d+\.\d+\.\d+$')][string]$Version = '1.0.0'
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$payload = Join-Path $root "Saved/LauncherInstaller/$([guid]::NewGuid())"
$output = Join-Path $root 'Build/LauncherInstaller'
New-Item -ItemType Directory -Path $payload, $output -Force | Out-Null
dotnet publish (Join-Path $PSScriptRoot 'StarshatterWarsLauncher.csproj') -c Release -r win-x64 --self-contained true -p:DebugType=None -o $payload
if ($LASTEXITCODE -ne 0) { throw 'Launcher publish failed' }

function Escape-Xml([string]$Value) { [Security.SecurityElement]::Escape($Value) }
function Write-Payload([string]$Folder) {
    foreach ($file in Get-ChildItem -LiteralPath $Folder -File) {
        $name = Escape-Xml $file.FullName
        '<Component><File Source="' + $name + '" KeyPath="yes" /></Component>'
    }
    foreach ($child in Get-ChildItem -LiteralPath $Folder -Directory) {
        '<Directory Name="' + (Escape-Xml $child.Name) + '">'
        Write-Payload $child.FullName
        '</Directory>'
    }
}
$files = (Write-Payload $payload) -join [Environment]::NewLine
$icon = Escape-Xml (Join-Path $root 'Application.ico')
$source = Join-Path (Split-Path $payload -Parent) "Launcher-$Version-$([guid]::NewGuid()).wxs"
@"
<Wix xmlns="http://wixtoolset.org/schemas/v4/wxs">
  <Package Name="Starshatter Wars Launcher" Manufacturer="Fractal Dev Games" Version="$Version"
           UpgradeCode="{5E6187B2-2054-4A42-AD53-7158D9336393}" Scope="perUser">
    <MajorUpgrade DowngradeErrorMessage="A newer version of the launcher is already installed." />
    <MediaTemplate EmbedCab="yes" />
    <Icon Id="AppIcon" SourceFile="$icon" />
    <Property Id="ARPPRODUCTICON" Value="AppIcon" />
    <StandardDirectory Id="LocalAppDataFolder">
      <Directory Name="Fractal Dev Games">
        <Directory Id="INSTALLFOLDER" Name="Starshatter Wars Launcher">
          $files
        </Directory>
      </Directory>
    </StandardDirectory>
    <StandardDirectory Id="ProgramMenuFolder">
      <Component Id="LauncherShortcut">
        <Shortcut Id="StartMenuLauncher" Name="Starshatter Wars Launcher"
                  Target="[INSTALLFOLDER]StarshatterWarsLauncher.exe" WorkingDirectory="INSTALLFOLDER" Icon="AppIcon" />
        <RegistryValue Root="HKCU" Key="Software\Fractal Dev Games\Starshatter Wars Launcher"
                       Name="Installed" Type="integer" Value="1" KeyPath="yes" />
      </Component>
    </StandardDirectory>
  </Package>
</Wix>
"@ | Set-Content -LiteralPath $source -Encoding utf8
& $Wix build -arch x64 $source -out (Join-Path $output "StarshatterWarsLauncher-$Version-x64.msi")
if ($LASTEXITCODE -ne 0) { throw 'MSI build failed' }
