using System.IO;
using System.Net;
using System.Text.Json;
using System.Text.RegularExpressions;
namespace StarshatterWarsLauncher;

internal sealed record ServiceEndpoints(string EconomyEndpoint="http://127.0.0.1:17881",bool AutoStartLocalEconomy=true,long Revision=0)
{
    internal static string SettingsPath => Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"Fractal Dev Games","SSW","service-endpoints.json");
    internal bool IsLocal => IsLoopback(new Uri(EconomyEndpoint).IdnHost);
    private static bool IsLoopback(string host) => host.Trim('[',']').Equals("localhost",StringComparison.OrdinalIgnoreCase)||host.Trim('[',']') is "127.0.0.1" or "::1";
    internal static bool TryNormalize(string address,out string normalized,out string reason)
    {
        normalized="";reason="Use an HTTP localhost address or an authenticated HTTPS server address.";
        if(string.IsNullOrWhiteSpace(address)||address.Length>2048||address.Any(c=>char.IsWhiteSpace(c)||char.IsControl(c))||address.Contains('\\')||
            !Regex.IsMatch(address,@"^https?://(\[[0-9a-fA-F:]+\]|[a-zA-Z0-9][a-zA-Z0-9.-]*)(:[0-9]{1,5})?(/[^?#]*)?$",RegexOptions.CultureInvariant)||
            !Uri.TryCreate(address,UriKind.Absolute,out var uri)||uri.Port<1||!string.IsNullOrEmpty(uri.UserInfo)||!string.IsNullOrEmpty(uri.Query)||!string.IsNullOrEmpty(uri.Fragment))return false;
        if(uri.Scheme!="https"&&!(uri.Scheme=="http"&&IsLoopback(uri.IdnHost)))return false;
        var host=uri.IdnHost.Trim('[',']');
        if(host.Contains(':')){if(!IPAddress.TryParse(host,out _))return false;}
        else foreach(string label in host.Split('.'))if(label.Length is <1 or >63||label.StartsWith('-')||label.EndsWith('-'))return false;
        normalized=uri.AbsoluteUri.TrimEnd('/');reason="";return true;
    }
    internal static ServiceEndpoints Load(string? path=null)
    {
        path??=SettingsPath;if(!File.Exists(path))return new();
        if(new FileInfo(path).Length>16384)throw new InvalidDataException("Service settings exceed their size limit.");
        using var document=JsonDocument.Parse(File.ReadAllText(path));var root=document.RootElement;
        if(root.GetProperty("schema").GetInt32()!=1)throw new InvalidDataException("Unsupported service settings version.");
        var economy=root.GetProperty("economy");
        if(!TryNormalize(economy.GetProperty("endpoint").GetString()??"",out var endpoint,out var reason))throw new InvalidDataException(reason);
        long revision=root.GetProperty("revision").GetInt64();if(revision<0||revision>9007199254740991L)throw new InvalidDataException("Invalid service settings revision.");
        return new(endpoint,economy.GetProperty("autoStartLocal").GetBoolean(),revision);
    }
    // Shared with the future admin console. Endpoint changes do not hot-switch a live game.
    internal static ServiceEndpoints Save(string endpoint,bool autoStart,long expectedRevision,string? path=null)
    {
        if(!TryNormalize(endpoint,out var normalized,out var reason))throw new InvalidDataException(reason);
        path??=SettingsPath;Directory.CreateDirectory(Path.GetDirectoryName(Path.GetFullPath(path))!);
        using var writer=File.Open(path+".lock",FileMode.OpenOrCreate,FileAccess.ReadWrite,FileShare.None);
        var previous=Load(path);if(previous.Revision!=expectedRevision||expectedRevision>=9007199254740991L)throw new InvalidOperationException("Service settings changed; reload before applying.");
        var next=new ServiceEndpoints(normalized,autoStart,expectedRevision+1);
        string json=JsonSerializer.Serialize(new {schema=1,revision=next.Revision,economy=new {endpoint=next.EconomyEndpoint,autoStartLocal=next.AutoStartLocalEconomy}},new JsonSerializerOptions{WriteIndented=true});
        string temporary=path+".pending";
        using(var file=new FileStream(temporary,FileMode.Create,FileAccess.Write,FileShare.None))using(var text=new StreamWriter(file)){text.Write(json);text.Flush();file.Flush(true);}
        if(File.Exists(path))File.Replace(temporary,path,path+".bak");else File.Move(temporary,path);
        return next;
    }
}
