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
        using(var release=JsonDocument.Parse(File.ReadAllText(Path.Combine(AppContext.BaseDirectory,"release-info.json"))))
        {
            Check(ReleasePresentation.Stage=="Alpha 4","Launcher shows Alpha 4");
            Check(ReleasePresentation.Warning==release.RootElement.GetProperty("warning").GetString(),"Launcher warning matches canonical game warning");
            Check(ReleasePresentation.Version("20261120-testZ")=="Alpha 4 | Build 20261120-testZ","Launcher game version uses the same format as the main menu");
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
        var page=(FrameworkElement)launcher.Content;page.Measure(new Size(1100,740));page.Arrange(new Rect(0,0,1100,740));page.UpdateLayout();
        var preview=new RenderTargetBitmap(1100,740,96,96,PixelFormats.Pbgra32);preview.Render(page);
        var png=new PngBitmapEncoder();png.Frames.Add(BitmapFrame.Create(preview));
        using(var output=File.Create(Path.GetFullPath("Saved/Review/LauncherStore.png")))png.Save(output);
        ((System.Windows.Controls.Button)launcher.FindName("NewsTabButton")).RaiseEvent(new RoutedEventArgs(System.Windows.Controls.Button.ClickEvent));
        Check(((FrameworkElement)launcher.FindName("NewsPanel")).Visibility==Visibility.Visible,"News tab can be restored");
        launcher.Close();
        Console.WriteLine($"{checks} launcher checks passed. Preview: {screenshot}");
    }
}
