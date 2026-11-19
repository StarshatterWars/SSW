using System.Diagnostics;
using System.IO;
using System.Net;
using System.Net.Http;
using System.Net.Sockets;
using System.Runtime.InteropServices;
using System.Text;
using Microsoft.Win32.SafeHandles;

namespace StarshatterWarsLauncher;

internal sealed record LocalFeedServerSettings(bool Enabled = false, string? DocumentRoot = null,
    int Port = 18787, int ReadyTimeoutSeconds = 12);

/// <summary>Owns one opt-in loopback-only Apache process, never an existing server/service.</summary>
internal sealed class LocalFeedServer : IDisposable
{
    private readonly SemaphoreSlim gate = new(1, 1);
    private readonly object ownership = new();
    private readonly StringBuilder diagnostics = new();
    private Process? child;
    private SafeFileHandle? job;
    private Uri? health;
    private string? healthToken;
    private bool disposed;

    internal async Task EnsureReadyAsync(LocalFeedServerSettings? settings, IEnumerable<string?> urls,
        CancellationToken token)
    {
        if (settings?.Enabled != true) return;
        Uri[] local = urls.Where(s => Uri.TryCreate(s, UriKind.Absolute, out Uri? u) &&
                u.Scheme == Uri.UriSchemeHttp && u.IsLoopback)
            .Select(s => new Uri(s!, UriKind.Absolute)).ToArray();
        // A hosted HTTPS feed never needs the bundled local server.
        if (local.Length == 0) return;
        if (settings.Port is < 1024 or > 65535 || local.Any(u => u.Host != "127.0.0.1" || u.Port != settings.Port))
            throw new IOException("The local feed must use 127.0.0.1 and the configured local server port.");
        await gate.WaitAsync(token);
        try
        {
            ObjectDisposedException.ThrowIf(disposed, this);
            using var http = new HttpClient(new HttpClientHandler { AllowAutoRedirect = false, UseProxy = false })
                { Timeout = TimeSpan.FromSeconds(1), MaxResponseContentBufferSize = 1024 };
            if (child is { HasExited: false } && await HealthyAsync(http, token)) return;
            StopOwned();
            string root = ResolveFeedRoot(settings.DocumentRoot);
            if (!Directory.Exists(root) || (File.GetAttributes(root) & FileAttributes.ReparsePoint) != 0)
                throw new IOException($"Local installation files are unavailable in {root}. Restore the release feed or configure a hosted update URL.");
            foreach (Uri manifest in local.Where(u => u.AbsolutePath.EndsWith("/manifest.json", StringComparison.OrdinalIgnoreCase)))
                if (!File.Exists(Path.Combine(root, Uri.UnescapeDataString(manifest.AbsolutePath).TrimStart('/').Replace('/', Path.DirectorySeparatorChar))))
                    throw new IOException($"Local installation files are unavailable in {root}. The signed release manifest has not been staged.");
            string runtime = Path.Combine(AppContext.BaseDirectory, "LocalFeedServer", "Apache24");
            string executable = Path.Combine(runtime, "bin", "httpd.exe");
            if (!File.Exists(executable)) throw new IOException("The bundled local update server is missing. Reinstall the launcher.");
            if (!PortAvailable(settings.Port))
                throw new IOException($"Local update port {settings.Port} is already in use. The launcher will not stop another server.");
            string state = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
                "Fractal Dev Games", "Starshatter Wars Launcher", "LocalFeedServer", Guid.NewGuid().ToString("N"));
            Directory.CreateDirectory(state);
            healthToken = Guid.NewGuid().ToString("N");
            await File.WriteAllTextAsync(Path.Combine(state, "health.txt"), healthToken, token);
            string config = Path.Combine(state, "httpd.conf");
            await File.WriteAllTextAsync(config, BuildConfiguration(runtime, root, state, settings.Port), new UTF8Encoding(false), token);
            health = new Uri($"http://127.0.0.1:{settings.Port}/__ssw_launcher_health");
            var start = new ProcessStartInfo(executable)
            {
                WorkingDirectory = runtime, UseShellExecute = false, CreateNoWindow = true,
                WindowStyle = ProcessWindowStyle.Hidden, RedirectStandardOutput = true, RedirectStandardError = true
            };
            start.ArgumentList.Add("-d"); start.ArgumentList.Add(runtime);
            start.ArgumentList.Add("-f"); start.ArgumentList.Add(config);
            // One process avoids an unmanaged Apache worker escaping ownership.
            start.ArgumentList.Add("-X");
            var process = new Process { StartInfo = start };
            process.OutputDataReceived += Capture; process.ErrorDataReceived += Capture;
            SafeFileHandle lifetime = CreateLifetimeJob();
            lock (ownership)
            {
                if (disposed) { lifetime.Dispose(); process.Dispose(); throw new ObjectDisposedException(nameof(LocalFeedServer)); }
                try
                {
                    if (!process.Start()) throw new IOException("Could not start the local update server.");
                    child = process; job = lifetime;
                    if (!AssignProcessToJobObject(lifetime, process.Handle))
                        throw new IOException("Could not attach the local update server to the launcher's lifetime.");
                    process.BeginOutputReadLine(); process.BeginErrorReadLine();
                }
                catch { StopOwned(); lifetime.Dispose(); process.Dispose(); throw; }
            }
            using var timeout = CancellationTokenSource.CreateLinkedTokenSource(token);
            timeout.CancelAfter(TimeSpan.FromSeconds(Math.Clamp(settings.ReadyTimeoutSeconds, 2, 30)));
            try
            {
                while (!await HealthyAsync(http, timeout.Token))
                {
                    if (process.HasExited) throw new IOException("The local update server could not start. " + DiagnosticText());
                    await Task.Delay(150, timeout.Token);
                }
            }
            catch (OperationCanceledException) when (!token.IsCancellationRequested)
            { throw new IOException("The local update server did not become ready in time. " + DiagnosticText()); }
        }
        catch (System.ComponentModel.Win32Exception ex)
        { StopOwned(); throw new IOException("Could not run the bundled update server. Check the Visual C++ runtime installation. " + ex.Message, ex); }
        catch { StopOwned(); throw; }
        finally { gate.Release(); }
    }

    private async Task<bool> HealthyAsync(HttpClient http, CancellationToken token)
    {
        if (health is null) return false;
        try { return await http.GetStringAsync(health, token) == healthToken; }
        catch (HttpRequestException) { return false; }
        catch (OperationCanceledException) when (!token.IsCancellationRequested) { return false; }
    }

    private void Capture(object sender, DataReceivedEventArgs args)
    { if (args.Data is not null) lock (diagnostics) if (diagnostics.Length < 8192) diagnostics.AppendLine(args.Data); }
    private string DiagnosticText() { lock (diagnostics) return diagnostics.ToString().Trim(); }

    internal static string ResolveFeedRoot(string? configured)
    {
        string selected = Environment.ExpandEnvironmentVariables(string.IsNullOrWhiteSpace(configured) ? "LauncherFeed" : configured);
        string path = Path.GetFullPath(selected, AppContext.BaseDirectory);
        if (Path.TrimEndingDirectorySeparator(path).Equals(Path.TrimEndingDirectorySeparator(Path.GetPathRoot(path)!), StringComparison.OrdinalIgnoreCase))
            throw new IOException("Choose a release-feed folder, not an entire drive.");
        return path;
    }

    private static bool PortAvailable(int port)
    {
        var listener = new TcpListener(IPAddress.Loopback, port);
        try { listener.Server.ExclusiveAddressUse = true; listener.Start(); return true; }
        catch (SocketException) { return false; }
        finally { listener.Stop(); }
    }

    private static string Quoted(string path)
    {
        if (path.IndexOfAny(['"', '\r', '\n']) >= 0) throw new IOException("The local feed path contains unsupported characters.");
        return '"' + path.Replace('\\', '/') + '"';
    }

    internal static string BuildConfiguration(string runtime, string root, string state, int port) => $$"""
        ServerRoot {{Quoted(runtime)}}
        Listen 127.0.0.1:{{port}}
        ServerName 127.0.0.1:{{port}}
        LoadModule authz_core_module modules/mod_authz_core.so
        LoadModule authz_host_module modules/mod_authz_host.so
        LoadModule alias_module modules/mod_alias.so
        LoadModule mime_module modules/mod_mime.so
        LoadModule dir_module modules/mod_dir.so
        TypesConfig conf/mime.types
        ServerTokens Prod
        ServerSignature Off
        TraceEnable Off
        Timeout 120
        KeepAlive On
        KeepAliveTimeout 2
        ThreadsPerChild 32
        MaxConnectionsPerChild 0
        EnableSendfile Off
        EnableMMAP Off
        ErrorLog {{Quoted(Path.Combine(state, "error.log"))}}
        LogLevel warn
        PidFile {{Quoted(Path.Combine(state, "httpd.pid"))}}
        DocumentRoot {{Quoted(root)}}
        <Directory />
          AllowOverride None
          Require all denied
        </Directory>
        <Directory {{Quoted(root)}}>
          Options None
          AllowOverride None
          Require local
        </Directory>
        Alias /__ssw_launcher_health {{Quoted(Path.Combine(state, "health.txt"))}}
        <Directory {{Quoted(state)}}>
          Options None
          AllowOverride None
          Require local
        </Directory>
        """;

    private void StopOwned()
    {
        lock (ownership)
        {
            Process? owned = child; child = null;
            try { if (owned is { HasExited: false }) owned.Kill(entireProcessTree: true); }
            catch (Exception ex) when (ex is InvalidOperationException or System.ComponentModel.Win32Exception) { }
            finally { job?.Dispose(); job = null; owned?.Dispose(); }
        }
    }
    public void Dispose() { lock (ownership) { disposed = true; StopOwned(); } }

    private static SafeFileHandle CreateLifetimeJob()
    {
        var handle = CreateJobObjectW(IntPtr.Zero, null);
        if (handle.IsInvalid) { handle.Dispose(); throw new IOException("Could not create the local server lifetime guard."); }
        var limits = new ExtendedLimits { Basic = new BasicLimits { LimitFlags = 0x2000 } }; // KILL_ON_JOB_CLOSE
        if (!SetInformationJobObject(handle, 9, ref limits, (uint)Marshal.SizeOf<ExtendedLimits>()))
        { handle.Dispose(); throw new IOException("Could not configure the local server lifetime guard."); }
        return handle;
    }
    [StructLayout(LayoutKind.Sequential)] private struct BasicLimits
    {
        internal long ProcessTime, JobTime; internal uint LimitFlags;
        internal UIntPtr MinimumWorkingSet, MaximumWorkingSet; internal uint ActiveProcessLimit;
        internal UIntPtr Affinity; internal uint PriorityClass, SchedulingClass;
    }
    [StructLayout(LayoutKind.Sequential)] private struct IoCounters { internal ulong ReadOperations, WriteOperations, OtherOperations, ReadBytes, WriteBytes, OtherBytes; }
    [StructLayout(LayoutKind.Sequential)] private struct ExtendedLimits
    { internal BasicLimits Basic; internal IoCounters Io; internal UIntPtr ProcessMemoryLimit, JobMemoryLimit, PeakProcessMemoryUsed, PeakJobMemoryUsed; }
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)] private static extern SafeFileHandle CreateJobObjectW(IntPtr attributes, string? name);
    [DllImport("kernel32.dll", SetLastError = true)] [return: MarshalAs(UnmanagedType.Bool)] private static extern bool SetInformationJobObject(SafeFileHandle job, int infoClass, ref ExtendedLimits info, uint length);
    [DllImport("kernel32.dll", SetLastError = true)] [return: MarshalAs(UnmanagedType.Bool)] private static extern bool AssignProcessToJobObject(SafeFileHandle job, IntPtr process);
}
