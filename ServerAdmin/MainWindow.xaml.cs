using System.Diagnostics;
using System.IO;
using System.Text.Json;
using System.Windows;
using System.Windows.Media;
using System.Windows.Threading;
using Microsoft.Win32;
using StarshatterWarsLauncher;
namespace FractalVerseAdmin;
public partial class MainWindow : Window
{
 private static readonly JsonSerializerOptions Json=new(JsonSerializerDefaults.Web);
 private readonly DispatcherTimer refresh=new(){Interval=TimeSpan.FromSeconds(2)};
 private string directory=UniverseServerManager.SaveDirectory;
 private UniverseStatus? snapshot;
 public MainWindow()
 {
  InitializeComponent();PlayersGrid.ItemsSource=Array.Empty<UniversePeer>();
  var args=Environment.GetCommandLineArgs();for(int i=0;i+1<args.Length;i++)if(args[i]=="--status-dir")directory=Path.GetFullPath(args[i+1]);
  refresh.Tick+=(_,_)=>RefreshStatus();Loaded+=(_,_)=>{RefreshStatus();refresh.Start();};Closed+=(_,_)=>refresh.Stop();
  RefreshStatus();
 }
 internal void Present(UniverseStatus? state,DateTimeOffset now)
 {
  snapshot=state;if(state is not null)SourceLabel.Text=Path.Combine(directory,"status.json");bool live=state is not null && UniverseServerManager.IsFresh(state,now);
  ConnectionLabel.Text=live?"ONLINE · "+state!.Server:"DISCONNECTED / STALE DATA";
  ConnectionLabel.Foreground=new SolidColorBrush(live?Color.FromRgb(100,215,160):Color.FromRgb(225,169,100));
  ClockLabel.Text=live?state!.UniverseUtc??"Awaiting clock":"—";
  RevisionLabel.Text=live?$"Revision {state!.Revision:N0} · Protocol {state.Protocol}":"No confirmed state";
  ClientsLabel.Text=live?$"{state!.ConnectedClients} / {state.MaximumClients}":"— / 8";
  UptimeLabel.Text=live?TimeSpan.FromSeconds(Math.Max(0,state!.UptimeSeconds)).ToString(@"d\.hh\:mm\:ss"):"—";
  CheckpointLabel.Text=live?(state!.CheckpointReady?"Ready":"Unavailable / simulation suspended"):"Unknown";
  CheckpointTimeLabel.Text=live && state!.LastCheckpointUtc>0?DateTimeOffset.FromUnixTimeSeconds(state.LastCheckpointUtc).ToString("yyyy-MM-dd HH:mm:ss 'GMT'"):"—";
  EconomyLabel.Text=live?$"Inventories: {state!.Inventories:N0}\nFreight journeys: {state.Journeys:N0}\nProduction jobs: {state.Jobs:N0}":"No confirmed economy state";
  PlayersGrid.ItemsSource=live?state!.Players??[]:Array.Empty<UniversePeer>();
  ProcessLabel.Text="No local process telemetry";
  if(live && state!.ProcessId>0)try {using var process=Process.GetProcessById(state.ProcessId);ProcessLabel.Text=$"PID {state.ProcessId} · {process.ProcessName}\nMemory: {process.WorkingSet64/1048576d:N1} MB\nCPU time: {process.TotalProcessorTime:g}";}catch(Exception e)when(e is ArgumentException or InvalidOperationException or System.ComponentModel.Win32Exception){}
 }
 private void RefreshStatus()
 {
  SourceLabel.Text=Path.Combine(directory,"status.json");
  try {var path=Path.Combine(directory,"status.json");if(new FileInfo(path).Length>65536)throw new IOException("Status response exceeds the monitoring limit.");Present(JsonSerializer.Deserialize<UniverseStatus>(File.ReadAllText(path),Json),DateTimeOffset.UtcNow);}
  catch(Exception e)when(e is IOException or UnauthorizedAccessException or JsonException){Present(null,DateTimeOffset.UtcNow);SourceLabel.Text+=" · "+e.Message;}
 }
 private void Refresh_Click(object sender,RoutedEventArgs e)=>RefreshStatus();
 private void SelectFolder_Click(object sender,RoutedEventArgs e){var picker=new OpenFolderDialog{Title="Select FractalVerse universe status folder"};if(picker.ShowDialog(this)==true){directory=picker.FolderName;RefreshStatus();}}
 private async void Log_Click(object sender,RoutedEventArgs e)
 {
  try {
   string path=Path.Combine(directory,"FractalVerse.log");
   using var stream=new FileStream(path,FileMode.Open,FileAccess.Read,FileShare.ReadWrite|FileShare.Delete);stream.Seek(Math.Max(0,stream.Length-256*1024),SeekOrigin.Begin);
   using var reader=new StreamReader(stream);string tail=await reader.ReadToEndAsync();
   var text=new System.Windows.Controls.TextBox{Text=tail,IsReadOnly=true,FontFamily=new FontFamily("Consolas"),FontSize=12,TextWrapping=TextWrapping.NoWrap,VerticalScrollBarVisibility=System.Windows.Controls.ScrollBarVisibility.Auto,HorizontalScrollBarVisibility=System.Windows.Controls.ScrollBarVisibility.Auto,Background=new SolidColorBrush(Color.FromRgb(18,33,49)),Foreground=Brushes.LightGray};
   new Window{Owner=this,Title="FractalVerse — Recent Log (read-only)",Width=1000,Height=650,Content=text,WindowStartupLocation=WindowStartupLocation.CenterOwner}.Show();
  }catch(Exception ex)when(ex is IOException or UnauthorizedAccessException){MessageBox.Show(this,ex.Message,"Server log",MessageBoxButton.OK,MessageBoxImage.Information);}
 }
}
