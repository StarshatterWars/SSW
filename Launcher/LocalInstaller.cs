using System.IO;

namespace StarshatterWarsLauncher;

internal static class LocalInstaller
{
    internal const string LockName = ".ssw-install.lock";
    internal static bool HasExistingFiles(string folder) =>
        Directory.EnumerateFileSystemEntries(folder).Any(path => Path.GetFileName(path) != LockName);

    internal static string StagePath(string folder) => Path.Combine(folder, ".ssw-stage-" + Guid.NewGuid().ToString("N"));

    internal static void Publish(string folder, string stage, bool overwrite)
    {
        string[] existing = Directory.EnumerateFileSystemEntries(folder)
            .Where(path => path != stage && Path.GetFileName(path) != LockName).ToArray();
        if (!overwrite && existing.Length > 0)
            throw new IOException("The installation folder now contains files. Confirm overwrite and try again.");
        string backup = Path.Combine(folder, ".ssw-backup-" + Guid.NewGuid().ToString("N"));
        var moved = new List<string>();
        var installed = new List<string>();
        if (existing.Length > 0) Directory.CreateDirectory(backup);
        static void Move(string source, string target)
        {
            if (Directory.Exists(source)) Directory.Move(source, target);
            else File.Move(source, target);
        }
        try
        {
            foreach (string path in existing)
            {
                Move(path, Path.Combine(backup, Path.GetFileName(path)));
                moved.Add(path);
            }
            foreach (string path in Directory.EnumerateFileSystemEntries(stage).ToArray())
            {
                string target = Path.Combine(folder, Path.GetFileName(path));
                Move(path, target);
                installed.Add(target);
            }
        }
        catch
        {
            foreach (string path in installed) Move(path, Path.Combine(stage, Path.GetFileName(path)));
            foreach (string path in moved) Move(Path.Combine(backup, Path.GetFileName(path)), path);
            if (Directory.Exists(backup)) Directory.Delete(backup);
            throw;
        }
    }

    internal static string Install(string source, string destination, IProgress<string> progress, CancellationToken token, bool overwrite = false)
    {
        source = Path.TrimEndingDirectorySeparator(Path.GetFullPath(source));
        destination = Path.TrimEndingDirectorySeparator(Path.GetFullPath(destination));
        if (destination.Equals(Path.GetPathRoot(destination), StringComparison.OrdinalIgnoreCase) ||
            destination.Equals(source, StringComparison.OrdinalIgnoreCase) ||
            destination.StartsWith(source + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase) ||
            source.StartsWith(destination + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase))
            throw new IOException("Choose a separate folder for the installation.");

        Directory.CreateDirectory(destination);
        string stage = StagePath(destination);
        Directory.CreateDirectory(stage);
        try
        {
            CopyDirectory(source, stage, "", progress, token);
            const string binary = "StarshatterWars/Binaries/Win64/StarshatterWars.exe";
            if (!File.Exists(Path.Combine(stage, "StarshatterWars.exe")) || !File.Exists(Path.Combine(stage, binary)))
                throw new IOException("The source package is missing a game executable.");
            token.ThrowIfCancellationRequested();
            Publish(destination, stage, overwrite);
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
