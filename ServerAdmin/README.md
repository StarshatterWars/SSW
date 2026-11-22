# FractalVerse Admin

Read-only Windows monitoring app for the local persistent universe server. Uses the same signed-in player's local FractalVerse status directory as the launcher; it does not open pilot saves, modify inventories, issue game commands or provide unauthenticated remote administration.

Build: `dotnet publish ServerAdmin/FractalVerseAdmin.csproj -c Release -r win-x64 --self-contained true -o Build/FractalVerseAdmin`

Run `FractalVerseAdmin.exe`. Select Server Folder to watch another local/test universe. An optional `--status-dir "absolute folder"` selects that folder on startup. The default is `%LOCALAPPDATA%/Fractal Dev Games/FractalVerse/LocalUniverse`.

Displays a recent server heartbeat, server GMT, uptime, client capacity and roster/ping, revision/protocol, checkpoint status/time, inventory/journey/production counts and local process memory/CPU time. View Recent Log reads only the last 256 KiB of `FractalVerse.log`. A stopped/stale server is not presented as online. Remote administration, authenticated commands and deployment controls remain separate Phase 5 work.

UI verification: `dotnet run --project ServerAdmin.Tests/FractalVerseAdmin.Tests.csproj -c Release`. The preview is a labelled test snapshot, not a live server screenshot.
