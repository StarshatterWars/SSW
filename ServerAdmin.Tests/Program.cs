using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.IO;
using FractalVerseAdmin;
using StarshatterWarsLauncher;
internal static class Program
{
 [STAThread]static void Main()
 {
  var window=new MainWindow();int checks=0;void Check(bool ok,string name){if(!ok)throw new Exception(name);Console.WriteLine("PASS "+name);checks++;}
  Check(((TextBlock)window.FindName("ConnectionLabel")).Text.Contains("DISCONNECTED"),"No live server is never presented as online");
  Check(window.Icon is not null,"Admin window uses the SSW icon");
  var now=DateTimeOffset.UtcNow;
  var state=new UniverseStatus(Online:true,Server:"FractalVerse (UI test snapshot)",Revision:123,UniverseUtc:"2228-11-20T12:00:00Z",ServerUtc:now.ToUnixTimeSeconds(),UptimeSeconds:500,
   MaximumClients:8,ConnectedClients:1,Players:[new UniversePeer("TEST PILOT",12)],CheckpointReady:true,LastCheckpointUtc:now.ToUnixTimeSeconds(),Inventories:146,Journeys:4,Jobs:6);
  window.Present(state,now);
  Check(((TextBlock)window.FindName("ClientsLabel")).Text=="1 / 8","Client capacity displayed correctly");
  Check(((TextBlock)window.FindName("CheckpointLabel")).Text=="Ready","Confirmed checkpoint displayed");
  Check(((ListBox)window.FindName("PlayersGrid")).Items.Count==1,"Connected player roster displayed");
  Check(((TextBlock)window.FindName("EconomyLabel")).Text.Contains("146"),"Actual economy counters displayed");
  var page=(FrameworkElement)window.Content;page.Measure(new Size(1050,760));page.Arrange(new Rect(0,0,1050,760));page.UpdateLayout();
  var image=new RenderTargetBitmap(1050,760,96,96,PixelFormats.Pbgra32);image.Render(page);var png=new PngBitmapEncoder();png.Frames.Add(BitmapFrame.Create(image));
  Directory.CreateDirectory("Saved/Review");using(var file=File.Create("Saved/Review/FractalVerseAdmin.png"))png.Save(file);
  window.Present(state,now.AddSeconds(12));
  Check(((TextBlock)window.FindName("ConnectionLabel")).Text.Contains("DISCONNECTED"),"Expired heartbeat stops claiming online");
  Check(((ListBox)window.FindName("PlayersGrid")).Items.Count==0,"Disconnected roster clears stale players");
  window.Close();Console.WriteLine($"{checks} admin checks passed");
 }
}

