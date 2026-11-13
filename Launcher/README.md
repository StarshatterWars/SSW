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

Run `New-PatchRelease.ps1` with the packaged game directory, a new output directory, a release version, and an RSA private key PEM. Host the generated `manifest.json`, `manifest.json.sig`, and `files/` together over HTTPS. Set `patchManifestUrl` and `patchPublicKeyPem` in the launcher's `launcher.json`. The script prints the public key to use. Keep the private key with the release tooling.

**Check for Updates** verifies the signed manifest and each file. It downloads changed files, carries the existing Saved directory forward, and keeps the previous install beside the new one.
