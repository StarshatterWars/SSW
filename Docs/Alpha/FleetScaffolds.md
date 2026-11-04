# Fleet authoring scaffolds — 2026-11-04

70 space-entity DEFs are provisioned, including fighters, ships, stations, drones, cargo, farcasters and hulks. Building/SAM/LCA ground definitions remain in the separate terrain stage.

47 missing canonical Blueprints were added; five retain visuals copied from model aliases, and 42 await mesh assignment. Twenty matching native classes were added; existing native classes and existing Blueprint assets were preserved.

Open `/Game/Models/<DEF filename>/BP_<DEF filename>` and assign the mesh to the inherited **HullMesh** component. Keep the existing root/component transforms; no generic replacement scale is imposed. For fighters, use their existing cockpit, gear and hardpoint components. No new meshes are imported.

The runtime, cutscene preloader, briefing preview and tactical reference use the DEF filename catalog. Internal aliases such as IM3500 -> CargoA and IM1906 -> TankerA resolve to those paths. Shared Airbase names are disambiguated by their model. Original DEF data is unchanged.

Verification: UE 5.7 editor build succeeded; 15 automated checks passed, zero failed. Coverage checks load all 70 canonical Blueprints/native classes and verify HullMesh slots, aliases and fighter inheritance. Fighter attachment, briefing-preview and campaign data/presentation checks passed. Mesh appearance and full flight/cinematic playback remain to be tested after mesh assignment.

| DEF filename | Internal name | Blueprint | Mesh work | Native class |
|---|---|---|---|---|
| Adventure | Adventure | /Game/Models/Adventure/BP_Adventure | Assign HullMesh | AdventureShipActor |
| Archaelith | Archaelith | /Game/Models/Archaelith/BP_Archaelith | Assign HullMesh | ArchaelithShipActor |
| Asher | Asher | /Game/Models/Asher/BP_Asher | Existing asset preserved | AsherShipActor |
| Avenger | Avenger | /Game/Models/Avenger/BP_Avenger | Existing asset preserved | AvengerShipActor |
| Baikal | Baikal | /Game/Models/Baikal/BP_Baikal | Existing asset preserved | BaikalShipActor |
| Berents | Berents | /Game/Models/Berents/BP_Berents | Existing asset preserved | BerentsShipActor |
| Broadsword | Broadsword | /Game/Models/Broadsword/BP_Broadsword | Assign HullMesh | BroadswordShipActor |
| CameraPod | CameraPod | /Game/Models/CameraPod/BP_CameraPod | Existing asset preserved | CameraPodShipActor |
| CargoA | IM3500 | /Game/Models/CargoA/BP_CargoA | Existing visual retained | CargoAShipActor |
| CargoB | IM4300 | /Game/Models/CargoB/BP_CargoB | Existing visual retained | CargoBShipActor |
| CargoC | IM2800 | /Game/Models/CargoC/BP_CargoC | Existing visual retained | CargoCShipActor |
| Cobra | Cobra | /Game/Models/Cobra/BP_Cobra | Existing asset preserved | CobraShipActor |
| Colony | Colony | /Game/Models/Colony/BP_Colony | Assign HullMesh | ColonyShipActor |
| Courageous | Courageous | /Game/Models/Courageous/BP_Courageous | Existing asset preserved | CourageousShipActor |
| Courier | Courier | /Game/Models/Courier/BP_Courier | Existing asset preserved | CourierShipActor |
| Cutlass | Cutlass | /Game/Models/Cutlass/BP_Cutlass | Assign HullMesh | CutlassShipActor |
| Devastator | Devastator | /Game/Models/Devastator/BP_Devastator | Existing asset preserved | DevastatorShipActor |
| Dragon | Dragon | /Game/Models/Dragon/BP_Dragon | Assign HullMesh | DragonShipActor |
| Eagle | Eagle | /Game/Models/Eagle/BP_Eagle | Existing asset preserved | EagleShipActor |
| Falcon | Falcon | /Game/Models/Falcon/BP_Falcon | Existing asset preserved | FalconShipActor |
| Farcaster | Farcaster | /Game/Models/Farcaster/BP_Farcaster | Existing asset preserved | FarcasterShipActor |
| Farcaster2 | Marakan | /Game/Models/Farcaster2/BP_Farcaster2 | Assign HullMesh | Farcaster2ShipActor |
| FoothillBase | Airbase2 | /Game/Models/FoothillBase/BP_FoothillBase | Assign HullMesh | FoothillBaseShipActor |
| Fortress | Station3 | /Game/Models/Fortress/BP_Fortress | Assign HullMesh | FortressShipActor |
| FreightXfer | Station1 | /Game/Models/FreightXfer/BP_FreightXfer | Existing visual retained | FreightXferShipActor |
| Goliath | Goliath | /Game/Models/Goliath/BP_Goliath | Existing asset preserved | GoliathShipActor |
| Harmony | Station2 | /Game/Models/Harmony/BP_Harmony | Assign HullMesh | HarmonyShipActor |
| Hulk-Broadsword | Hulk-Broadsword | /Game/Models/Hulk-Broadsword/BP_Hulk-Broadsword | Assign HullMesh | HulkBroadswordShipActor |
| Hulk-Dragon | Hulk-Dragon | /Game/Models/Hulk-Dragon/BP_Hulk-Dragon | Assign HullMesh | HulkDragonShipActor |
| Hulk-Imperial | Hulk-Imperial | /Game/Models/Hulk-Imperial/BP_Hulk-Imperial | Assign HullMesh | HulkImperialShipActor |
| Hulk-Tiger | Hulk-Tiger | /Game/Models/Hulk-Tiger/BP_Hulk-Tiger | Assign HullMesh | HulkTigerShipActor |
| Hulk-Vendetta | Hulk-Vendetta | /Game/Models/Hulk-Vendetta/BP_Hulk-Vendetta | Assign HullMesh | HulkVendettaShipActor |
| Hulk-Volnaris | Hulk-Volnaris | /Game/Models/Hulk-Volnaris/BP_Hulk-Volnaris | Assign HullMesh | HulkVolnarisShipActor |
| Imperial | Imperial | /Game/Models/Imperial/BP_Imperial | Assign HullMesh | ImperialShipActor |
| Intruder | Intruder | /Game/Models/Intruder/BP_Intruder | Existing asset preserved | IntruderShipActor |
| JanekStation | Airbase1 | /Game/Models/JanekStation/BP_JanekStation | Assign HullMesh | JanekStationShipActor |
| Marauder | Marauder | /Game/Models/Marauder/BP_Marauder | Assign HullMesh | MarauderShipActor |
| Mine1 | BlockadeMine | /Game/Models/Mine1/BP_Mine1 | Existing visual retained | Mine1ShipActor |
| Observatory | Observatory | /Game/Models/Observatory/BP_Observatory | Existing asset preserved | ObservatoryShipActor |
| Orion | Orion | /Game/Models/Orion/BP_Orion | Existing asset preserved | OrionShipActor |
| QShip | QShip | /Game/Models/QShip/BP_QShip | Assign HullMesh | QShipShipActor |
| RadixBase | Airbase2 | /Game/Models/RadixBase/BP_RadixBase | Assign HullMesh | RadixBaseShipActor |
| Raptor | Raptor | /Game/Models/Raptor/BP_Raptor | Existing asset preserved | RaptorShipActor |
| Razor | Razor | /Game/Models/Razor/BP_Razor | Existing asset preserved | RazorShipActor |
| Relay1 | Relay1 | /Game/Models/Relay1/BP_Relay1 | Assign HullMesh | Relay1ShipActor |
| S5 | Station5 | /Game/Models/S5/BP_S5 | Assign HullMesh | S5ShipActor |
| S6 | Station6 | /Game/Models/S6/BP_S6 | Assign HullMesh | S6ShipActor |
| S7 | Station7 | /Game/Models/S7/BP_S7 | Assign HullMesh | S7ShipActor |
| Saber | Saber | /Game/Models/Saber/BP_Saber | Assign HullMesh | SaberShipActor |
| Spectre | Spectre | /Game/Models/Spectre/BP_Spectre | Assign HullMesh | SpectreShipActor |
| Stormhawk | Stormhawk | /Game/Models/Stormhawk/BP_Stormhawk | Existing asset preserved | StormhawkShipActor |
| TanisBase | Airbase3 | /Game/Models/TanisBase/BP_TanisBase | Assign HullMesh | TanisBaseShipActor |
| TankerA | IM1906 | /Game/Models/TankerA/BP_TankerA | Assign HullMesh | TankerAShipActor |
| TankerB | IM1912 | /Game/Models/TankerB/BP_TankerB | Assign HullMesh | TankerBShipActor |
| TargetDrone | TargetDrone | /Game/Models/TargetDrone/BP_TargetDrone | Existing asset preserved | TargetDroneShipActor |
| TelmeraStarport | Airbase1 | /Game/Models/TelmeraStarport/BP_TelmeraStarport | Assign HullMesh | TelmeraStarportShipActor |
| Thunderbolt | Thunderbolt | /Game/Models/Thunderbolt/BP_Thunderbolt | Existing asset preserved | ThunderboltShipActor |
| Tiger | Tiger | /Game/Models/Tiger/BP_Tiger | Assign HullMesh | TigerShipActor |
| TigerBayBase | Airbase2 | /Game/Models/TigerBayBase/BP_TigerBayBase | Assign HullMesh | TigerBayBaseShipActor |
| Transport | Transport | /Game/Models/Transport/BP_Transport | Assign HullMesh | TransportShipActor |
| Vendetta | Vendetta | /Game/Models/Vendetta/BP_Vendetta | Assign HullMesh | VendettaShipActor |
| Viper | Viper | /Game/Models/Viper/BP_Viper | Existing asset preserved | ViperShipActor |
| Volnaris | Volnaris | /Game/Models/Volnaris/BP_Volnaris | Assign HullMesh | VolnarisShipActor |
| Wolf | Wolf | /Game/Models/Wolf/BP_Wolf | Existing asset preserved | WolfShipActor |
| ZZA | ZZA | /Game/Models/ZZA/BP_ZZA | Assign HullMesh | ZZAShipActor |
| ZZCA | ZZCA | /Game/Models/ZZCA/BP_ZZCA | Assign HullMesh | ZZCAShipActor |
| ZZCS | ZZCS | /Game/Models/ZZCS/BP_ZZCS | Assign HullMesh | ZZCSShipActor |
| ZZDD | ZZDD | /Game/Models/ZZDD/BP_ZZDD | Assign HullMesh | ZZDDShipActor |
| ZZF | ZZF | /Game/Models/ZZF/BP_ZZF | Assign HullMesh | ZZFShipActor |
| ZZFF | ZZFF | /Game/Models/ZZFF/BP_ZZFF | Assign HullMesh | ZZFFShipActor |
