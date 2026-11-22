# Ship and fighter skins

The `UI.ShipSkinPopupClass` registry entry accepts a Blueprint derived from `UShipSkinDlg` (`Display/ShipSkinDlg.h`). The in-flight Player Logbook has a paintbrush icon opening the standard draggable, stackable Ship Skins panel. Missing registration uses the native driver; the owner can register/reparent the authored widget normally.

`Content/GameData/Ships/skins.json` is the JSON-only collection catalog. Each stable ID maps a compatible ship design and a skin name in the existing `ships.json` material-slot definitions. The 24 existing finishes are included. Eight new finish families provide 40 hull bindings across Falcon, Stormhawk, Zodiac, Prospector and Crew Shuttle, using 24 new material instances. Purchased unlocks belong to the pilot's `OwnedShipSkins`. Flat category prices are defined in JSON: Normal 5,000 cr, Special 15,000 cr and Event 25,000 cr. Special and Event finishes are Rare. Credit purchases require docking and record a wallet receipt; no real-money payments are implemented.

The collection supports search, owned-craft filtering, selected/applied state, isolated material-instance preview, Apply and Reset. Applying requires an owned, surviving, compatible craft at the current dock and local simulation authority. In-flight and network sessions are read only. Failed saves restore the previous selection. `FPlayerHangarShip.SelectedSkin` and pilot unlocks are additive UE SaveGame fields. Free-flight provisioning restores the skin; story/campaign assignments retain authored craft/skins. No stats, hardware, damage or shared materials change.

## Reverse-engineering evidence

REA inspected the user's local `C:/CCP/EVE Online/tq/code.ccp` ZIP (SHA-256 `328170f160fca4d355e70912e1b1ceda7781ac38416074a03c1ddf9af5e28581`). Container inspection establishes archive membership, not running game behavior. Its Python 2.7 modules were read statically with the local cross-version bytecode reader because REA's PYC provider supports newer versions.

Evidence is under `Saved/Analysis/EveShipSkins`: controller.pyj (83 functions), skinPanel.pyj (100), and character-sheet skinsPanel.pyj (29), with hashes and disassembly. Observed client names distinguish licensed/unlicensed selection, previewed/applied/pending state, applying skins, failure rollback and selection layout. SSW implements its own C++ UI and persistence; server internals were not recovered.

Official references: https://support.eveonline.com/hc/en-us/articles/203280701-Ship-SKINs and https://developers.eveonline.com/docs/guides/skinr/ .
