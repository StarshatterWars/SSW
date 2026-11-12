# Legacy parity follow-up

Baseline: Stars45 under D:/GameDev/starshatter-open-master and the SSW source in this workspace. The November 8 audit covers simulation fixtures. This pass focuses on the code that players reach from the current Unreal screens and on features the previous audit excluded.

| Feature | Legacy behavior | SSW coverage and remaining verification |
|---|---|---|
| Cockpit and flight HUD | Tactical contact tracks, directional cockpit views, velocity marker, navigation and ILS modes, mission clock and flight phase | Native implementations exist. Fly T2 to verify target trails and drift cues, then T3 to verify recovery cues. Verify camera switching across cockpit, atmospheric climb and space. Legacy HUD palette switching and the capital-ship ordered-helm marker still need an active-HUD parity check. |
| Mods | ModConfig discovers packages, enables and disables them, and controls precedence for data overlays | The Options Mods page now enables installed loose mission packs. It reads missions at runtime through the existing parser, without modifying cooked tables. Legacy DAT mounting, global replacement priority, custom asset packages, and campaign packs remain open. |
| Multiplayer pilot records | ShipStats collects sortie combat and points. The dedicated server runs the same simulation. | The UE dedicated lobby now writes server-derived sortie totals for pilots with a platform ID. Test reconnect, respawn, distinct accounts sharing a display name, and persistence across server restarts. Profile migration and campaign progression synchronization remain open. |
| Pilot career | Pilot identity, rank, medals, points, training/campaign progress, flight time, and settings | Local profile persistence exists in PlayerCharacter and the UE player subsystem. Run a fresh career across progression milestones and reloads. Match statistics alone are insufficient evidence for awarding campaign medals or rank. |
| Navigation and travel | Sector maps, plotted routes, quantum and farcaster transfer, tactical grid and recovery | UE paths exist. Verify repeated transfers and interrupted Auto Nav, with separate checks for region ownership, arrival camera, and flight state. |
| Campaign and mission authoring | Dynamic planners, mission editors, campaign continuity, mission event scripts and debrief | Current code supports the principal flow. Run campaign-wide scripted and dynamic missions, editor round trips, and all campaign cutscene/skip/return paths to establish behavioral parity. |
| Data and asset overrides | Legacy data loader can resolve layered loose/archive data | Native UE assets are cooked and the boot-time DEF import has several specialized entry points. A general mod format needs explicit content-type contracts and compatible cooked asset packaging before global overrides can be offered. |

## Install a mission pack

Create `Saved/Mods/<Pack>/Missions/` under the game's active saved-data directory and place standalone `MISSION` DEF files in it. Use existing SSW designs and regions. Open **Options → Mods**, enable the pack, then open **Custom Missions**. Enabled packs contribute selectable scenarios and their source files are read-only from the mission roster. Options shows the actual saved-data path for the running installation.

## StarServer pilot data

Dedicated server records are under `Saved/StarServer/Pilots`. A hashed platform namespace and ID identify each file. The server saves connections and last seen time, then records flight time, points, collisions, kills, deaths and weapon accuracy inputs from the authoritative simulation at logout or world shutdown. Local campaign progress, rank, medals, taunts and preferences have no upload or merge path in this change.
