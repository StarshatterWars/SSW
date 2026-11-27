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
    private bool pilotRecoveryActive;
    private bool refreshingPatchNotes;
    private readonly CancellationTokenSource closing = new();
    private readonly LocalFeedServer localFeedServer = new();
    private readonly UniverseServerManager universeServer = new();
    private readonly EconomyServerManager economyServer = new();

    public MainWindow()
    {
        InitializeComponent();
        LoadRoadmap();
        ServerPlayers.ItemsSource=Array.Empty<UniversePeer>();
        ReleaseStage.Text=ReleasePresentation.Stage.ToUpperInvariant();
        AlphaWarning.Text=ReleasePresentation.Warning;
        LauncherVersion.Text = $"Launcher {typeof(MainWindow).Assembly.GetName().Version!.ToString(3)}";
        sourceGame = FindGame();
        string installed = Path.Combine(@"C:\SSW", "StarshatterWars.exe");
        if (File.Exists(installed)) game = installed;
        try
        {
            if (File.Exists(InstallRecord))
            {
                string selected = File.ReadAllText(InstallRecord).Trim();
                if (File.Exists(selected) && (game is null ||
                    File.Exists(Path.Combine(Path.GetDirectoryName(selected)!, ".ssw-version"))))
                    game = selected;
            }
        }
        catch (IOException) { }
        InstallButton.IsEnabled = true;
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
            _ = MonitorUniverseAsync();
            _ = MonitorEconomyAsync();
            await PrepareLocalFeedServerAsync();
            if (!closing.IsCancellationRequested) await Task.WhenAll(LoadFeedAsync(), LoadPatchNotesAsync());
        };
        Closed += (_, _) => { closing.Cancel(); localFeedServer.Dispose(); universeServer.Dispose(); economyServer.Dispose(); music.Close(); NewsView.Dispose(); };
        Activated += async (_, _) =>
        {
            if (IsLoaded && !busy && !closing.IsCancellationRequested) await LoadPatchNotesAsync();
        };
        Closing += (_, e) =>
        {
            if (!pilotRecoveryActive) return;
            e.Cancel = true;
            LaunchStatus.Text = "Please wait for pilot recovery to finish before closing the launcher.";
        };
    }

    private async Task MonitorEconomyAsync()
    {
        while(!closing.IsCancellationRequested)
        {
            try{
                var connection=await economyServer.RefreshAsync(game,closing.Token);closing.Token.ThrowIfCancellationRequested();
                EconomyAddress.Text="Address: "+connection.Endpoint;
                var state=connection.Status;
                EconomySummary.Text=connection.Label+"\n"+connection.Detail+(state is null?"":$"\nPing {connection.PingMs:0} ms · uptime {TimeSpan.FromSeconds(state.UptimeSeconds):g} · revision {state.Revision:N0}\nInventories {state.Inventories:N0} · active jobs {state.ActiveJobs:N0} · freight deliveries {state.DeliveredFreight:N0}\nCheckpoint: {(state.CheckpointReady?"READY":"BLOCKED")}");
            }catch(OperationCanceledException)when(closing.IsCancellationRequested){break;}
            catch(ObjectDisposedException)when(closing.IsCancellationRequested){break;}
            catch(Exception ex)when(ex is IOException or UnauthorizedAccessException or JsonException or InvalidOperationException or KeyNotFoundException or System.ComponentModel.Win32Exception)
            {EconomySummary.Text="ECONOMY · DISCONNECTED\n"+ex.Message;}
            try{await Task.Delay(2000,closing.Token);}catch(OperationCanceledException){break;}
        }
    }

    private async Task MonitorUniverseAsync()
    {
        while (!closing.IsCancellationRequested)
        {
            try
            {
                var config=JsonSerializer.Deserialize<LauncherSettings>(await File.ReadAllTextAsync(Path.Combine(AppContext.BaseDirectory,"launcher.json"),closing.Token),JsonOptions);
                var connection=await universeServer.RefreshAsync(config?.UniverseServer??new UniverseServerSettings(),game,closing.Token);
                UniverseIndicator.Text="UNIVERSE · "+connection.Label;
                UniverseIndicator.Foreground=new System.Windows.Media.SolidColorBrush(connection.Status?.Online==true?System.Windows.Media.Color.FromRgb(100,215,160):System.Windows.Media.Color.FromRgb(172,196,213));
                var state=connection.Status;
                ServerSummary.Text=connection.Detail+(state is null?"":$"\n{state.Server} · {state.ConnectedClients}/{state.MaximumClients} clients · revision {state.Revision:N0}\nUniverse GMT: {state.UniverseUtc} · uptime {TimeSpan.FromSeconds(Math.Max(0,state.UptimeSeconds)):g}");
                ServerPlayers.ItemsSource=state?.Players??[];
            }
            catch(OperationCanceledException) when(closing.IsCancellationRequested){break;}
            catch(ObjectDisposedException) when(closing.IsCancellationRequested){break;}
            catch(Exception ex) when(ex is IOException or UnauthorizedAccessException or JsonException or InvalidOperationException or System.ComponentModel.Win32Exception)
            {UniverseIndicator.Text="UNIVERSE · DISCONNECTED";ServerSummary.Text=ex.Message;}
            try{await Task.Delay(2000,closing.Token);}catch(OperationCanceledException){break;}
        }
    }

    private Task EnsureLocalFeedServerAsync(LauncherSettings? config, CancellationToken token) =>
        localFeedServer.EnsureReadyAsync(config?.LocalFeedServer,
            [config?.FeedUrl, config?.PatchManifestUrl, config?.GameArchiveUrl], token);

    private async Task PrepareLocalFeedServerAsync()
    {
        try
        {
            var config = JsonSerializer.Deserialize<LauncherSettings>(
                await File.ReadAllTextAsync(Path.Combine(AppContext.BaseDirectory, "launcher.json"), closing.Token), JsonOptions);
            await EnsureLocalFeedServerAsync(config, closing.Token);
        }
        catch (Exception ex) when (ex is IOException or JsonException or UnauthorizedAccessException or ArgumentException
            or OperationCanceledException or InvalidOperationException)
        {
            if (!closing.IsCancellationRequested)
                LaunchStatus.Text = $"Local update feed unavailable. {ex.Message} Play requires a verified current release.";
        }
    }

    private Uri? storeUri;
    private void SelectContentTab(string tab)
    {
        NewsPanel.Visibility=tab=="news"?Visibility.Visible:Visibility.Collapsed;
        StorePanel.Visibility=tab=="store"?Visibility.Visible:Visibility.Collapsed;
        RoadmapPanel.Visibility=tab=="roadmap"?Visibility.Visible:Visibility.Collapsed;
        ServerPanel.Visibility=tab=="server"?Visibility.Visible:Visibility.Collapsed;
        foreach(var button in new[]{NewsTabButton,StoreTabButton,RoadmapTabButton,ServerTabButton})
            button.Background=new System.Windows.Media.SolidColorBrush(System.Windows.Media.Color.FromRgb(23,43,59));
        var active=tab=="server"?ServerTabButton:tab=="roadmap"?RoadmapTabButton:tab=="store"?StoreTabButton:NewsTabButton;
        active.Background=new System.Windows.Media.SolidColorBrush(System.Windows.Media.Color.FromRgb(36,70,90));
    }
    private void NewsTab_Click(object sender, RoutedEventArgs e) => SelectContentTab("news");
    private void ServerTab_Click(object sender, RoutedEventArgs e) => SelectContentTab("server");
    private void RoadmapTab_Click(object sender, RoutedEventArgs e) => SelectContentTab("roadmap");
    private sealed record RoadmapPhase(string Title,string Status,string Description);
    private sealed record RoadmapDocument(string Updated,string Notice,RoadmapPhase[] Phases);
    private void LoadRoadmap()
    {
        try
        {
            var document=JsonSerializer.Deserialize<RoadmapDocument>(File.ReadAllText(Path.Combine(AppContext.BaseDirectory,"roadmap.json")),JsonOptions);
            if(document?.Phases is not {Length:>0})throw new InvalidDataException("Empty roadmap");
            RoadmapNotice.Text=$"Updated {document.Updated} · {document.Notice}";
            foreach(var phase in document.Phases)
            {
                var body=new System.Windows.Controls.StackPanel();
                body.Children.Add(new System.Windows.Controls.TextBlock{Text=phase.Status,FontSize=11,Foreground=System.Windows.Media.Brushes.LightSkyBlue});
                body.Children.Add(new System.Windows.Controls.TextBlock{Text=phase.Title,FontSize=18,FontWeight=FontWeights.SemiBold,TextWrapping=TextWrapping.Wrap,Margin=new Thickness(0,5,0,6)});
                body.Children.Add(new System.Windows.Controls.TextBlock{Text=phase.Description,FontSize=13,TextWrapping=TextWrapping.Wrap});
                RoadmapItems.Children.Add(new System.Windows.Controls.Border{Child=body,Background=new System.Windows.Media.SolidColorBrush(System.Windows.Media.Color.FromRgb(26,48,65)),CornerRadius=new CornerRadius(5),Padding=new Thickness(16),Margin=new Thickness(0,0,0,12)});
            }
        }
        catch(Exception ex) when(ex is IOException or UnauthorizedAccessException or JsonException)
        {RoadmapNotice.Text="The roadmap is unavailable in this installation. Install, update and play remain available.";}
    }
    internal Task StoreRefresh { get; private set; } = Task.CompletedTask;
    private void StoreTab_Click(object sender, RoutedEventArgs e)
    {
        StoreRefresh = RefreshStoreAsync();
    }
    private async Task RefreshStoreAsync()
    {
        SelectContentTab("store");
        try
        {
            var config=JsonSerializer.Deserialize<LauncherSettings>(await File.ReadAllTextAsync(Path.Combine(AppContext.BaseDirectory,"launcher.json"),closing.Token),JsonOptions);
            storeUri=StoreNavigation.Website(config?.StoreUrl);
            StoreVisitButton.IsEnabled=storeUri is not null;
            StoreStatus.Text=storeUri is null
                ? "The store is being prepared. No products are on sale and no payment details are collected by this launcher."
                : "Visit the official store in your browser to review current offers. Checkout and account ownership will be handled by the store provider.";
        }
        catch(Exception ex) when(ex is IOException or UnauthorizedAccessException or JsonException or OperationCanceledException)
        {storeUri=null;StoreVisitButton.IsEnabled=false;StoreStatus.Text="Store details are unavailable. Install, update and play remain available.";}
    }
    private void StoreVisit_Click(object sender, RoutedEventArgs e)
    {
        if(storeUri is null)return;
        try{Process.Start(new ProcessStartInfo(storeUri.AbsoluteUri){UseShellExecute=true});}
        catch(Exception ex) when(ex is System.ComponentModel.Win32Exception or InvalidOperationException)
        {StoreStatus.Text="The browser could not open. Please try again.";}
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
        MusicWaves.Visibility = Visibility.Visible; MusicMuted.Visibility = Visibility.Collapsed;
        MusicButton.ToolTip = "Mute launcher music"; System.Windows.Automation.AutomationProperties.SetName(MusicButton, "Mute launcher music");
        music.Play();
    }

    private void StopMusic()
    {
        musicEnabled = false;
        music.Close();
        MusicWaves.Visibility = Visibility.Collapsed; MusicMuted.Visibility = Visibility.Visible;
        MusicButton.ToolTip = "Enable launcher music"; System.Windows.Automation.AutomationProperties.SetName(MusicButton, "Enable launcher music");
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
        string versionFile = Path.Combine(directory, ".ssw-version");
        if (File.Exists(versionFile))
        {
            string release=File.ReadAllText(versionFile).Trim();
            if(!string.IsNullOrEmpty(release))return ReleasePresentation.Version(release);
        }
        string buildFile=Path.Combine(directory,"StarshatterWars","Content","Build","build-version.json");
        if(File.Exists(buildFile))
        {
            try
            {
                using var record=JsonDocument.Parse(File.ReadAllText(buildFile));
                string? build=record.RootElement.GetProperty("version").GetString();
                if(!string.IsNullOrWhiteSpace(build))return ReleasePresentation.Version(build);
            }
            catch(Exception ex) when(ex is IOException or JsonException or KeyNotFoundException or InvalidOperationException) { }
        }
        return $"{ReleasePresentation.Stage} | Game build not recorded";
    }

    private async void Play_Click(object sender, RoutedEventArgs e) => await SignInAndLaunchAsync("EOS");
    private async void Steam_Click(object sender, RoutedEventArgs e) => await SignInAndLaunchAsync("STEAM");
    private async void PlayOffline_Click(object sender, RoutedEventArgs e) => await SignInAndLaunchAsync("NULL");

    private async Task SignInAndLaunchAsync(string provider)
    {
        if (busy || game is null) return;
        SetBusy(true);
        LaunchStatus.Text = "Checking installed game version…";
        using var timeout = CancellationTokenSource.CreateLinkedTokenSource(closing.Token);
        timeout.CancelAfter(TimeSpan.FromMinutes(3));
        try
        {
            var config = JsonSerializer.Deserialize<LauncherSettings>(
                await File.ReadAllTextAsync(Path.Combine(AppContext.BaseDirectory, "launcher.json"), timeout.Token), JsonOptions);
            await EnsureLocalFeedServerAsync(config, timeout.Token);
            if (!Uri.TryCreate(config?.PatchManifestUrl, UriKind.Absolute, out Uri? manifestUrl) ||
                string.IsNullOrWhiteSpace(config?.PatchPublicKeyPem))
                throw new InvalidOperationException("Playing requires a configured update feed to verify your game version.");

            string versionPath = Path.Combine(Path.GetDirectoryName(game)!, ".ssw-version");
            if (!File.Exists(versionPath))
                throw new InvalidOperationException("Install the game through the launcher before playing.");

            using var http = new HttpClient(new HttpClientHandler { AllowAutoRedirect = false })
            {
                Timeout = TimeSpan.FromSeconds(10),
                MaxResponseContentBufferSize = 16 * 1024 * 1024
            };
            var manifest = await new GamePatcher(http).ReadManifestAsync(manifestUrl, config.PatchPublicKeyPem, timeout.Token);
            string installedVersion = (await File.ReadAllTextAsync(versionPath, timeout.Token)).Trim();
            if (!string.Equals(installedVersion, manifest.Version, StringComparison.Ordinal))
                throw new InvalidOperationException($"The game requires build {manifest.Version}. You have {installedVersion}. Use Check for Updates.");

            if (provider == "NULL")
            {
                await LaunchAsync(provider);
                return;
            }

            LaunchStatus.Text = provider == "EOS" ? "Signing in with Epic…" : "Checking the Steam session…";
            if (provider == "EOS")
            {
                string artifact = await PlatformSignIn.EpicAsync(game, sourceGame ?? game, timeout.Token);
                await LaunchAsync(provider, artifact: artifact);
            }
            else
            {
                uint appId = config.SteamAppId;
                ulong account = PlatformSignIn.Steam(game, appId);
                await LaunchAsync(provider, steamAppId: appId, steamAccount: account);
            }
        }
        catch (OperationCanceledException)
        {
            if (!closing.IsCancellationRequested) LaunchStatus.Text = "The update check or sign-in timed out.";
        }
        catch (Exception ex) when (ex is IOException or JsonException or InvalidOperationException
            or HttpRequestException or InvalidDataException or UnauthorizedAccessException
            or System.Security.Cryptography.CryptographicException or ArgumentException
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
        if(!value){universeServer.ResumeAutoStart();economyServer.ResumeAutoStart();}
        RepairButton.IsEnabled = !value && game is not null;
        InstallButton.IsEnabled = !value;
        UpdateButton.IsEnabled = !value;
        PlayButton.IsEnabled = SteamButton.IsEnabled = OfflineButton.IsEnabled = !value && game is not null;
    }

    private void Repair_Click(object sender, RoutedEventArgs e)
    {
        RepairButton.ContextMenu.PlacementTarget = RepairButton;
        RepairButton.ContextMenu.IsOpen = true;
    }

    private async void RecoverPilot_Click(object sender, RoutedEventArgs e)
    {
        if (busy || game is null) return;
        if (GameIsRunning()) { LaunchStatus.Text = "Close Starshatter Wars before recovering a pilot save."; return; }
        SetBusy(true);
        pilotRecoveryActive = true;
        try
        {
            string root = Path.GetDirectoryName(game)!;
            using var installLock = new FileStream(Path.Combine(root, LocalInstaller.LockName), FileMode.OpenOrCreate, FileAccess.ReadWrite, FileShare.None);
            LaunchStatus.Text = "Checking pilot recovery checkpoints…";
            var available = await PilotRecovery.RunAsync(game, null, closing.Token);
            if (available.Checkpoints is not { Length: > 0 })
            { LaunchStatus.Text = "No complete, valid pilot recovery checkpoints were found."; return; }
            var dialog = new PilotRecoveryWindow(available.Checkpoints) { Owner = this };
            if (dialog.ShowDialog() != true || dialog.Selected is null) { LaunchStatus.Text = "Recovery cancelled. Saves were not changed."; return; }
            if (GameIsRunning()) throw new IOException("Close Starshatter Wars before restoring the checkpoint.");
            LaunchStatus.Text = "Backing up saves, verifying the checkpoint and restoring the pilot…";
            var restored = await PilotRecovery.RunAsync(game, dialog.Selected, closing.Token);
            LaunchStatus.Text = $"Pilot restored. Backup: {restored.Backup}";
            MessageBox.Show(this, $"Pilot save restored successfully.\n\nYour previous saves are backed up here:\n{restored.Backup}", "Recover Pilot Save", MessageBoxButton.OK, MessageBoxImage.Information);
        }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException or JsonException or OperationCanceledException or System.ComponentModel.Win32Exception)
        { LaunchStatus.Text = $"Recovery unavailable. {ex.Message}"; }
        finally { pilotRecoveryActive = false; SetBusy(false); }
    }

    private async void LocateGame_Click(object sender, RoutedEventArgs e)
    {
        if (busy) return;
        var picker = new Microsoft.Win32.OpenFileDialog
        {
            Title = "Locate the Starshatter Wars game executable",
            Filter = "Starshatter Wars|StarshatterWars.exe",
            InitialDirectory = game is not null ? Path.GetDirectoryName(game) :
                Directory.Exists(@"C:\SSW") ? @"C:\SSW" : Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments),
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
        if (busy) return;
        var folder = new System.Windows.Controls.TextBox { Text = @"C:\SSW", Margin = new Thickness(0, 0, 0, 12) };
        var installButton = new System.Windows.Controls.Button { Content = "Install", IsDefault = true, MinWidth = 84 };
        var cancelButton = new System.Windows.Controls.Button { Content = "Cancel", IsCancel = true, MinWidth = 84 };
        var buttons = new System.Windows.Controls.StackPanel
        {
            Orientation = System.Windows.Controls.Orientation.Horizontal,
            HorizontalAlignment = HorizontalAlignment.Right
        };
        buttons.Children.Add(cancelButton);
        buttons.Children.Add(installButton);
        var content = new System.Windows.Controls.StackPanel { Margin = new Thickness(18) };
        content.Children.Add(new System.Windows.Controls.TextBlock { Text = "Game installation folder", Margin = new Thickness(0, 0, 0, 6) });
        content.Children.Add(folder);
        content.Children.Add(buttons);
        var picker = new Window
        {
            Owner = this, Title = "Install Starshatter Wars", Content = content, Width = 460,
            SizeToContent = SizeToContent.Height, ResizeMode = ResizeMode.NoResize,
            WindowStartupLocation = WindowStartupLocation.CenterOwner, ShowInTaskbar = false
        };
        installButton.Click += (_, _) => picker.DialogResult = true;
        if (picker.ShowDialog() != true) return;
        string destination = folder.Text.Trim();

        SetBusy(true);
        try
        {
            if (!Path.IsPathFullyQualified(destination)) throw new IOException("Enter a full installation path.");
            string install = Path.TrimEndingDirectorySeparator(Path.GetFullPath(destination));
            if (install.Equals(Path.GetPathRoot(install), StringComparison.OrdinalIgnoreCase))
                throw new IOException("Choose a game folder inside the drive.");
            Directory.CreateDirectory(install);
            if ((File.GetAttributes(install) & FileAttributes.ReparsePoint) != 0)
                throw new IOException("Choose a folder that is not a junction or symbolic link.");
            bool overwrite = LocalInstaller.HasExistingFiles(install);
            if (overwrite && MessageBox.Show(this,
                $"Overwrite the installation in {install}? Existing files will be backed up in that folder.",
                "Overwrite Starshatter Wars", MessageBoxButton.YesNo, MessageBoxImage.Warning,
                MessageBoxResult.No) != MessageBoxResult.Yes) return;
            if (overwrite && GameIsRunning()) throw new IOException("Close Starshatter Wars before overwriting.");
            if(overwrite){await economyServer.StopForUpdateAsync(game,closing.Token);await universeServer.StopForUpdateAsync(closing.Token);}
            using var installLock = new FileStream(Path.Combine(install, LocalInstaller.LockName),
                FileMode.OpenOrCreate, FileAccess.ReadWrite, FileShare.None, 1, FileOptions.DeleteOnClose);
            var progress = new Progress<string>(message => LaunchStatus.Text = message);
            string settings = await File.ReadAllTextAsync(Path.Combine(AppContext.BaseDirectory, "launcher.json"), closing.Token);
            var config = JsonSerializer.Deserialize<LauncherSettings>(settings, JsonOptions);
            await EnsureLocalFeedServerAsync(config, closing.Token);
            if (Uri.TryCreate(config?.PatchManifestUrl, UriKind.Absolute, out Uri? url) &&
                !string.IsNullOrWhiteSpace(config?.PatchPublicKeyPem))
            {
                using var http = new HttpClient(new HttpClientHandler { AllowAutoRedirect = false });
                Uri? archive = string.IsNullOrWhiteSpace(config.GameArchiveUrl) ? null : new Uri(config.GameArchiveUrl, UriKind.Absolute);
                var transfer = new Progress<GamePatcher.TransferProgress>(state =>
                {
                    TransferBar.Visibility = Visibility.Visible;
                    TransferBar.IsIndeterminate = state.Total <= 0;
                    TransferBar.Value = state.Total > 0 ? 100.0 * state.Bytes / state.Total : 0;
                    LaunchStatus.Text = state.Stage == "Downloading SSW"
                        ? $"{state.Stage} · {TransferBar.Value:0.0}% · {state.Bytes / 1073741824.0:0.00}/{state.Total / 1073741824.0:0.00} GiB"
                        : state.Stage;
                });
                var notes = CreateReleaseNotesProgress();
                await Task.Run(async () =>
                {
                    return await new GamePatcher(http).UpdateAsync(install, url, config.PatchPublicKeyPem,
                        progress, GameIsRunning, closing.Token, transfer, freshInstall: true, overwrite: overwrite,
                        archiveUrl: archive, releaseNotes: notes);
                }, closing.Token);
                game = Path.Combine(install, "StarshatterWars.exe");
            }
            else if (sourceGame is not null)
                game = await Task.Run(() => LocalInstaller.Install(Path.GetDirectoryName(sourceGame)!,
                    install, progress, closing.Token, overwrite), closing.Token);
            else
                throw new IOException("Configure a release feed or provide a local Windows build.");
            Directory.CreateDirectory(Path.GetDirectoryName(InstallRecord)!);
            await File.WriteAllTextAsync(InstallRecord, game, closing.Token);
            InstalledVersion.Text = DescribeBuild(game);
            LaunchStatus.Text = $"Installed to {Path.GetDirectoryName(game)}";
            await LoadPatchNotesAsync();
        }
        catch (OperationCanceledException) { }
        catch (Exception ex) when (ex is IOException or InvalidDataException or UnauthorizedAccessException or ArgumentException
            or HttpRequestException or JsonException or System.Security.Cryptography.CryptographicException)
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
            if (!File.Exists(Path.Combine(install, ".ssw-version")))
            {
                LaunchStatus.Text = "Use Install or Locate Game to select a launcher-managed installation before updating.";
                return;
            }
            string settings = await File.ReadAllTextAsync(Path.Combine(AppContext.BaseDirectory, "launcher.json"), closing.Token);
            var config = JsonSerializer.Deserialize<LauncherSettings>(settings, JsonOptions);
            await EnsureLocalFeedServerAsync(config, closing.Token);
            if (!Uri.TryCreate(config?.PatchManifestUrl, UriKind.Absolute, out Uri? url) ||
                string.IsNullOrWhiteSpace(config?.PatchPublicKeyPem))
            {
                LaunchStatus.Text = "Set the manifest URL and release public key in launcher.json.";
                return;
            }
            if (GameIsRunning()) { LaunchStatus.Text = "Close Starshatter Wars before updating."; return; }
            await economyServer.StopForUpdateAsync(game,closing.Token);
            await universeServer.StopForUpdateAsync(closing.Token);

            using var http = new HttpClient(new HttpClientHandler { AllowAutoRedirect = false })
            {
                MaxResponseContentBufferSize = 16 * 1024 * 1024,
                Timeout = TimeSpan.FromMinutes(2)
            };
            using var timeout = CancellationTokenSource.CreateLinkedTokenSource(closing.Token);
            timeout.CancelAfter(TimeSpan.FromHours(2));
            var progress = new Progress<string>(message => LaunchStatus.Text = message);
            var notes = CreateReleaseNotesProgress();
            string result = await Task.Run(async () =>
            {
                using var updateLock = new FileStream(Path.Combine(install, LocalInstaller.LockName),
                    FileMode.OpenOrCreate, FileAccess.ReadWrite, FileShare.None, 1, FileOptions.DeleteOnClose);
                return await new GamePatcher(http).UpdateAsync(install, url, config.PatchPublicKeyPem,
                    progress, GameIsRunning, timeout.Token, releaseNotes: notes);
            }, timeout.Token);
            LaunchStatus.Text = result;
            InstalledVersion.Text = DescribeBuild(game);
            await LoadPatchNotesAsync();
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

    private async Task LoadPatchNotesAsync()
    {
        if (refreshingPatchNotes || closing.IsCancellationRequested) return;
        refreshingPatchNotes = true;
        try
        {
            string settings = await File.ReadAllTextAsync(Path.Combine(AppContext.BaseDirectory, "launcher.json"), closing.Token);
            var config = JsonSerializer.Deserialize<LauncherSettings>(settings, JsonOptions);
            await EnsureLocalFeedServerAsync(config, closing.Token);
            if (!Uri.TryCreate(config?.PatchManifestUrl, UriKind.Absolute, out Uri? url) ||
                string.IsNullOrWhiteSpace(config?.PatchPublicKeyPem))
            {
                AvailableVersion.Text = "Update feed is not configured.";
                return;
            }
            using var http = new HttpClient(new HttpClientHandler { AllowAutoRedirect = false })
            {
                Timeout = TimeSpan.FromSeconds(10),
                MaxResponseContentBufferSize = 16 * 1024 * 1024
            };
            var manifest = await new GamePatcher(http).ReadManifestAsync(url, config.PatchPublicKeyPem, closing.Token);
            AvailableVersion.Text = $"Latest build {manifest.Version}";
            if (!string.IsNullOrWhiteSpace(manifest.About)) ShowPatchNotes(manifest.About);
        }
        catch (Exception ex) when (ex is IOException or InvalidDataException or HttpRequestException or JsonException
            or System.Security.Cryptography.CryptographicException or ArgumentException or UnauthorizedAccessException or OperationCanceledException)
        {
            // The release feed is optional when starting the launcher offline.
            if (!closing.IsCancellationRequested)
                AvailableVersion.Text = "Latest build unavailable · check the localhost connection.";
        }
        finally { refreshingPatchNotes = false; }
    }

    private IProgress<string> CreateReleaseNotesProgress() => new Progress<string>(ShowPatchNotes);

    private void ShowPatchNotes(string message)
    {
        PatchNotes.Text = message;
        AboutPatch.Visibility = Visibility.Visible;
    }

    private static bool GameIsRunning() => GameIsRunning(true);

    private static bool GameIsRunning(bool includeServer)
    {
        Process[] games = includeServer
            ? Process.GetProcessesByName("StarshatterWars").Concat(Process.GetProcessesByName("StarServer")).ToArray()
            : Process.GetProcessesByName("StarshatterWars");
        foreach (Process process in games) process.Dispose();
        return games.Length > 0;
    }

    private async Task LaunchAsync(string provider, string? artifact = null, uint steamAppId = 0, ulong steamAccount = 0)
    {
        if (game is null) return;
        string directory = Path.GetDirectoryName(game)!;
        string executable = game;
        if (provider == "EOS")
        {
            var config = JsonSerializer.Deserialize<LauncherSettings>(
                File.ReadAllText(Path.Combine(AppContext.BaseDirectory, "launcher.json")), JsonOptions);
            if (config?.UseEasyAntiCheatBootstrapper == true)
            {
                executable = Path.Combine(directory, "start_protected_game.exe");
                if (!File.Exists(executable) || !File.Exists(Path.Combine(directory, "EasyAntiCheat", "Settings.json")))
                {
                    LaunchStatus.Text = "Easy Anti-Cheat files are missing. Install the protected game build.";
                    return;
                }
            }
        }
        var start = new ProcessStartInfo(executable) { WorkingDirectory = directory };
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
            if (process is null) { LaunchStatus.Text = "Could not start the game."; return; }
            LaunchStatus.Text = "Starting game…";
            await Task.WhenAny(process.WaitForExitAsync(closing.Token), Task.Delay(TimeSpan.FromSeconds(6), closing.Token));
            closing.Token.ThrowIfCancellationRequested();
            bool running = GameIsRunning(includeServer: false);
            int? exitCode = process.HasExited ? process.ExitCode : null;
            LaunchStatus.Text = GameLaunchDiagnostics.Describe(provider, exitCode, running);
            if (running) StopMusic();
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
            var config = JsonSerializer.Deserialize<LauncherSettings>(settings, JsonOptions);
            await EnsureLocalFeedServerAsync(config, closing.Token);
            string? feedUrl = config?.FeedUrl;
            if (!string.IsNullOrWhiteSpace(feedUrl))
            {
                if (!Uri.TryCreate(feedUrl, UriKind.Absolute, out Uri? uri) || !GamePatcher.IsDownloadUrl(uri))
                    throw new InvalidDataException("Use HTTPS or a localhost HTTP address for the news feed.");

                using var http = new HttpClient(new HttpClientHandler { AllowAutoRedirect = false })
                    { MaxResponseContentBufferSize = MaxFeedBytes, Timeout = TimeSpan.FromSeconds(6) };
                feed = JsonSerializer.Deserialize<Feed>(await http.GetByteArrayAsync(uri, closing.Token), JsonOptions);
                if (feed?.Entries is not { Length: > 0 }) throw new InvalidDataException("The feed contains no entries.");
                source = "Web feed";
            }
        }
        catch (Exception ex) when (ex is IOException or InvalidDataException or HttpRequestException or JsonException
            or ArgumentException or UnauthorizedAccessException or OperationCanceledException)
        {
            if (closing.IsCancellationRequested) return;
            source = "Web feed unavailable · showing bundled feed";
        }

        try
        {
            feed ??= JsonSerializer.Deserialize<Feed>(
                await File.ReadAllTextAsync(Path.Combine(AppContext.BaseDirectory, "feed.json"), closing.Token), JsonOptions);
            // Keep readable details underneath the optional browser. News must
            // never block installation or leave an empty panel if WebView stalls.
            FeedStatus.Text = string.Join("\n\n", (feed?.Entries ?? []).OfType<FeedEntry>().Take(20)
                .Select(entry => $"{entry.Title}\n{entry.Date} · {entry.Category}\n{entry.Body}"));
            string html = RenderFeed(feed?.Entries ?? [], source);
            var environment = await CoreWebView2Environment.CreateAsync(userDataFolder:
                Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "StarshatterWars", "LauncherWebView"))
                .WaitAsync(TimeSpan.FromSeconds(10), closing.Token);
            if (closing.IsCancellationRequested) return;
            NewsView.Visibility = Visibility.Visible;
            await NewsView.EnsureCoreWebView2Async(environment).WaitAsync(TimeSpan.FromSeconds(10), closing.Token);
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
                    // The native text beneath the browser remains available.
                }
            };
            core.NewWindowRequested += (_, args) => args.Handled = true;
            NewsView.NavigateToString(html);
            NewsView.Visibility = Visibility.Visible;
        }
        catch (WebView2RuntimeNotFoundException)
        {
            NewsView.Visibility = Visibility.Hidden;
            if (string.IsNullOrWhiteSpace(FeedStatus.Text))
                FeedStatus.Text = "News requires the Microsoft Edge WebView2 Runtime. You can still launch the game.";
        }
        catch (Exception ex) when (ex is IOException or InvalidDataException or JsonException or InvalidOperationException
            or ArgumentException or UnauthorizedAccessException or TimeoutException
            or System.Runtime.InteropServices.COMException or OperationCanceledException)
        {
            if (!closing.IsCancellationRequested)
            {
                NewsView.Visibility = Visibility.Hidden;
                if (string.IsNullOrWhiteSpace(FeedStatus.Text) || FeedStatus.Text.StartsWith("Loading news", StringComparison.Ordinal))
                    FeedStatus.Text = "News and patch notes are unavailable. Install, update and play remain available.";
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

    private sealed record LauncherSettings(string? FeedUrl, uint SteamAppId, string? PatchManifestUrl, string? PatchPublicKeyPem, string? GameArchiveUrl, bool UseEasyAntiCheatBootstrapper, LocalFeedServerSettings? LocalFeedServer = null, string? StoreUrl = null, UniverseServerSettings? UniverseServer = null);
    private sealed record Feed(FeedEntry[]? Entries);
    private sealed record FeedEntry(string? Category, string? Title, string? Date, string? Body);
}
