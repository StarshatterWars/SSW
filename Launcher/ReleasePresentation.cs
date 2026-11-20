using System.IO;
using System.Text.Json;
namespace StarshatterWarsLauncher;
internal static class ReleasePresentation
{
    internal const string DefaultWarning = "Work in progress. This alpha build may contain bugs, crashes, incomplete features and save incompatibilities. Gameplay and balance may change.";
    private static readonly Lazy<(string Stage,string Warning)> Info = new(() =>
    {
        try
        {
            using var doc=JsonDocument.Parse(File.ReadAllText(Path.Combine(AppContext.BaseDirectory,"release-info.json")));
            string stage=doc.RootElement.GetProperty("stage").GetString() ?? "Alpha 4";
            string warning=doc.RootElement.GetProperty("warning").GetString() ?? DefaultWarning;
            return (string.IsNullOrWhiteSpace(stage)?"Alpha 4":stage,string.IsNullOrWhiteSpace(warning)?DefaultWarning:warning);
        }
        catch(Exception ex) when(ex is IOException or UnauthorizedAccessException or JsonException or KeyNotFoundException or InvalidOperationException)
        { return ("Alpha 4",DefaultWarning); }
    });
    internal static string Stage => Info.Value.Stage;
    internal static string Warning => Info.Value.Warning;
    internal static string Version(string build) => $"{Stage} | Build {build}";
}
