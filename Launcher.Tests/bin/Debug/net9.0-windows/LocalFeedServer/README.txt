Starshatter Wars local release-feed server

The launcher installer includes a minimal Apache HTTP Server runtime staged by
New-LocalFeedServerBundle.ps1. Apache's LICENSE.txt, NOTICE.txt and distribution
readme remain beside the binary. bundle-manifest.json records source and hashes.

Apache starts only when localFeedServer.enabled is true and a configured feed
uses HTTP on 127.0.0.1 with the configured port (default 18787). Hosted HTTPS
feeds do not start it. It is not installed as a Windows service and needs no
elevation. It listens only on loopback and serves static release files.

The signed game manifest, ZIP and individual patch files stay in a separate
release-feed directory; they are not embedded in the launcher MSI. Example:

  D:\SSW\Build\LauncherFeed\current\manifest.json
  D:\SSW\Build\LauncherFeed\current\manifest.json.sig
  D:\SSW\Build\LauncherFeed\current\feed.json
  D:\SSW\Build\LauncherFeed\current\StarshatterWars.zip
  D:\SSW\Build\LauncherFeed\current\files\...

All configured download URLs then begin http://127.0.0.1:18787/current/ .
The launcher creates its own Apache configuration and logs under the user's
LocalAppData folder. No XAMPP configuration or process is changed. The owned
server is hidden, health-checked and terminated when the launcher exits; a
Windows job also terminates it if the launcher process is killed.

A port conflict is reported instead of terminating or adopting another server.
Missing local release files keep news readable, but installation, updating and
Play still require the signed current release. Version checks are not bypassed.
