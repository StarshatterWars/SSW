using System.Diagnostics;
using System.IO;
using System.Text.Json;

namespace StarshatterWarsLauncher;

internal static class PilotRecovery
{
    internal sealed record Checkpoint(string Slot, string Profile, string Pilot, DateTimeOffset SavedUtc, int Campaign, int Points)
    {
        public string Label => $"{SavedUtc.ToLocalTime():yyyy-MM-dd HH:mm:ss zzz}  ·  {(string.IsNullOrWhiteSpace(Pilot) ? "Unnamed pilot" : Pilot)}  ·  {Points:N0} points  ·  {Profile[^8..]}";
    }
    internal sealed record Result(bool Success, Checkpoint[]? Checkpoints, string? Backup, string? Error);
    private static readonly JsonSerializerOptions Json = new(JsonSerializerDefaults.Web);

    internal static async Task<Result> RunAsync(string game, Checkpoint? checkpoint, CancellationToken token)
    {
        string root = Path.GetDirectoryName(Path.GetFullPath(game))!;
        string binary = Path.Combine(root, "StarshatterWars", "Binaries", "Win64", "StarshatterWars.exe");
        if (!File.Exists(binary)) throw new IOException("Locate the packaged game before recovering a pilot.");
        if (!SupportsRecovery(binary)) throw new IOException("Install the next game build to enable pilot recovery. The current build has no recovery tool.");
        string saved = Path.Combine(root, "StarshatterWars", "Saved");
        string requests = Path.Combine(saved, "PilotRecoveryRequests");
        Directory.CreateDirectory(requests);
        // A reparse point could redirect a restore outside the selected installation.
        foreach (string folder in new[] { root, Path.Combine(root, "StarshatterWars"), saved, requests, Path.Combine(saved, "SaveGames"), Path.Combine(saved, "PilotRecoveryBackups") })
            if (Directory.Exists(folder) && (File.GetAttributes(folder) & FileAttributes.ReparsePoint) != 0)
                throw new IOException("Pilot recovery cannot use redirected save folders.");
        string saves = Path.Combine(saved, "SaveGames");
        if (Directory.Exists(saves) && Directory.EnumerateFileSystemEntries(saves).Any(p => (File.GetAttributes(p) & FileAttributes.ReparsePoint) != 0))
            throw new IOException("Pilot recovery cannot use redirected save files.");
        string path = Path.Combine(requests, Guid.NewGuid().ToString("N") + ".json");
        await File.WriteAllTextAsync(path, JsonSerializer.Serialize(new
        {
            action = checkpoint is null ? "list" : "restore",
            slot = checkpoint?.Slot, profile = checkpoint?.Profile,
            savedUtc = checkpoint?.SavedUtc.UtcDateTime.ToString("yyyy-MM-ddTHH:mm:ss.fffZ")
        }, Json), token);
        var start = new ProcessStartInfo(binary) { UseShellExecute = false, CreateNoWindow = true, WindowStyle = ProcessWindowStyle.Hidden, WorkingDirectory = root };
        foreach (string arg in new[] { "-SSWPilotRecovery", "-RecoveryRequest=" + path, "-SSWOffline", "-nullrhi", "-nosound", "-nosplash", "-unattended", "-windowed", "-ResX=1", "-ResY=1" }) start.ArgumentList.Add(arg);
        using var process = Process.Start(start) ?? throw new IOException("Could not start the pilot recovery tool.");
        // Do not interrupt a restore when the launcher closes. Native recovery owns rollback.
        await process.WaitForExitAsync();
        string report = path + ".result.json";
        if (!File.Exists(report)) throw new IOException("This game build does not support pilot recovery, or recovery could not start. Install the next game build and try again.");
        var result = JsonSerializer.Deserialize<Result>(await File.ReadAllTextAsync(report), Json)
            ?? throw new InvalidDataException("The recovery report is unreadable.");
        if (!result.Success) throw new IOException(result.Error ?? "Pilot recovery failed validation. Your recovery report is in Saved/PilotRecoveryRequests.");
        if (process.ExitCode != 0) throw new IOException("The recovery tool did not exit cleanly. Keep the recovery report and backup before trying again.");
        return result;
    }

    internal static bool SupportsRecovery(string binary)
    {
        byte[] marker = System.Text.Encoding.Unicode.GetBytes("SSWPilotRecovery");
        using var stream = File.OpenRead(binary);
        byte[] buffer = new byte[1024 * 1024 + marker.Length];
        int overlap = 0, count;
        while ((count = stream.Read(buffer, overlap, buffer.Length - overlap)) > 0)
        {
            int total = overlap + count;
            if (buffer.AsSpan(0, total).IndexOf(marker) >= 0) return true;
            overlap = Math.Min(marker.Length - 1, total);
            buffer.AsSpan(total - overlap, overlap).CopyTo(buffer);
        }
        return false;
    }
}
