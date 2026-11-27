namespace StarshatterWarsLauncher;

internal static class GameLaunchDiagnostics
{
    internal static string Describe(string provider, int? exitCode, bool gameRunning)
    {
        if (gameRunning) return provider == "NULL" ? "Started in offline mode." : "Signed in. Game started.";
        if (exitCode == 777006)
            return "The game crashed before startup (777006). This is a game-build error, not a sign-in failure. Install the corrected build when available.";
        if (exitCode is not null)
            return $"The game closed before opening a window (exit {exitCode}). Use Repair; if it continues, report this code with the installed build number.";
        return "The game is still starting. If no window appears, check the game log.";
    }
}
