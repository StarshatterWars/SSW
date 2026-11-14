using System.IO;
using System.IO.Compression;
using System.Net.Http;
using System.Security.Cryptography;
using System.Text.Json;

namespace StarshatterWarsLauncher;

internal sealed class GamePatcher(HttpClient http)
{
    internal sealed record Entry(string Path, long Size, string Sha256);
    internal sealed record Manifest(string Version, Entry[] Files, string? About = null);
    internal sealed record TransferProgress(string Stage, long Bytes, long Total);
    private static readonly JsonSerializerOptions Json = new(JsonSerializerDefaults.Web);
    private const string Saved = "StarshatterWars/Saved";
    private const string ModsNotice = Saved + "/Mods/SSW-Mods-Notice.txt";
    private const string ServerSaved = "StarServer/WindowsServer/StarshatterWars/Saved";
    private const string LocalServerConfig = "Saved/StarServer/Config";

    internal async Task<string> UpdateAsync(string install, Uri manifestUrl, string publicKey,
        IProgress<string> progress, Func<bool> gameIsRunning, CancellationToken token,
        IProgress<TransferProgress>? transfer = null, bool freshInstall = false, bool overwrite = false,
        Uri? archiveUrl = null, IProgress<string>? releaseNotes = null)
    {
        Manifest manifest = await ReadManifestAsync(manifestUrl, publicKey, token);
        return await ApplyAsync(install, manifestUrl, manifest, progress, gameIsRunning, token,
            transfer, freshInstall, overwrite, archiveUrl, releaseNotes);
    }

    internal async Task<Manifest> ReadManifestAsync(Uri manifestUrl, string publicKey, CancellationToken token)
    {
        if (!IsDownloadUrl(manifestUrl) ||
            !string.IsNullOrEmpty(manifestUrl.Query) || !string.IsNullOrEmpty(manifestUrl.Fragment))
            throw new InvalidDataException("Use HTTPS or a loopback HTTP address, without credentials, query, or fragment.");

        byte[] manifestBytes = await http.GetByteArrayAsync(manifestUrl, token);
        byte[] signature = await http.GetByteArrayAsync(new Uri(manifestUrl.AbsoluteUri + ".sig"), token);
        using var key = RSA.Create();
        key.ImportFromPem(publicKey);
        if (key.KeySize < 2048 || !key.VerifyData(manifestBytes, signature, HashAlgorithmName.SHA256, RSASignaturePadding.Pkcs1))
            throw new InvalidDataException("The update signature is invalid.");

        Manifest manifest = JsonSerializer.Deserialize<Manifest>(manifestBytes, Json) ??
            throw new InvalidDataException("The manifest is empty.");
        Validate(manifest);
        return manifest;
    }

    private async Task<string> ApplyAsync(string install, Uri manifestUrl, Manifest manifest,
        IProgress<string> progress, Func<bool> gameIsRunning, CancellationToken token,
        IProgress<TransferProgress>? transfer, bool freshInstall, bool overwrite,
        Uri? archiveUrl, IProgress<string>? releaseNotes)
    {
        string versionPath = Path.Combine(install, ".ssw-version");
        string installedVersion = File.Exists(versionPath) ? (await File.ReadAllTextAsync(versionPath, token)).Trim() : "";
        if (!freshInstall && IsBuildStamp(installedVersion) && IsBuildStamp(manifest.Version) &&
            string.CompareOrdinal(manifest.Version, installedVersion) < 0)
            return $"Already on a newer build · {installedVersion}";
        if (!string.IsNullOrWhiteSpace(manifest.About))
            releaseNotes?.Report(manifest.About);
        long total = manifest.Files.Sum(file => file.Size);
        long completed = 0;
        transfer?.Report(new("Downloading SSW", 0, archiveUrl is null ? total : 0));
        if (freshInstall) Directory.CreateDirectory(install);
        if (freshInstall && !overwrite && LocalInstaller.HasExistingFiles(install))
            throw new IOException("The installation folder now contains files. Confirm overwrite and try again.");

        string stage = LocalInstaller.StagePath(install);
        bool changed = false;
        Directory.CreateDirectory(stage);
        try
        {
            if (freshInstall && archiveUrl is not null)
            {
                await DownloadArchiveAsync(archiveUrl, stage, manifest, transfer, token);
                changed = true;
            }
            else for (int index = 0; index < manifest.Files.Length; index++)
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
                    long offset = completed;
                    await DownloadAsync(fileUrl, destination, entry.Size, token,
                        transfer is null ? null : bytes => transfer.Report(new("Downloading SSW", offset + bytes, total)));
                    if (!await MatchesAsync(destination, entry, token))
                        throw new InvalidDataException($"Verification failed for {entry.Path}.");
                }
                completed += entry.Size;
            }

            if (archiveUrl is null) transfer?.Report(new("Downloaded SSW", total, total));

            if (!freshInstall && !changed && installedVersion == manifest.Version)
                return $"Already up to date · {manifest.Version}";

            // Keep pilot saves, mods, and local settings with the new install.
            CopySaved(Path.Combine(install, Saved), Path.Combine(stage, Saved), token);
            CopySaved(Path.Combine(install, ServerSaved), Path.Combine(stage, ServerSaved), token);
            CopySaved(Path.Combine(install, LocalServerConfig), Path.Combine(stage, LocalServerConfig), token);
            await File.WriteAllTextAsync(Path.Combine(stage, ".ssw-version"), manifest.Version, token);
            token.ThrowIfCancellationRequested();
            if (gameIsRunning()) throw new IOException("Close Starshatter Wars before installing the update.");
            progress.Report("Installing verified build…");
            transfer?.Report(new("Installing SSW", 0, 1));
            LocalInstaller.Publish(install, stage, !freshInstall || overwrite);
            transfer?.Report(new("Installed SSW", 1, 1));
            return $"Installed {manifest.Version}";
        }
        finally
        {
            if (Directory.Exists(stage)) Directory.Delete(stage, recursive: true);
        }
    }

    private static bool IsDownloadUrl(Uri url) => url.IsAbsoluteUri &&
        string.IsNullOrEmpty(url.UserInfo) &&
        (url.Scheme == "https" || (url.Scheme == "http" && url.IsLoopback));

    private static bool IsBuildStamp(string version) =>
        DateTime.TryParseExact(version, "yyyyMMdd-HHmmss'Z'", System.Globalization.CultureInfo.InvariantCulture,
            System.Globalization.DateTimeStyles.None, out _);

    private async Task DownloadArchiveAsync(Uri url, string stage, Manifest manifest,
        IProgress<TransferProgress>? progress, CancellationToken token)
    {
        if (!IsDownloadUrl(url)) throw new InvalidDataException("Use HTTPS or loopback HTTP for the ZIP.");
        string archive = stage + ".zip";
        try
        {
            using (var response = await http.GetAsync(url, HttpCompletionOption.ResponseHeadersRead, token))
            {
                response.EnsureSuccessStatusCode();
                await using Stream input = await response.Content.ReadAsStreamAsync(token);
                await using var output = File.Create(archive);
                long limit = checked(manifest.Files.Sum(file => file.Size) + 128L * 1024 * 1024);
                long expected = response.Content.Headers.ContentLength ?? 0;
                if (expected > limit) throw new InvalidDataException("The ZIP exceeds the release size limit.");
                byte[] buffer = new byte[128 * 1024];
                long written = 0, reported = 0;
                int count;
                while ((count = await input.ReadAsync(buffer, token)) > 0)
                {
                    if (count > limit - written) throw new InvalidDataException("The ZIP exceeds the release size limit.");
                    await output.WriteAsync(buffer.AsMemory(0, count), token);
                    written += count;
                    if (written - reported >= 4 * 1024 * 1024)
                    {
                        progress?.Report(new("Downloading SSW", written, expected));
                        reported = written;
                    }
                }
                if (expected > 0 && written != expected) throw new InvalidDataException("The ZIP download is incomplete.");
                progress?.Report(new("Downloaded SSW", written, written));
            }
            using var zip = ZipFile.OpenRead(archive);
            var entries = new Dictionary<string, ZipArchiveEntry>(StringComparer.OrdinalIgnoreCase);
            foreach (var entry in zip.Entries)
            {
                string relative = entry.FullName.Replace('\\', '/');
                if (relative.StartsWith("./", StringComparison.Ordinal)) relative = relative[2..];
                if (entry.Name.Length == 0) continue;
                if (!entries.TryAdd(relative, entry)) throw new InvalidDataException("Duplicate file in ZIP.");
            }
            long extracted = 0, total = manifest.Files.Sum(file => file.Size);
            foreach (Entry file in manifest.Files)
            {
                token.ThrowIfCancellationRequested();
                if (!entries.Remove(file.Path, out var entry) || entry.Length != file.Size)
                    throw new InvalidDataException($"ZIP file missing or has the wrong size: {file.Path}");
                string path = Path.Combine(stage, file.Path);
                Directory.CreateDirectory(Path.GetDirectoryName(path)!);
                await using (var input = entry.Open())
                await using (var output = File.Create(path))
                {
                    byte[] buffer = new byte[128 * 1024];
                    long remaining = file.Size;
                    int count;
                    while ((count = await input.ReadAsync(buffer, token)) > 0)
                    {
                        if (count > remaining) throw new InvalidDataException($"ZIP entry exceeds its signed size: {file.Path}");
                        await output.WriteAsync(buffer.AsMemory(0, count), token);
                        remaining -= count;
                    }
                }
                if (!await MatchesAsync(path, file, token)) throw new InvalidDataException($"Verification failed for {file.Path}.");
                extracted += file.Size;
                progress?.Report(new("Extracting SSW", extracted, total));
            }
            if (entries.Count != 0) throw new InvalidDataException("The ZIP contains files absent from the signed manifest.");
        }
        finally { File.Delete(archive); }
    }

    private static void Validate(Manifest manifest)
    {
        if (string.IsNullOrWhiteSpace(manifest.Version) || manifest.Version.Length > 80 ||
            manifest.Version.Any(char.IsControl) || manifest.Files is not { Length: > 0 } ||
            manifest.About is { Length: > 500 })
            throw new InvalidDataException("The manifest has no valid version or files.");

        var paths = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach (Entry entry in manifest.Files)
        {
            if (entry is null || string.IsNullOrEmpty(entry.Path) ||
                entry.Path.Equals(Saved, StringComparison.OrdinalIgnoreCase) ||
                (entry.Path.StartsWith(Saved + "/", StringComparison.OrdinalIgnoreCase) &&
                 !entry.Path.Equals(ModsNotice, StringComparison.OrdinalIgnoreCase)) ||
                entry.Path.Equals(ServerSaved, StringComparison.OrdinalIgnoreCase) ||
                entry.Path.StartsWith(ServerSaved + "/", StringComparison.OrdinalIgnoreCase) ||
                entry.Path.Equals(LocalServerConfig, StringComparison.OrdinalIgnoreCase) ||
                entry.Path.StartsWith(LocalServerConfig + "/", StringComparison.OrdinalIgnoreCase) ||
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

    private async Task DownloadAsync(Uri uri, string destination, long size, CancellationToken token, Action<long>? progress)
    {
        using var response = await http.GetAsync(uri, HttpCompletionOption.ResponseHeadersRead, token);
        response.EnsureSuccessStatusCode();
        await using Stream input = await response.Content.ReadAsStreamAsync(token);
        await using var output = File.Create(destination);
        byte[] buffer = new byte[128 * 1024];
        long written = 0;
        long lastReported = 0;
        int count;
        while ((count = await input.ReadAsync(buffer, token)) != 0)
        {
            if (count > size - written) throw new InvalidDataException("A download exceeds its manifest size.");
            await output.WriteAsync(buffer.AsMemory(0, count), token);
            written += count;
            if (written == size || written - lastReported >= 4 * 1024 * 1024)
            {
                progress?.Invoke(written);
                lastReported = written;
            }
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
            else if (!File.Exists(target)) File.Copy(path, target);
        }
    }
}
