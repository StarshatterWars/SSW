using System.Diagnostics;
using System.IO;
using System.Net.Http;
using System.Text.Json;

namespace StarshatterWarsLauncher;

internal sealed record UniverseServerSettings(bool AutoStartLocal = true, string? PermanentStatusUrl = null, int Port = 17880, int CompatibleChangelist = 47537391);
internal sealed record UniversePeer(string Name = "Pilot", double PingMs = -1)
{ public string Ping => PingMs>=0?$"{PingMs:0} ms":"—"; }
internal sealed record UniverseStatus(int Protocol = 1, bool Online = false, string Server = "FractalVerse", string? Instance = null,
    int ProcessId = 0, long Revision = 0, string? UniverseUtc = null, long ServerUtc = 0, double UptimeSeconds = 0,
    int MaximumClients = 8, int ConnectedClients = 0, UniversePeer[]? Players = null, bool CheckpointReady = false,
    long LastCheckpointUtc = 0, int Inventories = 0, int Journeys = 0, int Jobs = 0);
internal sealed record UniverseConnection(string Label, string Detail, UniverseStatus? Status = null, bool Local = false);

// Local authority outlives the launcher. Only its own graceful stop request is used before updating files.
internal sealed class UniverseServerManager : IDisposable
{
    private static readonly JsonSerializerOptions Json = new(JsonSerializerDefaults.Web);
    internal static readonly string SaveDirectory = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
        "Fractal Dev Games", "FractalVerse", "LocalUniverse");
    private readonly HttpClient http = new() { Timeout = TimeSpan.FromSeconds(3), MaxResponseContentBufferSize=65536 };
    private Process? started;
    private string? instance;
    private bool localSelected;
    private bool remoteSelected;
    private bool suspended;
    public void ResumeAutoStart() => suspended=false;
    internal static bool IsFresh(UniverseStatus state, DateTimeOffset now) => state.Protocol == 1 && state.Online &&
        state.MaximumClients == 8 && state.ConnectedClients >= 0 && state.ConnectedClients <= 8 &&
        state.ServerUtc > 0 && state.ServerUtc <= DateTimeOffset.MaxValue.ToUnixTimeSeconds() &&
        double.IsFinite(state.UptimeSeconds) && state.UptimeSeconds>=0 && state.UptimeSeconds<=TimeSpan.MaxValue.TotalSeconds &&
        state.Revision>=0 && state.Inventories>=0 && state.Journeys>=0 && state.Jobs>=0 &&
        (state.Players?.Length??0)<=8 && (state.ConnectedClients==0 || state.Players?.Length==state.ConnectedClients) &&
        (state.Players??[]).All(peer=>peer is not null && !string.IsNullOrWhiteSpace(peer.Name) && peer.Name.Length<=64 &&
            double.IsFinite(peer.PingMs) && peer.PingMs>=-1 && peer.PingMs<=30000) &&
        Math.Abs(now.ToUnixTimeSeconds() - state.ServerUtc) <= 10;
    internal static string? FindExecutable(string? game)
    {
        if (game is null) return null;
        var dir = new DirectoryInfo(Path.GetDirectoryName(Path.GetFullPath(game))!);
        if (dir.Name.Equals("Win64",StringComparison.OrdinalIgnoreCase) && dir.Parent?.Name=="Binaries" && dir.Parent.Parent?.Name=="StarshatterWars")
            dir=dir.Parent.Parent.Parent!;
        if(!File.Exists(Path.Combine(dir.FullName,"StarshatterWars.exe")))return null;
        string exe=Path.Combine(dir.FullName,"FractalVerse","WindowsServer","StarshatterWars","Binaries","Win64","FractalVerse.exe");
        if(File.Exists(exe))return exe;
        return null; // Never substitute the arena StarServer executable.
    }
    private static UniverseStatus? LocalStatus()
    {
        try
        {
            var path=Path.Combine(SaveDirectory,"status.json");
            if(new FileInfo(path).Length>65536)return null;
            return JsonSerializer.Deserialize<UniverseStatus>(File.ReadAllText(path),Json);
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException or JsonException) { return null; }
    }
    public async Task<UniverseConnection> RefreshAsync(UniverseServerSettings settings, string? game, CancellationToken token)
    {
        token.ThrowIfCancellationRequested();
        if(suspended)return new("UPDATING", "Universe server is stopped while game files are updated.");
        if (!localSelected && settings.PermanentStatusUrl is { Length: > 0 } url)
        {
            try
            {
                var uri = new Uri(url);
                if (uri.Scheme != "https" && !(uri.Scheme == "http" && uri.IsLoopback))
                    return new("SERVER CONFIGURATION", "Permanent server status must use HTTPS.");
                string json = await http.GetStringAsync(uri,token);
                var status = JsonSerializer.Deserialize<UniverseStatus>(json,Json);
                if (status is not null && IsFresh(status,DateTimeOffset.UtcNow))
                { remoteSelected=true; return new("ONLINE", "Permanent universe server",status); }
            }
            catch (Exception e) when (e is HttpRequestException or TaskCanceledException or JsonException or UriFormatException) { }
            // Once attached to a permanent universe, do not silently fork it into a local world.
            if (remoteSelected) return new("RECONNECTING", "Permanent universe server is unavailable.");
        }
        token.ThrowIfCancellationRequested();
        if(suspended)return new("UPDATING", "Universe server is stopped while game files are updated.");
        var cached=LocalStatus();
        if (cached is not null && IsFresh(cached,DateTimeOffset.UtcNow) && ProcessAlive(cached.ProcessId))
        { localSelected=true;instance=cached.Instance;return new("ONLINE · LOCAL", "Persistent local FractalVerse",cached,true); }
        if (started is not null && !started.HasExited)
            return new("CONNECTING · LOCAL", "Starting FractalVerse and loading the universe…",null,true);
        if (started is not null)
            return new("DISCONNECTED", "FractalVerse stopped. Its universe checkpoint is retained.",null,true);
        if (!settings.AutoStartLocal) return new("DISCONNECTED", "Local universe auto-start is disabled.");
        string? exe=FindExecutable(game);
        if (exe is null) return new("SERVER NOT INSTALLED", "The FractalVerse package is required. StarServer is reserved for arena matches.");
        Directory.CreateDirectory(SaveDirectory);
        instance=Guid.NewGuid().ToString("N");
        var start=new ProcessStartInfo(exe){WorkingDirectory=Path.GetDirectoryName(exe),UseShellExecute=false,CreateNoWindow=true,WindowStyle=ProcessWindowStyle.Hidden};
        var arguments=new List<string>{"/Game/Maps/Transition?game=/Script/StarshatterWars.FractalVerseGameMode","-server","-SSWFractalVerseServer",
            "-MULTIHOME=127.0.0.1","-port="+Math.Clamp(settings.Port,1024,65535),"-FractalVerseSaveDir="+SaveDirectory,"-FractalVerseInstance="+instance,"-abslog="+Path.Combine(SaveDirectory,"FractalVerse.log"),"-unattended","-nosound","-NullRHI"};
        // Match the verified UE 5.7 compatible changelist, retaining protocol/game checks.
        // Source builds can report zero despite declaring this value in Build.version.
        if (settings.CompatibleChangelist > 0) arguments.Add("-networkversionoverride=" + settings.CompatibleChangelist);
        start.Arguments=string.Join(" ",arguments.Select(UnrealArgument));
        started=Process.Start(start) ?? throw new IOException("FractalVerse could not start.");
        localSelected=true;
        return new("CONNECTING · LOCAL", "Starting FractalVerse and loading the universe…",null,true);
    }
    // Unreal reads its raw command line: quote option values, not the entire -key=value token.
    internal static string UnrealArgument(string argument)
    {
        if (argument.IndexOfAny(['"','\r','\n','\0'])>=0) throw new ArgumentException("Invalid Unreal argument.",nameof(argument));
        if (!argument.Any(char.IsWhiteSpace)) return argument;
        int separator=argument.IndexOf('=');
        return separator>0 ? argument[..(separator+1)]+"\""+argument[(separator+1)..]+"\"" : "\""+argument+"\"";
    }
    private static bool ProcessAlive(int id)
    {try {using var p=Process.GetProcessById(id);return !p.HasExited && (p.ProcessName.Equals("FractalVerse",StringComparison.OrdinalIgnoreCase) || p.ProcessName.StartsWith("FractalVerse-",StringComparison.OrdinalIgnoreCase));}catch(Exception ex) when(ex is ArgumentException or InvalidOperationException or System.ComponentModel.Win32Exception){return false;}}
    public async Task StopForUpdateAsync(CancellationToken token)
    {
        suspended=true;
        var status=LocalStatus();
        if(status is null || !ProcessAlive(status.ProcessId) || string.IsNullOrEmpty(status.Instance))return;
        await File.WriteAllTextAsync(Path.Combine(SaveDirectory,"shutdown.request"),status.Instance,token);
        using var p=Process.GetProcessById(status.ProcessId);
        using var timeout=CancellationTokenSource.CreateLinkedTokenSource(token);timeout.CancelAfter(TimeSpan.FromSeconds(20));
        try {await p.WaitForExitAsync(timeout.Token);}catch(OperationCanceledException) when(!token.IsCancellationRequested)
        {throw new IOException("FractalVerse is still saving. Wait for the server to close before updating.");}
        started?.Dispose();started=null;localSelected=false;
    }
    public void Dispose(){started?.Dispose();http.Dispose();}
}
