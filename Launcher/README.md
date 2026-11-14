# Starshatter Wars Launcher

Open `Build/Launcher/StarshatterWarsLauncher.exe`. It finds `Build/Windows/StarshatterWars.exe` relative to the project directory. The installed build label shows the game binary's UTC build timestamp and its Unreal Engine version.

**Play with Epic** signs in through the EOS SDK using the desktop client in the project-local `Config/UserEngine.ini`. The game attempts persistent authentication and loads the account-linked pilot save. **Play with Steam** checks the running Steam client for the SSW App ID configured as `steamAppId` in `launcher.json`. The game confirms the same Steam account. If either handoff fails, the game shows its existing sign-in screen. **Play Offline** launches with local-play settings. The Music button toggles the bundled SSW menu loop.

Set `feedUrl` in `Build/Launcher/launcher.json` to the HTTPS URL of a JSON file with the same structure as `feed.json`. The launcher displays the bundled feed if the URL is unset or the request fails. The WebView2 Runtime is needed to display news.

Publish from the project root with:

```powershell
dotnet publish Launcher/StarshatterWarsLauncher.csproj -c Release -r win-x64 --self-contained true -o Build/Launcher
```

## Install a local copy

Select **Install Game…** and choose a new or empty folder. The launcher copies the current Windows package, omits developer symbols and the source build's Saved directory, and remembers the installed copy for future launches and updates.

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
