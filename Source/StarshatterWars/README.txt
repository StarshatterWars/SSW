STARSHATTER WARS — FOUR-WAY MISSIONS PANEL DRIVER (SOURCE INTEGRATION)

CONTENTS
- Mission/MissionsPanelDriver.cpp/.h : native 4-category entry screen
- Screen/MenuScreen.cpp/.h          : mission driver and four routing methods
- Mission/SingleMissionDlg.cpp/.h   : 4 navigation tabs, standalone handoff
- Mission/MultiplayerDlg.cpp/.h     : existing multiplayer roster route
- Mission/MissionBriefingDlg.cpp/.h : standalone mission context and PROCEED diagnostics

Place these COMPLETE files under D:\SSW\Source\StarshatterWars preserving folder names.
Do NOT delete/change CampaignMissionDlg.cpp/.h or CustomMissionDlg.cpp/.h; retain your working versions.
The new driver is native C++ (no Blueprint needs to be created). Existing Blueprint
mission selection, campaign selection, custom mission editor and briefing classes remain configured.

ROUTING
- Existing ShowMissionSelectDlg() opens four-way driver
- CAMPAIGN -> ShowCampaignSelectDlg (campaign choice)
- SINGLE -> ShowSingleMissionDlg (existing Single roster)
- CUSTOM -> ShowCustomMissionDlg (custom roster/editor)
- MULTIPLAYER -> ShowMultiplayerMissionDlg (current multiplayer roster)
- Existing SINGLE/CUSTOM pages retain navigation; BACK routes to driver.

BRIEFING
- Uses standalone mission model where supported, without changing the story campaign.
- Existing MissionBriefing PROCEED is connected to OnCommit(). It requires a hydrated
  legacy Mission AND an authored map at /Game/Maps/<SystemName>.
- OnCommit now displays explicit error messages for missing mission/viewport/map
  instead of only silently returning. It DOES NOT generate a missing map or mission.
- If a DataTable mission has no hydrated legacy model, the roster reports an error.

NOTE
- This is a substantial restoration based on the last locally available hub/briefing
  source files. UnrealBuildTool / runtime could not be executed in this environment.
- Test four tabs and one valid hydrated Single Mission before replacing more components.
- Preserve a backup of your working source before extracting.


EMPTY DRIVER FIX (Oct 27):
- Corrected an early return in MissionsPanelDriver.cpp. The old code saw the existing
  MissionsDriver_Root canvas and skipped constructing the entire interface.
- Now checks for MissionsDriver_Contents, the actual generated UI.
- Supports an existing CanvasRoot in a Blueprint and also a native canvas root.
- Calls BuildDriverLayout from NativeConstruct and Show (idempotent).
- No registry change needed. This is a complete replacement bundle.
