using System.Diagnostics;
using System.IO;
using System.Net.Http;
using System.Text.Json;
namespace StarshatterWarsLauncher;
internal sealed record EconomyStatus(int Protocol=0,string Server="",int HandoffProtocol=0,bool Online=false,bool CheckpointReady=false,bool NpcAuthorityActive=false,
    string? WorldId=null,int ProcessId=0,long ServerUtc=0,long Revision=0,double UptimeSeconds=0,double EconomySeconds=0,int Inventories=0,int ActiveJobs=0,int DeliveredFreight=0);
internal sealed record EconomyConnection(string Label,string Detail,string Endpoint,EconomyStatus? Status=null,double PingMs=-1);
internal sealed class EconomyServerManager:IDisposable
{
    internal static string StateDirectory => Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"Fractal Dev Games","FractalEconomy","LocalEconomy");
    private readonly HttpClient http=new(){Timeout=TimeSpan.FromSeconds(3),MaxResponseContentBufferSize=65536};
    private static readonly JsonSerializerOptions Json=new(JsonSerializerDefaults.Web);
    private readonly string? settingsFile;
    private readonly string stateDirectory;
    internal EconomyServerManager(string? settingsFile=null,string? stateDirectory=null){this.settingsFile=settingsFile;this.stateDirectory=stateDirectory??StateDirectory;}
    private Process? started;
    private string? pinnedEndpoint;
    private bool suspended;
    private readonly SemaphoreSlim lifecycle=new(1,1);
    private int startAttempts;
    private DateTimeOffset nextStartAttempt;
    internal void ResumeAutoStart()=>suspended=false;
    internal static bool IsFresh(EconomyStatus value,DateTimeOffset now)=>value.Protocol==1&&value.HandoffProtocol==1&&value.Server=="FractalEconomy"&&value.Online&&value.ProcessId>0&&
        (!value.NpcAuthorityActive||!string.IsNullOrWhiteSpace(value.WorldId))&&value.ServerUtc>0&&Math.Abs(now.ToUnixTimeSeconds()-value.ServerUtc)<=10&&value.Revision>=0&&value.Revision<=9007199254740991L&&
        double.IsFinite(value.UptimeSeconds)&&value.UptimeSeconds>=0&&value.UptimeSeconds<TimeSpan.MaxValue.TotalSeconds-1&&
        double.IsFinite(value.EconomySeconds)&&value.EconomySeconds>=0&&value.Inventories>=0&&value.ActiveJobs>=0&&value.DeliveredFreight>=0;
    internal static string? FindExecutable(string? game)
    {
        if(game is null)return null;var dir=new DirectoryInfo(Path.GetDirectoryName(Path.GetFullPath(game))!);
        if(dir.Name.Equals("Win64",StringComparison.OrdinalIgnoreCase)&&dir.Parent?.Name=="Binaries"&&dir.Parent.Parent?.Name=="StarshatterWars")dir=dir.Parent.Parent.Parent!;
        if(!File.Exists(Path.Combine(dir.FullName,"StarshatterWars.exe")))return null;
        string exe=Path.Combine(dir.FullName,"EconomyServer","FractalEconomy.exe");return File.Exists(exe)?exe:null;
    }
    internal static bool ReadyPackage(string executable)
    {
        try{return VerifyPackage(executable);}
        catch(Exception ex)when(ex is IOException or UnauthorizedAccessException or JsonException or KeyNotFoundException or InvalidOperationException or FormatException or ArgumentException){return false;}
    }
    private static bool VerifyPackage(string executable)
    {
        string file=Path.Combine(Path.GetDirectoryName(executable)!,"server-manifest.json");if(!File.Exists(file))return false;
        using var doc=JsonDocument.Parse(File.ReadAllText(file));var root=doc.RootElement;
        if(!root.TryGetProperty("liveClientCutoverReady",out var live)||live.ValueKind!=JsonValueKind.True)return false;
        if(!root.TryGetProperty("standbyAutostartReady",out var ready)||ready.ValueKind!=JsonValueKind.True||root.GetProperty("protocol").GetInt32()!=1||root.GetProperty("handoffProtocol").GetInt32()!=1||root.GetProperty("server").GetString()!="FractalEconomy")return false;
        var directory=Path.GetDirectoryName(Path.GetFullPath(executable))!;
        foreach(var entry in root.GetProperty("files").EnumerateArray()){
            string relative=entry.GetProperty("path").GetString()??"";
            if(Path.IsPathRooted(relative)||relative.Contains(':')||relative.Split('/', '\\').Any(part=>part is ".." or "." or ""))return false;
            string path=Path.GetFullPath(Path.Combine(directory,relative));
            if(!path.StartsWith(directory+Path.DirectorySeparatorChar,StringComparison.OrdinalIgnoreCase)||!File.Exists(path)||new FileInfo(path).Length!=entry.GetProperty("length").GetInt64())return false;
            using var stream=File.OpenRead(path);string hash=Convert.ToHexString(System.Security.Cryptography.SHA256.HashData(stream));
            if(!hash.Equals(entry.GetProperty("sha256").GetString(),StringComparison.OrdinalIgnoreCase))return false;
        }
        return root.GetProperty("files").EnumerateArray().Any(entry=>entry.GetProperty("path").GetString()=="FractalEconomy.exe");
    }
    internal async Task<EconomyConnection> RefreshAsync(string? game,CancellationToken token)
    {
        await lifecycle.WaitAsync(token);
        try{return await RefreshCoreAsync(game,token);}finally{lifecycle.Release();}
    }
    private async Task<EconomyConnection> RefreshCoreAsync(string? game,CancellationToken token)
    {
        ServiceEndpoints settings;
        try{settings=ServiceEndpoints.Load(settingsFile);}catch(Exception ex)when(ex is IOException or JsonException or UnauthorizedAccessException or InvalidOperationException or KeyNotFoundException)
        {return new("CONFIGURATION ERROR",ex.Message,"Shared service settings are invalid.");}
        if(pinnedEndpoint is not null&&pinnedEndpoint!=settings.EconomyEndpoint)return new("ENDPOINT CHANGED","Restart the launcher when the game is idle to apply the new server. Active transactions never switch worlds silently.",settings.EconomyEndpoint);
        pinnedEndpoint??=settings.EconomyEndpoint;
        if(suspended)return new("UPDATING","Economy auto-start is suspended while game files are updated.",pinnedEndpoint);
        try{
            var began=Stopwatch.GetTimestamp();var text=await http.GetStringAsync(pinnedEndpoint+"/status",token);var status=JsonSerializer.Deserialize<EconomyStatus>(text,Json);
            if(status is not null&&IsFresh(status,DateTimeOffset.UtcNow)){
                string label=!status.CheckpointReady?"CHECKPOINT BLOCKED":status.NpcAuthorityActive&&!string.IsNullOrEmpty(status.WorldId)?"ONLINE":"AWAITING HANDOFF";
                return new(label,status.NpcAuthorityActive?"NPC economy is advanced by this service.":"Service is available; the live NPC handoff has not been activated.",pinnedEndpoint,status,Stopwatch.GetElapsedTime(began).TotalMilliseconds);
            }
            return new("INVALID HEARTBEAT","The endpoint answered with incompatible or stale status. No replacement service is started.",pinnedEndpoint);
        }catch(OperationCanceledException)when(token.IsCancellationRequested){throw;}
        catch(Exception ex)when(ex is HttpRequestException or TaskCanceledException or JsonException){ }
        token.ThrowIfCancellationRequested();if(suspended)return new("UPDATING","Economy auto-start is suspended.",pinnedEndpoint);
        if(!settings.IsLocal)return new("DISCONNECTED","Remote economy unavailable. No local replacement is started.",pinnedEndpoint);
        if(!settings.AutoStartLocalEconomy)return new("DISCONNECTED","Local economy auto-start is disabled.",pinnedEndpoint);
        string? exe=FindExecutable(game);if(exe is null)return new("NOT INSTALLED","The native economy package will be included with the verified live-economy build.",pinnedEndpoint);
        if(!ReadyPackage(exe))return new("INTEGRATION PENDING","The installed native economy package is incomplete or not verified for standby startup.",pinnedEndpoint);
        if(started is not null){
            if(!started.HasExited)return new("STARTING","Loading the persistent local NPC economy…",pinnedEndpoint);
            started.Dispose();started=null;
        }
        if(startAttempts>=3)return new("START FAILED","The economy could not restart after three attempts. Its checkpoint is retained; inspect the service before retrying.",pinnedEndpoint);
        if(DateTimeOffset.UtcNow<nextStartAttempt)return new("RECONNECTING","Waiting before restarting the same persistent economy. No replacement world is created.",pinnedEndpoint);
        var uri=new Uri(pinnedEndpoint);if(uri.Host is not ("127.0.0.1" or "localhost")||uri.Scheme!="http"||uri.AbsolutePath!="/")return new("LOCAL START UNAVAILABLE","Bundled local service requires an HTTP IPv4 localhost endpoint without a path prefix.",pinnedEndpoint);
        var start=StartInfo(exe,stateDirectory,uri.Port);
        ++startAttempts;nextStartAttempt=DateTimeOffset.UtcNow.AddSeconds(Math.Min(60,5*startAttempts));
        Directory.CreateDirectory(stateDirectory);started=Process.Start(start)??throw new IOException("Native economy service could not start.");
        return new("STARTING","Starting the persistent local NPC economy…",pinnedEndpoint);
    }
    internal static ProcessStartInfo StartInfo(string exe,string state,int port)
    {
        if(port<1024||port>65535)throw new ArgumentOutOfRangeException(nameof(port));
        var start=new ProcessStartInfo(exe){UseShellExecute=false,CreateNoWindow=true,WindowStyle=ProcessWindowStyle.Hidden,WorkingDirectory=Path.GetDirectoryName(exe)};
        foreach(string arg in new[]{"--data",Path.Combine(Path.GetDirectoryName(exe)!,"GameData"),"--state",state,"--port",port.ToString(System.Globalization.CultureInfo.InvariantCulture),"--await-bootstrap"})start.ArgumentList.Add(arg);
        return start;
    }
    internal async Task StopForUpdateAsync(string? game,CancellationToken token)
    {
        suspended=true;await lifecycle.WaitAsync(token);
        try{await StopCoreAsync(game,token);}finally{lifecycle.Release();}
    }
    private async Task StopCoreAsync(string? game,CancellationToken token)
    {
        var settings=ServiceEndpoints.Load(settingsFile);if(!settings.IsLocal)return;
        EconomyStatus? status;
        try{status=JsonSerializer.Deserialize<EconomyStatus>(await http.GetStringAsync(settings.EconomyEndpoint+"/status",token),Json);}
        catch(HttpRequestException){return;}catch(TaskCanceledException)when(!token.IsCancellationRequested){throw new IOException("Economy shutdown timed out. Game files were not replaced.");}
        if(status is null||!IsFresh(status,DateTimeOffset.UtcNow))throw new IOException("Cannot verify the running economy service before updating.");
        string? exe=FindExecutable(game);if(exe is null)throw new IOException("The economy service is running outside the selected installation. Stop it before updating.");
        using var process=Process.GetProcessById(status.ProcessId);
        if(!string.Equals(process.MainModule?.FileName,exe,StringComparison.OrdinalIgnoreCase))throw new IOException("The economy process does not match this installation.");
        string key=(await File.ReadAllTextAsync(Path.Combine(stateDirectory,"authority.key"),token)).Trim();
        using var request=new HttpRequestMessage(HttpMethod.Post,settings.EconomyEndpoint+"/shutdown");request.Headers.Authorization=new("Bearer",key);
        using var response=await http.SendAsync(request,token);response.EnsureSuccessStatusCode();
        using var deadline=CancellationTokenSource.CreateLinkedTokenSource(token);deadline.CancelAfter(TimeSpan.FromSeconds(15));
        await process.WaitForExitAsync(deadline.Token);
    }
    public void Dispose(){http.Dispose();started?.Dispose();}
}
