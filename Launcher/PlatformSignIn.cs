using System.IO;
using System.Runtime.InteropServices;
using System.Text.RegularExpressions;

namespace StarshatterWarsLauncher;

internal static class PlatformSignIn
{
    private const string EosLibrary = "EOSSDK-Win64-Shipping.dll";
    private const string SteamLibrary = "steam_api64.dll";
    private static nint eosLibrary, steamLibrary;
    private static bool eosInitialized;

    static PlatformSignIn()
    {
        NativeLibrary.SetDllImportResolver(typeof(PlatformSignIn).Assembly, (name, _, _) =>
            name == EosLibrary ? eosLibrary : name == SteamLibrary ? steamLibrary : 0);
    }

    public static async Task<string> EpicAsync(string game, string sourceGame, CancellationToken cancellation)
    {
        string install = Path.GetDirectoryName(game)!;
        string config = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
            "StarshatterWars", "LauncherEOS.ini");
        if (!File.Exists(config))
            config = Path.GetFullPath(Path.Combine(Path.GetDirectoryName(sourceGame)!, "..", "..", "Config", "UserEngine.ini"));
        if (!File.Exists(config))
            throw new InvalidOperationException("Configure the Epic desktop client in Config/UserEngine.ini.");
        string ini = await File.ReadAllTextAsync(config, cancellation);
        string artifact = Regex.Match(ini, @"(?m)^\s*DefaultArtifactName\s*=\s*([^\r\n]+)").Groups[1].Value.Trim().Trim('"');
        string Field(string row, string key)
        {
            Match match = Regex.Match(row, $@"(?:\(|,)\s*{key}\s*=\s*(?:""([^""]*)""|([^,)]+))");
            return (match.Groups[1].Success ? match.Groups[1].Value : match.Groups[2].Value).Trim();
        }
        string row = ini.Split('\n').FirstOrDefault(line =>
            line.TrimStart().StartsWith("+Artifacts=", StringComparison.OrdinalIgnoreCase) &&
            Field(line, "ArtifactName") == artifact) ?? "";
        var options = new EosPlatformOptions
        {
            Version = 14, Product = Field(row, "ProductId"), Sandbox = Field(row, "SandboxId"),
            Deployment = Field(row, "DeploymentId"), Client = Field(row, "ClientId"),
            Secret = Field(row, "ClientSecret"), Flags = 2
        };
        if (new[] {options.Product, options.Sandbox, options.Deployment, options.Client, options.Secret}.Any(string.IsNullOrWhiteSpace))
            throw new InvalidOperationException("The selected Epic desktop client configuration is incomplete.");

        if (eosLibrary == 0) eosLibrary = NativeLibrary.Load(Path.Combine(install, "Engine", "Binaries", "Win64", EosLibrary));
        if (!eosInitialized)
        {
            var init = new EosInit { Version = 4, Product = "StarshatterWars", Build = "Launcher" };
            int result = EOS_Initialize(ref init);
            if (result != 0) throw new InvalidOperationException($"Epic SDK initialization failed ({result}).");
            eosInitialized = true;
        }

        nint platform = EOS_Platform_Create(ref options);
        if (platform == 0) throw new InvalidOperationException("Could not initialize the Epic desktop client.");
        try
        {
            nint auth = EOS_Platform_GetAuthInterface(platform);
            if (auth == 0) throw new InvalidOperationException("Epic authentication is unavailable.");
            int result = await Login(auth, platform, 2, cancellation);
            if (result != 0) result = await Login(auth, platform, 6, cancellation);
            if (result != 0) throw new InvalidOperationException($"Epic sign-in did not complete ({result}).");
            return artifact;
        }
        finally { EOS_Platform_Release(platform); }
    }

    private static async Task<int> Login(nint auth, nint platform, int type, CancellationToken cancellation)
    {
        int? result = null;
        LoginCallback callback = data => result = Marshal.ReadInt32(data);
        nint credentials = Marshal.AllocHGlobal(Marshal.SizeOf<EosCredentials>());
        try
        {
            Marshal.StructureToPtr(new EosCredentials { Version = 4, Type = type }, credentials, false);
            var options = new EosLoginOptions { Version = 3, Credentials = credentials, Scope = 1 };
            EOS_Auth_Login(auth, ref options, 0, callback);
        }
        finally { Marshal.FreeHGlobal(credentials); }

        try
        {
            while (result is null)
            {
                cancellation.ThrowIfCancellationRequested();
                EOS_Platform_Tick(platform);
                await Task.Delay(20, cancellation);
            }
            return result.Value;
        }
        finally { GC.KeepAlive(callback); }
    }

    public static ulong Steam(string game, uint appId)
    {
        if (appId == 0) throw new InvalidOperationException("Set steamAppId in launcher.json, then sign into the Steam desktop client.");
        string steamworks = Path.Combine(Path.GetDirectoryName(game)!, "Engine", "Binaries", "ThirdParty", "Steamworks");
        if (steamLibrary == 0)
            steamLibrary = NativeLibrary.Load(Directory.EnumerateFiles(steamworks, SteamLibrary, SearchOption.AllDirectories).First());
        string? oldApp = Environment.GetEnvironmentVariable("SteamAppId");
        string? oldGame = Environment.GetEnvironmentVariable("SteamGameId");
        Environment.SetEnvironmentVariable("SteamAppId", appId.ToString());
        Environment.SetEnvironmentVariable("SteamGameId", appId.ToString());
        try
        {
            if (SteamAPI_InitFlat(0) != 0)
                throw new InvalidOperationException("Open Steam, sign in, and check that this account can use the configured App ID.");
            try
            {
                nint user = SteamAPI_SteamUser_v023();
                if (user == 0 || !SteamAPI_ISteamUser_BLoggedOn(user))
                    throw new InvalidOperationException("Sign in to the Steam desktop client first.");
                return SteamAPI_ISteamUser_GetSteamID(user);
            }
            finally { SteamAPI_Shutdown(); }
        }
        finally
        {
            Environment.SetEnvironmentVariable("SteamAppId", oldApp);
            Environment.SetEnvironmentVariable("SteamGameId", oldGame);
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct EosInit
    {
        public int Version;
        public nint Allocate, Reallocate, Release;
        [MarshalAs(UnmanagedType.LPUTF8Str)] public string Product;
        [MarshalAs(UnmanagedType.LPUTF8Str)] public string Build;
        public nint Reserved, SystemOptions, ThreadAffinity;
    }
    [StructLayout(LayoutKind.Sequential)]
    private struct EosPlatformOptions
    {
        public int Version;
        public nint Reserved;
        [MarshalAs(UnmanagedType.LPUTF8Str)] public string Product, Sandbox, Client, Secret;
        public int IsServer;
        public nint Encryption, Country, Locale;
        [MarshalAs(UnmanagedType.LPUTF8Str)] public string Deployment;
        public ulong Flags;
        public nint Cache;
        public uint TickBudget;
        public nint Rtc, Integrated, SystemOptions, Timeout;
    }
    [StructLayout(LayoutKind.Sequential)]
    private struct EosCredentials
    {
        public int Version;
        public nint Id, Token;
        public int Type;
        public nint SystemOptions;
        public int ExternalType;
    }
    [StructLayout(LayoutKind.Sequential)]
    private struct EosLoginOptions
    {
        public int Version;
        public nint Credentials;
        public int Scope;
        public ulong Flags;
    }

    [UnmanagedFunctionPointer(CallingConvention.Winapi)]
    private delegate void LoginCallback(nint data);
    [DllImport(EosLibrary)] private static extern int EOS_Initialize(ref EosInit options);
    [DllImport(EosLibrary)] private static extern nint EOS_Platform_Create(ref EosPlatformOptions options);
    [DllImport(EosLibrary)] private static extern void EOS_Platform_Release(nint platform);
    [DllImport(EosLibrary)] private static extern nint EOS_Platform_GetAuthInterface(nint platform);
    [DllImport(EosLibrary)] private static extern void EOS_Platform_Tick(nint platform);
    [DllImport(EosLibrary)] private static extern void EOS_Auth_Login(nint auth, ref EosLoginOptions options, nint data, LoginCallback callback);
    [DllImport(SteamLibrary, CallingConvention = CallingConvention.Cdecl)] private static extern int SteamAPI_InitFlat(nint error);
    [DllImport(SteamLibrary, CallingConvention = CallingConvention.Cdecl)] private static extern void SteamAPI_Shutdown();
    [DllImport(SteamLibrary, CallingConvention = CallingConvention.Cdecl)] private static extern nint SteamAPI_SteamUser_v023();
    [DllImport(SteamLibrary, CallingConvention = CallingConvention.Cdecl)]
    [return: MarshalAs(UnmanagedType.I1)] private static extern bool SteamAPI_ISteamUser_BLoggedOn(nint user);
    [DllImport(SteamLibrary, CallingConvention = CallingConvention.Cdecl)] private static extern ulong SteamAPI_ISteamUser_GetSteamID(nint user);
}
