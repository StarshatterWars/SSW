#Requires -Version 7.0
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Package,
    [string]$ReportDirectory,
    [ValidateRange(15,300)][int]$TimeoutSeconds=120
)
$ErrorActionPreference='Stop'
$packagePath=[IO.Path]::GetFullPath($Package)
$exe=Join-Path $packagePath 'StarshatterWars/Binaries/Win64/StarshatterWars.exe'
if(!(Test-Path -LiteralPath $exe -PathType Leaf)){throw "Packaged client executable missing: $exe"}
if(!$ReportDirectory){$ReportDirectory=Join-Path ([IO.Path]::GetTempPath()) ('SSW-Startup-'+[guid]::NewGuid().ToString('N'))}
$reportPath=[IO.Path]::GetFullPath($ReportDirectory)
[void][IO.Directory]::CreateDirectory($reportPath)
$userPath=Join-Path $reportPath 'IsolatedUser'
$logPath=Join-Path $reportPath 'PackagedStartup.log'
$start=[Diagnostics.ProcessStartInfo]::new($exe)
$start.WorkingDirectory=$packagePath;$start.UseShellExecute=$false;$start.CreateNoWindow=$true;$start.WindowStyle=[Diagnostics.ProcessWindowStyle]::Hidden
# UserDir isolates account/checkpoint paths; no pilot recovery or real save edits.
foreach($arg in @('-unattended','-nosplash','-nosound','-NullRHI','-ExecCmds=quit',('-UserDir='+$userPath),('-abslog='+$logPath),'-ini:Engine:[OnlineSubsystem]:DefaultPlatformService=NULL','-ini:Game:[/Script/StarshatterWars.SSWDiagnosticsSettings]:bRequirePlatformSignIn=False')){
    $start.ArgumentList.Add($arg)
}
$watch=[Diagnostics.Stopwatch]::StartNew()
$probe=[Diagnostics.Process]::Start($start)
if(!$probe){throw 'Could not start packaged startup probe.'}
try{
    if(!$probe.WaitForExit($TimeoutSeconds*1000)){
        $probe.Kill($true);$probe.WaitForExit()
        throw "Packaged startup timed out. Nothing should be published. Log: $logPath"
    }
    $exitCode=$probe.ExitCode
    $result=@{executable=$exe;exitCode=$exitCode;seconds=$watch.Elapsed.TotalSeconds;isolatedUser=$userPath;log=$logPath}
    [IO.File]::WriteAllText((Join-Path $reportPath 'startup-result.json'),($result|ConvertTo-Json))
    if($exitCode -ne 0){throw "Packaged startup failed (exit $exitCode). Nothing should be published. Log: $logPath"}
    if(!(Test-Path -LiteralPath $logPath)){throw 'Packaged startup produced no log; startup could not be verified.'}
    $text=[IO.File]::ReadAllText($logPath)
    if($text -match 'Fatal error:|ObjectSerializationError:'){throw "Packaged startup reported a fatal error. Log: $logPath"}
    Write-Host ('Packaged offline startup passed in {0:N2}s; isolated saves; log: {1}' -f $watch.Elapsed.TotalSeconds,$logPath)
    [pscustomobject]$result
}finally{$probe.Dispose()}
