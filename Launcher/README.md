# Starshatter Wars Launcher

Open `Build/Launcher/StarshatterWarsLauncher.exe`. It finds `Build/Windows/StarshatterWars.exe` relative to the project directory. The installed build label shows the game binary's UTC build timestamp and its Unreal Engine version.

**Play with Epic** signs in through the EOS SDK using the desktop client in the project-local `Config/UserEngine.ini`. The game attempts persistent authentication and loads the account-linked pilot save. **Play with Steam** checks the running Steam client for the SSW App ID configured as `steamAppId` in `launcher.json`. The game confirms the same Steam account. If either handoff fails, the game shows its existing sign-in screen. **Play Offline** launches with local-play settings. The Music button toggles the bundled SSW menu loop.

Set `feedUrl` in `Build/Launcher/launcher.json` to the HTTPS URL of a JSON file with the same structure as `feed.json`. The launcher displays the bundled feed if the URL is unset or the request fails. The WebView2 Runtime is needed to display news.

Publish from the project root with:

```powershell
dotnet publish Launcher/StarshatterWarsLauncher.csproj -c Release -r win-x64 --self-contained true -o Build/Launcher
```

## Install a local copy

Select **Install Gameâ€¦** and choose a new or empty folder. The launcher copies the current Windows package, omits developer symbols and the source build's Saved directory, and remembers the installed copy for future launches and updates.

## Publish updates

For a full ZIP install, ZIP the contents of the release's `files` directory so both the top-level executable and the game subdirectory are at the archive root. Configure `gameArchiveUrl` with a direct download URL that returns ZIP bytes. The launcher downloads and verifies the signed manifest first, then downloads the ZIP, extracts the listed files, and checks their signed sizes and SHA-256 hashes before installing. Keep `files/` online for incremental updates.

For a localhost test MSI, pass a configured JSON file to `New-LauncherInstaller.ps1 -LauncherConfig`. Loopback HTTP is supported for testing. A Google Drive sharing page or confirmation page is not a direct ZIP download endpoint.

## Easy Anti-Cheat bootstrapper

For a protected release, use Epic's Windows `dist` files from the EOS SDK download under `SDK/Tools/EOS_AntiCheatTools`. Place `start_protected_game.exe` at the package root and the `EasyAntiCheat` directory beside it. Set its `Settings.json` to the product, sandbox, and deployment from the Epic Developer Portal and point its executable setting at `StarshatterWars/Binaries/Win64/StarshatterWars.exe`. Publish the protected files with the signed release. Set `useEasyAntiCheatBootstrapper` to `true` in that launcher's configuration to route Play with Epic through the bootstrapper. Play Offline uses the regular executable.

The protected release also requires Easy Anti-Cheat service installation and EOS Anti-Cheat session enforcement in multiplayer.

Run `New-PatchRelease.ps1` with the packaged game directory, a new output directory, an RSA private key PEM, and a short plain-English `-About` description for players. Host the generated `manifest.json`, `manifest.json.sig`, and `files/` together over HTTPS. Set `patchManifestUrl` and `patchPublicKeyPem` in the launcher's `launcher.json`. The script prints the public key to use. Keep the private key with the release tooling.

The release script stages the local server under `StarServer/WindowsServer` alongside the game.
By default, releases use a UTC build stamp in `yyyyMMdd-HHmmssZ` format. The launcher compares signed
build stamps and skips older releases. Use the same stamp for the manifest and full ZIP of a release.
The release also includes `SSW-Mods-Notice.txt` in the game's `Saved/Mods` directory.
It uses `Build/StarServer/WindowsServer` by default. Pass `-StarServerPackage` to select another server build.
Include that directory in the same signed ZIP. The launcher extracts it inside the selected SSW installation.
For EOS hosting, provision `Saved/StarServer/Config/Custom/StarServer/DefaultEngine.ini` locally inside the SSW installation.

**Check for Updates** verifies the signed manifest and each file. It downloads changed files, carries saves and the local StarServer configuration forward, and keeps the previous install beside the new one.


## Bundled local release server

Launcher 1.0.12 includes an opt-in, launcher-owned Apache HTTP Server 2.4.71
for localhost installation tests. Stage its reviewed Windows runtime with
`New-LocalFeedServerBundle.ps1`; the installer script includes it automatically.
The published Apache Lounge SHA-256 is pinned and its original license and
notices travel with the minimal static-file runtime. The distributor requires
the current Visual C++ x64 runtime; a missing runtime produces a readable error.

`launcher.localhost.json` is the current test configuration: it uses port 18787
and the separate `D:/SSW/Build/LauncherFeed` release directory. Pass it explicitly
with `New-LauncherInstaller.ps1 -LauncherConfig`. Populate its `current` folder
with the signed manifest, signature, full ZIP, news and `files` patch payload
only after a new game package is verified. The game download is not embedded
in the launcher installer. Source `launcher.json` keeps local hosting off by
default; future hosted HTTPS feeds bypass Apache even when it is bundled.

The local process has no visible window, listens on 127.0.0.1 only, and has a
private readiness token and startup deadline. It is attached to a Windows
lifetime job and stops when the launcher exits. It does not install a service,
require elevation, or stop/reconfigure XAMPP or another Apache instance. A busy
port or missing release files gets a clear error while bundled news remains
readable. Playing still requires the signed current release: no cached-version
or offline patch-check bypass has been introduced.


### Pilot save recovery (launcher 1.0.13)

Repair → Recover Pilot Save lists only complete, native-validated autosave
checkpoints, with pilot name, local date/time and points. Select a checkpoint
and choose Back Up and Restore. The installed game must be closed, and recovery
shares the install/update lock. The game build must include the native recovery
handler; older builds are rejected before starting a process.

Recovery runs headless and offline before game subsystems or sign-in initialize.
It validates all four checkpoint areas and the selected profile/date, round-trips
the reconstructed pilot and index, then copies and verifies every current `.sav`
under `StarshatterWars/Saved/PilotRecoveryBackups/<UTC-date>-<unique-id>`.
Only after backup verification does it replace the account save, named pilot,
profile index and active checkpoint manifest. Selecting an older checkpoint also
selects its matching universe/economy generation. Failed writes attempt rollback
from the verified backup. Original checkpoints remain available. Recovery reports
are retained in `StarshatterWars/Saved/PilotRecoveryRequests`.

The launcher cannot close during recovery. Reparse-point save folders/files are
rejected. Recovery is local maintenance, never an online game or EAC session.

### Store panel (launcher 1.0.14)

News and Store have separate tabs. The Store is a presentation placeholder; no checkout, real products, prices or entitlements are active. An optional `storeUrl` in `launcher.json` enables the official-store browser button. It accepts HTTPS URLs without embedded credentials. Keep it null until a provider is configured.

Epic sign-in tries EOS Persistent Auth before Account Portal. The SDK reserves email/password login for Epic internal use; the launcher never collects or forwards Epic passwords. Steam remains deferred.
