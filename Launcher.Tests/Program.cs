using System.IO;
using System.Text;
using System.Text.Json;
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using StarshatterWarsLauncher;

internal static class Program
{
    [STAThread]
    private static void Main()
    {
        SynchronizationContext.SetSynchronizationContext(new System.Windows.Threading.DispatcherSynchronizationContext());
        int checks = 0;
        using (var signingKey = System.Security.Cryptography.RSA.Create(2048))
        {
            var handler = new ReleaseMetadataHandler();
            using var client = new System.Net.Http.HttpClient(handler);
            var patcher = new GamePatcher(client);
            foreach (string version in new[] { "20261121-230553Z", "20261122-121633Z" })
            {
                handler.Manifest = Encoding.UTF8.GetBytes(JsonSerializer.Serialize(new
                {
                    version,
                    files = new[] { "StarshatterWars.exe", "StarshatterWars/Binaries/Win64/StarshatterWars.exe" }
                        .Select(path => new { path, size = 1, sha256 = new string('A', 64) }).ToArray()
                }));
                handler.Signature = signingKey.SignData(handler.Manifest, System.Security.Cryptography.HashAlgorithmName.SHA256,
                    System.Security.Cryptography.RSASignaturePadding.Pkcs1);
                var result = patcher.ReadManifestAsync(new Uri("http://127.0.0.1/current/manifest.json"),
                    signingKey.ExportSubjectPublicKeyInfoPem(), CancellationToken.None).GetAwaiter().GetResult();
                if (result.Version != version) throw new Exception("Latest version did not refresh");
            }
            if (handler.Requests != 4 || !handler.AllUncached) throw new Exception("Release metadata may be cached");
            Console.WriteLine("PASS Latest build refreshes from signed uncached manifest and signature");
            checks++;
        }
        void CompleteUi(Task task)
        {
            var watch=System.Diagnostics.Stopwatch.StartNew();
            while(!task.IsCompleted)
            {
                if(watch.Elapsed.TotalSeconds>10)throw new TimeoutException("Launcher UI task did not complete");
                var frame=new System.Windows.Threading.DispatcherFrame();
                System.Windows.Threading.Dispatcher.CurrentDispatcher.BeginInvoke(System.Windows.Threading.DispatcherPriority.Background,new Action(()=>frame.Continue=false));
                System.Windows.Threading.Dispatcher.PushFrame(frame);
                Thread.Sleep(1);
            }
            task.GetAwaiter().GetResult();
        }
        void Check(bool valid, string name) { if (!valid) throw new Exception(name); Console.WriteLine("PASS " + name); checks++; }
        var serverNow=DateTimeOffset.UtcNow;
        var serverState=new UniverseStatus(Online:true,ServerUtc:serverNow.ToUnixTimeSeconds(),MaximumClients:8,ConnectedClients:1,Players:[new UniversePeer("TEST PILOT",10)]);
        Check(UniverseServerManager.IsFresh(serverState,serverNow),"Fresh server heartbeat is online");
        Check(!UniverseServerManager.IsFresh(serverState,serverNow.AddSeconds(11)),"Stale heartbeat is disconnected");
        Check(!UniverseServerManager.IsFresh(serverState with {Protocol=2},serverNow),"Wrong server protocol rejected");
        Check(!UniverseServerManager.IsFresh(serverState with {ConnectedClients=9},serverNow),"Server client cap is eight");
        Check(!UniverseServerManager.IsFresh(serverState with {UptimeSeconds=double.PositiveInfinity},serverNow),"Invalid telemetry cannot claim online");
        Check(!UniverseServerManager.IsFresh(serverState with {Players=null},serverNow),"Connected count needs a matching roster");
        Check(UniverseServerManager.FindExecutable(null) is null,"No arena server is substituted for missing FractalVerse");
        Check(UniverseServerManager.UnrealArgument("-FractalVerseSaveDir=C:/Test Install/World")=="-FractalVerseSaveDir=\"C:/Test Install/World\"","Unreal save paths retain spaces");
        Check(UniverseServerManager.UnrealArgument("-port=17880")=="-port=17880","Numeric Unreal options remain numeric");
        bool rejectedArgument=false;try{UniverseServerManager.UnrealArgument("-port=1\n-server");}catch(ArgumentException){rejectedArgument=true;}
        Check(rejectedArgument,"Injected Unreal options rejected");
        using(var release=JsonDocument.Parse(File.ReadAllText(Path.Combine(AppContext.BaseDirectory,"release-info.json"))))
        {
            Check(ReleasePresentation.Stage=="Alpha 5","Launcher shows Alpha 5");
            Check(ReleasePresentation.Warning==release.RootElement.GetProperty("warning").GetString(),"Launcher warning matches canonical game warning");
            Check(ReleasePresentation.Version("20261120-testZ")=="Alpha 5 | Build 20261120-testZ","Launcher game version uses the same format as the main menu");
        }
        Check(StoreNavigation.Website(null) is null,"Store is inactive until configured");
        Check(StoreNavigation.Website("https://store.example.com/ssw")?.Host=="store.example.com","Configured HTTPS store is accepted");
        Check(StoreNavigation.Website("http://store.example.com") is null,"Plain HTTP checkout is rejected");
        Check(StoreNavigation.Website("https://user:password@store.example.com") is null,"Embedded credentials cannot be passed to a store URL");
        Check(StoreNavigation.Website("file:///C:/SSW") is null,"Local executable and file URLs are not store links");
        string folder = Path.Combine(Path.GetTempPath(), "SSW-RecoveryTests-" + Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(folder);
        string binary = Path.Combine(folder, "fixture.exe");
        File.WriteAllBytes(binary, new byte[200]);
        Check(!PilotRecovery.SupportsRecovery(binary), "Older binaries rejected without running them");
        byte[] marker = Encoding.Unicode.GetBytes("SSWPilotRecovery");
        byte[] data = new byte[1024 * 1024 + 100]; marker.CopyTo(data, 1024 * 1024 + marker.Length - 5);
        File.WriteAllBytes(binary, data);
        Check(PilotRecovery.SupportsRecovery(binary), "Capability marker spanning read blocks detected");
        var report = JsonSerializer.Deserialize<PilotRecovery.Result>("""
        {"success":true,"checkpoints":[{"slot":"State_example_manifest_previous","profile":"PlayerSave_Profile_0123456789abcdef0123456789abcdef","pilot":"Test Pilot","savedUtc":"2026-11-19T18:38:00.000Z","campaign":1,"points":100}],"backup":null,"error":null}
        """, new JsonSerializerOptions(JsonSerializerDefaults.Web))!;
        var cp = report.Checkpoints![0];
        Check(report.Success && cp.Points == 100 && cp.Campaign == 1, "Native report fields deserialize");
        Check(cp.Label.Contains("Test Pilot") && cp.Label.Contains("100 points"), "Checkpoint labels identify pilot and progress");
        Check(cp.SavedUtc.UtcDateTime.ToString("yyyy-MM-ddTHH:mm:ss.fffZ") == "2026-11-19T18:38:00.000Z", "Selected UTC date round trips exactly");
        var window = new PilotRecoveryWindow(report.Checkpoints);
        Check(window.Selected is null, "No checkpoint selected by default");
        var content = (FrameworkElement)window.Content;
        content.Measure(new Size(740, 320)); content.Arrange(new Rect(0, 0, 740, 320)); content.UpdateLayout();
        var image = new RenderTargetBitmap(740, 320, 96, 96, PixelFormats.Pbgra32); image.Render(content);
        var encoder = new PngBitmapEncoder(); encoder.Frames.Add(BitmapFrame.Create(image));
        string screenshot = Path.GetFullPath("Saved/Review/PilotRecoveryDialog.png");
        Directory.CreateDirectory(Path.GetDirectoryName(screenshot)!);
        using (var file = File.Create(screenshot)) encoder.Save(file);
        var launcher=new MainWindow();
        var store=(System.Windows.Controls.Button)launcher.FindName("StoreTabButton");
        store.RaiseEvent(new RoutedEventArgs(System.Windows.Controls.Button.ClickEvent));
        CompleteUi(launcher.StoreRefresh);
        Check(((FrameworkElement)launcher.FindName("StorePanel")).Visibility==Visibility.Visible,"Store tab opens its dedicated panel");
        Check(((FrameworkElement)launcher.FindName("NewsPanel")).Visibility==Visibility.Collapsed,"News remains separate from the store");
        Check(!((System.Windows.Controls.Button)launcher.FindName("StoreVisitButton")).IsEnabled,"Purchases remain inactive until a provider is configured");
        Check(launcher.Icon is not null,"Launcher window uses the SSW icon");
        var page=(FrameworkElement)launcher.Content;page.Measure(new Size(1100,740));page.Arrange(new Rect(0,0,1100,740));page.UpdateLayout();
        var preview=new RenderTargetBitmap(1100,740,96,96,PixelFormats.Pbgra32);preview.Render(page);
        var png=new PngBitmapEncoder();png.Frames.Add(BitmapFrame.Create(preview));
        using(var output=File.Create(Path.GetFullPath("Saved/Review/LauncherStore.png")))png.Save(output);
        ((System.Windows.Controls.Button)launcher.FindName("NewsTabButton")).RaiseEvent(new RoutedEventArgs(System.Windows.Controls.Button.ClickEvent));
        Check(((FrameworkElement)launcher.FindName("NewsPanel")).Visibility==Visibility.Visible,"News tab can be restored");
        ((System.Windows.Controls.Button)launcher.FindName("RoadmapTabButton")).RaiseEvent(new RoutedEventArgs(System.Windows.Controls.Button.ClickEvent));
        Check(((FrameworkElement)launcher.FindName("RoadmapPanel")).Visibility==Visibility.Visible,"Roadmap opens its dedicated panel");
        Check(((FrameworkElement)launcher.FindName("StorePanel")).Visibility==Visibility.Collapsed && ((FrameworkElement)launcher.FindName("NewsPanel")).Visibility==Visibility.Collapsed,"Roadmap does not overlap news or store");
        var cards=(System.Windows.Controls.StackPanel)launcher.FindName("RoadmapItems");
        Check(cards.Children.Count==7,"Bundled roadmap loads offline with all planned phases");
        var roadmapJson=File.ReadAllText(Path.Combine(AppContext.BaseDirectory,"roadmap.json"));
        Check(roadmapJson.Contains("Player-owned stations") && roadmapJson.Contains("Player corporations"),"Ownership and corporations appear in roadmap");
        Check(roadmapJson.Contains("AFTER FULL-SCALE RESIZE"),"Walkable hangars explicitly follow resizing");
        page.Measure(new Size(1000,720));page.Arrange(new Rect(0,0,1000,720));page.UpdateLayout();
        var roadmapImage=new RenderTargetBitmap(1000,720,96,96,PixelFormats.Pbgra32);roadmapImage.Render(page);
        var roadmapPng=new PngBitmapEncoder();roadmapPng.Frames.Add(BitmapFrame.Create(roadmapImage));
        using(var output=File.Create(Path.GetFullPath("Saved/Review/LauncherRoadmap.png")))roadmapPng.Save(output);
        ((System.Windows.Controls.Button)launcher.FindName("NewsTabButton")).RaiseEvent(new RoutedEventArgs(System.Windows.Controls.Button.ClickEvent));
        Check(((FrameworkElement)launcher.FindName("RoadmapPanel")).Visibility==Visibility.Collapsed,"News closes roadmap cleanly");
        ((System.Windows.Controls.Button)launcher.FindName("ServerTabButton")).RaiseEvent(new RoutedEventArgs(System.Windows.Controls.Button.ClickEvent));
        Check(((FrameworkElement)launcher.FindName("ServerPanel")).Visibility==Visibility.Visible,"Universe server has its own status view");
        Check(((FrameworkElement)launcher.FindName("RoadmapPanel")).Visibility==Visibility.Collapsed && ((FrameworkElement)launcher.FindName("NewsPanel")).Visibility==Visibility.Collapsed,"Server view does not overlap other pages");
        Check(((System.Windows.Controls.TextBlock)launcher.FindName("UniverseIndicator")).Text.Contains("DISCONNECTED"),"Launcher never claims online without a heartbeat");
        page.Measure(new Size(1000,720));page.Arrange(new Rect(0,0,1000,720));page.UpdateLayout();
        var serverImage=new RenderTargetBitmap(1000,720,96,96,PixelFormats.Pbgra32);serverImage.Render(page);
        var serverPng=new PngBitmapEncoder();serverPng.Frames.Add(BitmapFrame.Create(serverImage));
        using(var output=File.Create(Path.GetFullPath("Saved/Review/LauncherServer.png")))serverPng.Save(output);
        launcher.Close();
        Console.WriteLine($"{checks} launcher checks passed. Preview: {screenshot}");
    }
}

internal sealed class ReleaseMetadataHandler : System.Net.Http.HttpMessageHandler
{
    internal byte[] Manifest = [];
    internal byte[] Signature = [];
    internal int Requests;
    internal bool AllUncached = true;
    protected override Task<System.Net.Http.HttpResponseMessage> SendAsync(System.Net.Http.HttpRequestMessage request, CancellationToken token)
    {
        Requests++;
        AllUncached &= request.Headers.CacheControl is { NoCache: true, NoStore: true };
        return Task.FromResult(new System.Net.Http.HttpResponseMessage(System.Net.HttpStatusCode.OK)
        {
            Content = new System.Net.Http.ByteArrayContent(request.RequestUri!.AbsolutePath.EndsWith(".sig") ? Signature : Manifest)
        });
    }
}
