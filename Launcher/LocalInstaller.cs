using System.IO;

namespace StarshatterWarsLauncher;

internal static class LocalInstaller
{
    internal static string Install(string source, string destination, IProgress<string> progress, CancellationToken token)
    {
        source = Path.TrimEndingDirectorySeparator(Path.GetFullPath(source));
        destination = Path.TrimEndingDirectorySeparator(Path.GetFullPath(destination));
        if (destination.Equals(Path.GetPathRoot(destination), StringComparison.OrdinalIgnoreCase) ||
            destination.Equals(source, StringComparison.OrdinalIgnoreCase) ||
            destination.StartsWith(source + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase) ||
            source.StartsWith(destination + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase))
            throw new IOException("Choose a separate folder for the installation.");

        if (Directory.Exists(destination) && Directory.EnumerateFileSystemEntries(destination).Any())
            throw new IOException("Choose a new or empty installation folder.");

        string stage = destination + "-Installing-" + Guid.NewGuid().ToString("N");
        Directory.CreateDirectory(stage);
        try
        {
            CopyDirectory(source, stage, "", progress, token);
            const string binary = "StarshatterWars/Binaries/Win64/StarshatterWars.exe";
            if (!File.Exists(Path.Combine(stage, "StarshatterWars.exe")) || !File.Exists(Path.Combine(stage, binary)))
                throw new IOException("The source package is missing a game executable.");
            token.ThrowIfCancellationRequested();
            if (Directory.Exists(destination)) Directory.Delete(destination);
            Directory.Move(stage, destination);
            return Path.Combine(destination, "StarshatterWars.exe");
        }
        finally
        {
            if (Directory.Exists(stage)) Directory.Delete(stage, recursive: true);
        }
    }

    private static void CopyDirectory(string source, string destination, string relative,
        IProgress<string> progress, CancellationToken token)
    {
        foreach (string path in Directory.EnumerateFileSystemEntries(source))
        {
            token.ThrowIfCancellationRequested();
            string name = Path.GetFileName(path);
            string entry = relative.Length == 0 ? name : relative + "/" + name;
            if (entry.Equals("StarshatterWars/Saved", StringComparison.OrdinalIgnoreCase) ||
                Path.GetExtension(name).Equals(".pdb", StringComparison.OrdinalIgnoreCase)) continue;
            FileAttributes attributes = File.GetAttributes(path);
            if ((attributes & FileAttributes.ReparsePoint) != 0)
                throw new IOException($"The package contains a linked file or directory: {entry}");
            string target = Path.Combine(destination, name);
            if ((attributes & FileAttributes.Directory) != 0)
            {
                Directory.CreateDirectory(target);
                CopyDirectory(path, target, entry, progress, token);
            }
            else
            {
                progress.Report($"Installing {entry}");
                File.Copy(path, target);
                if (new FileInfo(target).Length != new FileInfo(path).Length)
                    throw new IOException($"Incomplete copy: {entry}");
            }
        }
    }
}
