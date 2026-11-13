using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Net;
using System.Net.Http;
using System.Text;
using System.Text.Json;
using System.Windows;
using Microsoft.Web.WebView2.Core;

namespace StarshatterWarsLauncher;

public partial class MainWindow : Window
{
    private const int MaxFeedBytes = 256 * 1024;
    private static readonly JsonSerializerOptions JsonOptions = new(JsonSerializerDefaults.Web);
    private readonly System.Windows.Media.MediaPlayer music = new() { Volume = 0.20 };
    private bool musicEnabled;
    private readonly string? sourceGame;
    private string? game;
    private static readonly string InstallRecord = Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "StarshatterWars", "LauncherInstall.txt");
    private bool busy;
    private readonly CancellationTokenSource closing = new();

    public MainWindow()
    {
        InitializeComponent();
        sourceGame = FindGame();
        game = sourceGame;
        try
        {
            if (File.Exists(InstallRecord))
            {
                string selected = File.ReadAllText(InstallRecord).Trim();
                if (File.Exists(selected)) game = selected;
            }
        }
        catch (IOException) { }
        InstallButton.IsEnabled = sourceGame is not null;
        PlayButton.IsEnabled = SteamButton.IsEnabled = OfflineButton.IsEnabled = game is not null;
        InstalledVersion.Text = game is null ? "Game not installed" : DescribeBuild(game);
        LaunchStatus.Text = game is null ? "Use Locate Game to select your Starshatter Wars installation." : "Ready to launch";
        music.MediaEnded += (_, _) =>
        {
            if (!musicEnabled) return;
            music.Position = TimeSpan.Zero;
            music.Play();
        };
        music.MediaFailed += (_, _) =>
        {
            StopMusic();
            MusicButton.IsEnabled = false;
        };
        Loaded += async (_, _) =>
        {
            StartMusic();
            await LoadFeedAsync();
        };
        Closed += (_, _) => { closing.Cancel(); music.Close(); NewsView.Dispose(); };
    }

    private void Music_Click(object sender, RoutedEventArgs e)
    {
        if (musicEnabled) StopMusic();
        else StartMusic();
    }

    private void StartMusic()
    {
        string track = Path.Combine(AppContext.BaseDirectory, "MenuMusic.wav");
        if (!File.Exists(track)) { MusicButton.IsEnabled = false; return; }
        music.Open(new Uri(track));
        musicEnabled = true;
        MusicButton.Content = "Music On";
        music.Play();
    }

    private void StopMusic()
    {
        musicEnabled = false;
        music.Close();
        MusicButton.Content = "Music Off";
    }

    private static string? FindGame()
    {
        for (DirectoryInfo? folder = new(AppContext.BaseDirectory); folder is not null; folder = folder.Parent)
        {
            string candidate = Path.Combine(folder.FullName, "Build", "Windows", "StarshatterWars.exe");
            if (File.Exists(candidate)) return candidate;
        }
        return null;
    }

    private static string DescribeBuild(string game)
    {
        string directory = Path.GetDirectoryName(game)!;
        string binary = Path.Combine(directory, "StarshatterWars", "Binaries", "Win64", "StarshatterWars.exe");
        if (!File.Exists(binary)) binary = game;
        string version = FileVersionInfo.GetVersionInfo(binary).ProductVersion ?? "unknown";
        DateTime buildTime = File.GetLastWriteTimeUtc(binary);
        string versionFile = Path.Combine(directory, ".ssw-version");
        if (File.Exists(versionFile))
            return $"Installed {File.ReadAllText(versionFile).Trim()}  ·  UE {version}";
        return $"Installed build {buildTime:yyyy-MM-dd HH:mm} UTC  ·  UE {version}";
    }

    private async void Play_Click(object sender, RoutedEventArgs e) => await SignInAndLaunchAsync("EOS");
    private async void Steam_Click(object sender, RoutedEventArgs e) => await SignInAndLaunchAsync("STEAM");
    private void PlayOffline_Click(object sender, RoutedEventArgs e) => Launch("NULL");

    private async Task SignInAndLaunchAsync(string provider)
    {
        if (game is null) return;
        SetBusy(true);
        LaunchStatus.Text = provider == "EOS" ? "Signing in with Epic…" : "Checking the Steam session…";
        using var timeout = CancellationTokenSource.CreateLinkedTokenSource(closing.Token);
        timeout.CancelAfter(TimeSpan.FromMinutes(3));
        try
        {
            if (provider == "EOS")
            {
                string artifact = await PlatformSignIn.EpicAsync(game, sourceGame ?? game, timeout.Token);
                Launch(provider, artifact: artifact);
            }
            else
            {
                string settings = await File.ReadAllTextAsync(Path.Combine(AppContext.BaseDirectory, "launcher.json"), timeout.Token);
                uint appId = JsonSerializer.Deserialize<LauncherSettings>(settings, JsonOptions)?.SteamAppId ?? 0;
                ulong account = PlatformSignIn.Steam(game, appId);
                Launch(provider, steamAppId: appId, steamAccount: account);
            }
        }
        catch (OperationCanceledException)
        {
            if (!closing.IsCancellationRequested) LaunchStatus.Text = "Sign-in timed out.";
        }
        catch (Exception ex) when (ex is IOException or JsonException or InvalidOperationException
            or DllNotFoundException or EntryPointNotFoundException or BadImageFormatException
            or System.ComponentModel.Win32Exception)
        {
            LaunchStatus.Text = ex.Message;
        }
        finally
        {
            SetBusy(false);
        }
    }

    private void SetBusy(bool value)
    {
        busy = value;
        InstallButton.IsEnabled = !value && sourceGame is not null;
        UpdateButton.IsEnabled = !value;
        PlayButton.IsEnabled = SteamButton.IsEnabled = OfflineButton.IsEnabled = !value && game is not null;
    }

    private async void LocateGame_Click(object sender, RoutedEventArgs e)
    {
        if (busy) return;
        var picker = new Microsoft.Win32.OpenFileDialog
        {
            Title = "Locate the Starshatter Wars game executable",
            Filter = "Starshatter Wars|StarshatterWars.exe",
            CheckFileExists = true
        };
        if (picker.ShowDialog(this) != true) return;
        string directory = Path.GetDirectoryName(picker.FileName)!;
        if (!File.Exists(Path.Combine(directory, "StarshatterWars", "Binaries", "Win64", "StarshatterWars.exe")))
        {
            LaunchStatus.Text = "Select StarshatterWars.exe at the top level of the game installation.";
            return;
        }
        SetBusy(true);
        try
        {
            Directory.CreateDirectory(Path.GetDirectoryName(InstallRecord)!);
            await File.WriteAllTextAsync(InstallRecord, picker.FileName, closing.Token);
            game = picker.FileName;
            InstalledVersion.Text = DescribeBuild(game);
            LaunchStatus.Text = "Ready to launch";
        }
        catch (OperationCanceledException) { }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException)
        {
            LaunchStatus.Text = $"Could not save the game location. {ex.Message}";
        }
        finally { SetBusy(false); }
    }

    private async void Install_Click(object sender, RoutedEventArgs e)
    {
        if (busy || sourceGame is null) return;
        var picker = new Microsoft.Win32.OpenFolderDialog { Title = "Choose an empty folder for Starshatter Wars" };
        if (picker.ShowDialog(this) != true) return;

        SetBusy(true);
        try
        {
            var progress = new Progress<string>(message => LaunchStatus.Text = message);
            game = await Task.Run(() => LocalInstaller.Install(Path.GetDirectoryName(sourceGame)!,
                picker.FolderName, progress, closing.Token), closing.Token);
            Directory.CreateDirectory(Path.GetDirectoryName(InstallRecord)!);
            await File.WriteAllTextAsync(InstallRecord, game, closing.Token);
            InstalledVersion.Text = DescribeBuild(game);
            LaunchStatus.Text = $"Installed to {Path.GetDirectoryName(game)}";
        }
        catch (OperationCanceledException) { }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException or ArgumentException)
        {
            LaunchStatus.Text = $"Install failed. {ex.Message}";
        }
        finally { SetBusy(false); }
    }

    private async void Update_Click(object sender, RoutedEventArgs e)
    {
        if (busy) return;
        if (game is null) { LaunchStatus.Text = "Install the Windows build first."; return; }
        SetBusy(true);
        try
        {
            string install = Path.GetDirectoryName(game)!;
            string settings = await File.ReadAllTextAsync(Path.Combine(AppContext.BaseDirectory, "launcher.json"), closing.Token);
            var config = JsonSerializer.Deserialize<LauncherSettings>(settings, JsonOptions);
            if (!Uri.TryCreate(config?.PatchManifestUrl, UriKind.Absolute, out Uri? url) ||
                string.IsNullOrWhiteSpace(config?.PatchPublicKeyPem))
            {
                LaunchStatus.Text = "Set the manifest URL and release public key in launcher.json.";
                return;
            }
            if (GameIsRunning()) { LaunchStatus.Text = "Close Starshatter Wars before updating."; return; }

            using var http = new HttpClient(new HttpClientHandler { AllowAutoRedirect = false })
            {
                MaxResponseContentBufferSize = 16 * 1024 * 1024,
                Timeout = TimeSpan.FromMinutes(2)
            };
            using var timeout = CancellationTokenSource.CreateLinkedTokenSource(closing.Token);
            timeout.CancelAfter(TimeSpan.FromHours(2));
            var progress = new Progress<string>(message => LaunchStatus.Text = message);
            string result = await Task.Run(async () =>
            {
                using var updateLock = new FileStream(install + ".update.lock", FileMode.OpenOrCreate,
                    FileAccess.ReadWrite, FileShare.None);
                return await new GamePatcher(http).UpdateAsync(install, url, config.PatchPublicKeyPem,
                    progress, GameIsRunning, timeout.Token);
            }, timeout.Token);
            LaunchStatus.Text = result;
            InstalledVersion.Text = DescribeBuild(game);
        }
        catch (OperationCanceledException)
        {
            if (!closing.IsCancellationRequested) LaunchStatus.Text = "Update timed out.";
        }
        catch (Exception ex) when (ex is IOException or InvalidDataException or HttpRequestException or JsonException
            or System.Security.Cryptography.CryptographicException or ArgumentException or UnauthorizedAccessException)
        {
            LaunchStatus.Text = $"Update failed. {ex.Message}";
        }
        finally { SetBusy(false); }
    }

    private static bool GameIsRunning()
    {
        Process[] games = Process.GetProcessesByName("StarshatterWars");
        foreach (Process process in games) process.Dispose();
        return games.Length > 0;
    }

    private void Launch(string provider, string? artifact = null, uint steamAppId = 0, ulong steamAccount = 0)
    {
        if (game is null) return;
        var start = new ProcessStartInfo(game) { WorkingDirectory = Path.GetDirectoryName(game)! };
        start.ArgumentList.Add($"-ini:Engine:[OnlineSubsystem]:DefaultPlatformService={provider}");
        if (provider == "NULL")
            start.ArgumentList.Add("-ini:Game:[/Script/StarshatterWars.SSWDiagnosticsSettings]:bRequirePlatformSignIn=False");
        else
        {
            start.ArgumentList.Add($"-SSWLauncherProvider={provider}");
            if (artifact is not null) start.ArgumentList.Add($"-EOSArtifactNameOverride={artifact}");
            if (steamAppId != 0)
            {
                start.Environment["SteamAppId"] = steamAppId.ToString(CultureInfo.InvariantCulture);
                start.Environment["SteamGameId"] = steamAppId.ToString(CultureInfo.InvariantCulture);
                start.ArgumentList.Add($"-ini:Engine:[OnlineSubsystemSteam]:SteamDevAppId={steamAppId}");
                start.ArgumentList.Add($"-SSWLauncherSteamAccount={steamAccount}");
            }
        }

        try
        {
            using Process? process = Process.Start(start);
            if (process is not null) StopMusic();
            LaunchStatus.Text = process is null ? "Could not start the game." :
                provider == "NULL" ? "Started in offline mode." : "Signed in. Game started.";
        }
        catch (System.ComponentModel.Win32Exception ex)
        {
            LaunchStatus.Text = $"Could not start the game. {ex.Message}";
        }
    }

    private async Task LoadFeedAsync()
    {
        Feed? feed = null;
        string source = "Bundled feed";
        try
        {
            string settings = await File.ReadAllTextAsync(Path.Combine(AppContext.BaseDirectory, "launcher.json"), closing.Token);
            string? feedUrl = JsonSerializer.Deserialize<LauncherSettings>(settings, JsonOptions)?.FeedUrl;
            if (!string.IsNullOrWhiteSpace(feedUrl))
            {
                if (!Uri.TryCreate(feedUrl, UriKind.Absolute, out Uri? uri) || uri.Scheme != Uri.UriSchemeHttps)
                    throw new InvalidDataException("The feed URL must use HTTPS.");

                using var http = new HttpClient(new HttpClientHandler { AllowAutoRedirect = false })
                    { MaxResponseContentBufferSize = MaxFeedBytes, Timeout = TimeSpan.FromSeconds(6) };
                feed = JsonSerializer.Deserialize<Feed>(await http.GetByteArrayAsync(uri, closing.Token), JsonOptions);
                if (feed?.Entries is not { Length: > 0 }) throw new InvalidDataException("The feed contains no entries.");
                source = "Web feed";
            }
        }
        catch (Exception ex) when (ex is IOException or HttpRequestException or JsonException or OperationCanceledException)
        {
            if (closing.IsCancellationRequested) return;
            source = "Web feed unavailable · showing bundled feed";
        }

        try
        {
            feed ??= JsonSerializer.Deserialize<Feed>(
                await File.ReadAllTextAsync(Path.Combine(AppContext.BaseDirectory, "feed.json"), closing.Token), JsonOptions);
            string html = RenderFeed(feed?.Entries ?? [], source);
            var environment = await CoreWebView2Environment.CreateAsync(userDataFolder:
                Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "StarshatterWars", "LauncherWebView"));
            if (closing.IsCancellationRequested) return;
            NewsView.Visibility = Visibility.Visible;
            await NewsView.EnsureCoreWebView2Async(environment);
            if (closing.IsCancellationRequested) return;
            var core = NewsView.CoreWebView2;
            core.Settings.IsScriptEnabled = false;
            core.Settings.IsWebMessageEnabled = false;
            core.Settings.AreDevToolsEnabled = false;
            core.NavigationCompleted += (_, args) =>
            {
                if (!args.IsSuccess)
                {
                    NewsView.Visibility = Visibility.Hidden;
                    FeedStatus.Text = "News and patch notes could not be displayed.";
                }
            };
            core.NewWindowRequested += (_, args) => args.Handled = true;
            NewsView.NavigateToString(html);
            NewsView.Visibility = Visibility.Visible;
        }
        catch (WebView2RuntimeNotFoundException)
        {
            NewsView.Visibility = Visibility.Hidden;
            FeedStatus.Text = "News requires the Microsoft Edge WebView2 Runtime. You can still launch the game.";
        }
        catch (Exception ex) when (ex is IOException or JsonException or InvalidOperationException
            or System.Runtime.InteropServices.COMException or OperationCanceledException)
        {
            if (!closing.IsCancellationRequested)
            {
                NewsView.Visibility = Visibility.Hidden;
                FeedStatus.Text = "News and patch notes could not be loaded.";
            }
        }
    }

    private static string RenderFeed(IEnumerable<FeedEntry> entries, string source)
    {
        var html = new StringBuilder("""
            <!doctype html><html lang="en"><meta charset="utf-8">
            <meta http-equiv="Content-Security-Policy" content="default-src 'none'; style-src 'unsafe-inline'">
            <style>
            html{color-scheme:dark}body{margin:18px 24px;background:#122131;color:#eaf4ff;font:15px 'Segoe UI',sans-serif}
            article{padding:12px 0 16px;border-bottom:1px solid #2b4257}h2{font-size:19px;margin:6px 0}
            small{color:#93cbe4}p{margin:6px 0;white-space:pre-wrap;overflow-wrap:anywhere}
            </style>
            """);
        html.Append("<small>").Append(WebUtility.HtmlEncode(source)).Append("</small>");
        foreach (FeedEntry entry in entries.OfType<FeedEntry>().Take(20))
        {
            html.Append("<article><small>").Append(WebUtility.HtmlEncode(entry.Category)).Append(" · ")
                .Append(WebUtility.HtmlEncode(entry.Date)).Append("</small><h2>")
                .Append(WebUtility.HtmlEncode(entry.Title)).Append("</h2><p>")
                .Append(WebUtility.HtmlEncode(entry.Body)).Append("</p></article>");
        }
        return html.Append("</html>").ToString();
    }

    private sealed record LauncherSettings(string? FeedUrl, uint SteamAppId, string? PatchManifestUrl, string? PatchPublicKeyPem);
    private sealed record Feed(FeedEntry[]? Entries);
    private sealed record FeedEntry(string? Category, string? Title, string? Date, string? Body);
}
