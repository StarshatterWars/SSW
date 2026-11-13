using System.IO;
using System.Net.Http;
using System.Security.Cryptography;
using System.Text.Json;

namespace StarshatterWarsLauncher;

internal sealed class GamePatcher(HttpClient http)
{
    internal sealed record Entry(string Path, long Size, string Sha256);
    internal sealed record Manifest(string Version, Entry[] Files);
    private static readonly JsonSerializerOptions Json = new(JsonSerializerDefaults.Web);
    private const string Saved = "StarshatterWars/Saved";

    internal async Task<string> UpdateAsync(string install, Uri manifestUrl, string publicKey,
        IProgress<string> progress, Func<bool> gameIsRunning, CancellationToken token)
    {
        if (manifestUrl.Scheme != "https" || !string.IsNullOrEmpty(manifestUrl.UserInfo) ||
            !string.IsNullOrEmpty(manifestUrl.Query) || !string.IsNullOrEmpty(manifestUrl.Fragment))
            throw new InvalidDataException("Use an HTTPS manifest URL without credentials, query, or fragment.");

        byte[] manifestBytes = await http.GetByteArrayAsync(manifestUrl, token);
        byte[] signature = await http.GetByteArrayAsync(new Uri(manifestUrl.AbsoluteUri + ".sig"), token);
        using var key = RSA.Create();
        key.ImportFromPem(publicKey);
        if (key.KeySize < 2048 || !key.VerifyData(manifestBytes, signature, HashAlgorithmName.SHA256, RSASignaturePadding.Pkcs1))
            throw new InvalidDataException("The update signature is invalid.");

        Manifest manifest = JsonSerializer.Deserialize<Manifest>(manifestBytes, Json) ??
            throw new InvalidDataException("The manifest is empty.");
        Validate(manifest);

        string stage = install + "-Update-" + Guid.NewGuid().ToString("N");
        string backup = install + "-BeforeUpdate-" + Guid.NewGuid().ToString("N");
        bool changed = false;
        Directory.CreateDirectory(stage);
        try
        {
            for (int index = 0; index < manifest.Files.Length; index++)
            {
                token.ThrowIfCancellationRequested();
                Entry entry = manifest.Files[index];
                string destination = Path.Combine(stage, entry.Path);
                string source = Path.Combine(install, entry.Path);
                Directory.CreateDirectory(Path.GetDirectoryName(destination)!);
                progress.Report($"Verifying {index + 1:N0}/{manifest.Files.Length:N0}  {entry.Path}");
                if (File.Exists(source) && await MatchesAsync(source, entry, token))
                {
                    File.Copy(source, destination);
                    if (!await MatchesAsync(destination, entry, token))
                        throw new InvalidDataException($"Local file changed while staging {entry.Path}.");
                }
                else
                {
                    changed = true;
                    progress.Report($"Downloading {index + 1:N0}/{manifest.Files.Length:N0}  {entry.Path}");
                    Uri fileUrl = new(manifestUrl, "files/" + string.Join("/", entry.Path.Split('/').Select(Uri.EscapeDataString)));
                    await DownloadAsync(fileUrl, destination, entry.Size, token);
                    if (!await MatchesAsync(destination, entry, token))
                        throw new InvalidDataException($"Verification failed for {entry.Path}.");
                }
            }

            string versionPath = Path.Combine(install, ".ssw-version");
            if (!changed && File.Exists(versionPath) && (await File.ReadAllTextAsync(versionPath, token)).Trim() == manifest.Version)
                return $"Already up to date · {manifest.Version}";

            // Keep pilot saves, mods, and local settings with the new install.
            CopySaved(Path.Combine(install, Saved), Path.Combine(stage, Saved), token);
            await File.WriteAllTextAsync(Path.Combine(stage, ".ssw-version"), manifest.Version, token);
            token.ThrowIfCancellationRequested();
            if (gameIsRunning()) throw new IOException("Close Starshatter Wars before installing the update.");
            progress.Report("Installing verified build…");
            Directory.Move(install, backup);
            try { Directory.Move(stage, install); }
            catch { Directory.Move(backup, install); throw; }
            return $"Installed {manifest.Version}";
        }
        finally
        {
            if (Directory.Exists(stage)) Directory.Delete(stage, recursive: true);
        }
    }

    private static void Validate(Manifest manifest)
    {
        if (string.IsNullOrWhiteSpace(manifest.Version) || manifest.Version.Length > 80 ||
            manifest.Version.Any(char.IsControl) || manifest.Files is not { Length: > 0 })
            throw new InvalidDataException("The manifest has no valid version or files.");

        var paths = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach (Entry entry in manifest.Files)
        {
            if (entry is null || string.IsNullOrEmpty(entry.Path) ||
                entry.Path.Equals(Saved, StringComparison.OrdinalIgnoreCase) ||
                entry.Path.StartsWith(Saved + "/", StringComparison.OrdinalIgnoreCase) ||
                entry.Path.Equals(".ssw-version", StringComparison.OrdinalIgnoreCase) ||
                entry.Path.Split('/').Any(part => !ValidSegment(part)) ||
                entry.Size < 0 || entry.Sha256 is not { Length: 64 } || !entry.Sha256.All(Uri.IsHexDigit) ||
                !paths.Add(entry.Path))
                throw new InvalidDataException("The manifest contains an invalid or duplicate file entry.");
        }
        if (!paths.Contains("StarshatterWars.exe") || !paths.Contains("StarshatterWars/Binaries/Win64/StarshatterWars.exe"))
            throw new InvalidDataException("The manifest must include both game executables.");
    }

    private static bool ValidSegment(string part)
    {
        if (part.Length == 0 || part.EndsWith('.') || part.EndsWith(' ') ||
            part.IndexOfAny(Path.GetInvalidFileNameChars()) >= 0 || part.Contains('\\'))
            return false;
        string stem = part.Split('.')[0].ToUpperInvariant();
        return stem is not ("CON" or "PRN" or "AUX" or "NUL") &&
            !(stem.Length == 4 && (stem.StartsWith("COM") || stem.StartsWith("LPT")) &&
              "0123456789¹²³".Contains(stem[3]));
    }

    private async Task DownloadAsync(Uri uri, string destination, long size, CancellationToken token)
    {
        using var response = await http.GetAsync(uri, HttpCompletionOption.ResponseHeadersRead, token);
        response.EnsureSuccessStatusCode();
        await using Stream input = await response.Content.ReadAsStreamAsync(token);
        await using var output = File.Create(destination);
        byte[] buffer = new byte[128 * 1024];
        long written = 0;
        int count;
        while ((count = await input.ReadAsync(buffer, token)) != 0)
        {
            if (count > size - written) throw new InvalidDataException("A download exceeds its manifest size.");
            await output.WriteAsync(buffer.AsMemory(0, count), token);
            written += count;
        }
        if (written != size) throw new InvalidDataException("A download is incomplete.");
    }

    private static async Task<bool> MatchesAsync(string path, Entry entry, CancellationToken token)
    {
        if (new FileInfo(path).Length != entry.Size) return false;
        await using FileStream file = File.OpenRead(path);
        return Convert.ToHexString(await SHA256.HashDataAsync(file, token))
            .Equals(entry.Sha256, StringComparison.OrdinalIgnoreCase);
    }

    private static void CopySaved(string source, string destination, CancellationToken token)
    {
        if (!Directory.Exists(source)) return;
        if ((File.GetAttributes(source) & FileAttributes.ReparsePoint) != 0)
            throw new IOException("The Saved folder contains a linked directory.");
        Directory.CreateDirectory(destination);
        foreach (string path in Directory.EnumerateFileSystemEntries(source))
        {
            token.ThrowIfCancellationRequested();
            FileAttributes attributes = File.GetAttributes(path);
            if ((attributes & FileAttributes.ReparsePoint) != 0)
                throw new IOException("The Saved folder contains a linked file or directory.");
            string target = Path.Combine(destination, Path.GetFileName(path));
            if ((attributes & FileAttributes.Directory) != 0) CopySaved(path, target, token);
            else File.Copy(path, target);
        }
    }
}
