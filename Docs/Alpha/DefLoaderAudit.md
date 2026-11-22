# DEF loader cleanup — 21 November 2026

## Removed from the current port

- `Design/ShipDesign.cpp`: native DEF constructor, catalog scanning, mod fallback scanning, duplicated component parsers and legacy skin-file loader. Runtime design lookup and editor dropdown lists now use `ShipDesignRegistry`, populated by the JSON ingestion subsystem.
- `Design/WeaponDesign.cpp`: unused DEF entry point and parser; lookup and enumeration use `WeaponDesignRegistry`.
- `System/SSWBootSubsystem.cpp`: design boot already calls subsystem JSON loaders; no new DEF boot calls were introduced.
- `Sim/SimShot.cpp`: old bolt/sprite render allocation and flicker updates. Native shot motion, lifetime and hit calculation remain in the simulation. Solid-model radius support remains for authored collision geometry.
- `Ship/QuantumFlash.cpp/.h`: superseded legacy flash mesh/texture renderer. Quantum explosion timing and UE effects remain.

## Retained deliberately

- `System/StarshatterShipDesignSubsystem::LoadShipDesign` and `System/StarshatterWeaponDesignSubsystem::LoadWeaponDesign`: explicitly invoked by editor migration commands and DEF/JSON parity tests, not the normal JSON boot path.
- `Design/NativeSkinDefinition`: editor-only metadata recovery when requested by conversion tools. Normal JSON ship loading disables recovery.
- Campaign header, editor conversion and custom mission parsing: still have callers. These are not proven dead code.
- Galaxy catalog: `GameData/Galaxy/galaxy.json` is authoritative and includes all 93 region rows alongside 21 systems. The deprecated main Galaxy.def ingestor and its parser helpers have been removed. Galaxy/region tables, registry bindings and runtime table hydration remain for existing consumers. JSON validation completes before either table is populated; no automatic asset save is performed. Ground terrain now comes from the shared JSON catalog; see the final cleanup below.

## HUD assets

`DataLoader::LoadHUDTexture` binds imported `/Game/UI/HUD` UTexture2D assets directly, including packaged/cooked assets. It does not decode external PNG/PCX files or read texture pixels back to the CPU. The legacy bitmap decoder remains for explicit custom-content compatibility.

## Validation

See the current build/test logs under `Saved/Logs/ClientCookReadyEditorCleanBuild.log`, `Saved/Logs/ClientCookReadyGameCleanBuild.log` and `Saved/Automation/ClientCookReadyRegression`. The full rendered suite passed 340 tests with zero fixture warnings. A focused options check follows the final keyboard focus API update. The owner performs the client cook and package.


## Final JSON cleanup and cook gate — 21 November 2026

- Carrier, orbital-station and Janek ground career entry use JSON scene definitions. Training departure restrictions remain agent-controlled.
- All five campaign headers now use `Campaigns/01..05/campaign.json`, merged with separate JSON missions, templates, actions and zones. Existing campaign/galaxy DataTable bridges remain for current consumers.
- 195 authored missions and templates (including cinematics) were exported and passed complete reflected-field DEF/JSON round-trip parity.
- Runtime standalone missions, campaign scenes, templates, story imports and OOB initialization enumerate/read JSON. Missing JSON no longer silently invokes built-in DEF parsing.
- `StarSystem::Load` and its unused per-system star/planet/moon/region/terrain DEF parsers were removed. Ground profiles now come from `Galaxy/terrain_regions.json` for all nine ground sites. Janek runway/site values are retained.
- `Campaign::Load` now uses JSON for built-in and custom campaign directories and JSON mission catalogs. The duplicated legacy campaign header/list/action parsers were removed; native strategic planning remains unchanged. Owned JSON definitions stay alive with the campaign to avoid dangling mission-data references.
- Mod discovery, overrides and editing require JSON. The mod editor's live-snapshot conversion may use an internal temporary legacy serialization, then emits JSON; it is not a DEF mod runtime path. DAT archive container support remains, with JSON mission payloads.
- `Explosion::Initialize` no longer attempts to read the unshipped `Explosions.def`. Existing zeroed optional legacy sprite/particle configuration is preserved; native UE effects provide presentation.
- Space Explosion Pack presentation receives one 1:100 scale conversion. Separately calibrated muzzle flashes and quantum effects are excluded. Simulation hit, damage and blast calculations are untouched.

### Explicitly retained

The subsystem DEF readers used by migration commandlets and legacy/JSON parity tests remain. `FreightManifest::LoadProfile` and legacy skin metadata recovery are migration helpers. Mission serialization/editor compatibility is not claimed removed. Source DEF files are retained as references; no cooked assets or registry bridges were deleted.

### Deferred presentation

A shared ready-room will eventually replace empty docked-bay views. Hyper weather/biome integration awaits the imported pack; JSON profile metadata is present, not a completed pack integration.

### Current validation

Final build and regression results are recorded under `Saved/Logs/DefDependencyCleanupEditor.log`, `Saved/Logs/DefDependencyCleanupClient.log` and `Saved/Automation/DefDependencyCleanupRegression`. Client cooking and packaging remain with the owner.


### Award/promotion popup follow-up

Awards and promotion use a centered, viewport-bounded dark window, the shared title-bar styling, a geometric red close X, draggable positioning and aspect-fitted insignia. Closing while already in live play dismisses only the popup. The old campaign screen's classes/assets are retained until the owner completes T1–T4 and verifies Campaign 2 acceptance through an agent.
