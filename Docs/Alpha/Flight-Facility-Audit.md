# Flight facilities and external-berth guidance â€” 27 November 2026

## Runtime changes

`Content/GameData/Ships/flight_facilities.json` supplies explicit angle units, coordinate scale and complete optional deck replacements. The 116 original ship rows and their table/DEF authoring remain unchanged. Only the runtime copy is normalized by ShipDesignRegistry. Unknown mod designs retain the native radians/already-scaled contract. Replacement coordinates are already in simulation metres and angles in radians; replacements are not rescaled. This file stages with the existing GameData UFS rule.

All 23 carrier/station/starbase designs have a policy. Twenty provide launch and recovery, totaling 61 decks and 88 launch-slot/recovery centerlines. Colony, Marakan Lab (Station7) and Mining Station remain docking-only by explicit user direction. Wreck/hulk variants are not operational facilities.

The old authoring mixes raw definition-space deck coordinates with newer scaled fixes. Explicit scale restores Adventure, Dragon, Goliath, Fortress, freight transfer, Harmony, Observatory, S5, S6 and the older airbase aliases without rescaling Orion, S4, the revised named ground bases, asteroid base, processing facility or shipyard. Slot compatibility masks, cycles and capacity remain authored. Native collision rays now transform direction with component scale, preserving the world ray parameter under uniform and non-uniform scale.

Specific corrections:

- Dragon launch slots clear its center divider. Recovery approaches the actual asymmetric side bay, rather than the opposite side and central hull.
- Goliath's old paths intersect its enclosed hull. Launch and recovery now use measured clear exposed upper-hull deck positions, with recovery approaching from the side. This is an open deck layout, not a claim that a new enclosed bay has been modeled.
- Fortress's third recovery leg follows its radial bay heading and ends at the parking slot. Harmony's recovery height clears the lip and its second bay has a unique label.
- Observatory launch follows the clear forward corridor; its recovery heading follows the side approach. Marakan transfer station's second recovery heading is reversed to match its inbound path.
- Asteroid base retains its original approach/departure axis and door/interior placement. Its outer shell is rotated to align the shell opening with that assembly; child component world transforms are preserved and repeated construction does not accumulate rotation. Door components are not included in the primary-hull ray inventory.
- Processing Facility and the existing placeholder Shipyard now receive their authored actor scale before binding, so navigation/collision sizing observes the same final mesh scale as the corrected decks. No new shipyard model is imported.

## External berths

`FExternalBerthGuidance` is a native value-only corridor around the exact endpoint returned by Ship::GetTransferBerth. Four oriented hoops lead toward that endpoint. Both upper/lower berths follow station position and orientation and allow for craft/hull clearance. Station-class docking-only structures can expose the existing external-berth geometry without receiving flight decks.

With ILS enabled, the HUD previews the nearest berth of a selected permitted station. Assigned recovery decks retain priority. Fighters/shuttles retain normal bay previews where flight decks exist; larger craft or docking-only stations use the external preview. Region membership, station survival and faction permission are checked before projection. UE only projects/draws the native coordinates.

This is approach guidance, not a new external-docking clearance, occupancy or station-service transaction system. It does not mark a craft docked merely for reaching a graphic. Existing transfers still use the same final berth coordinate.

## Validation boundary

Native automation spawns all authored facility Blueprints and checks every launch slot/recovery centerline against the cooked primary hull using the production collision adapter. A separate policy test verifies coverage, units, masks, recovery heading alignment, docking-only exceptions and retention of already corrected Orion coordinates. Scaled-ray tests cover rotation, translation, uniform/non-uniform scale and non-unit direction parameters. External-berth tests verify exact transfer endpoints, both berths, rotated frames, four hoop planes and invalid inputs.

Centerline coverage is not full swept-aircraft clearance or a manual flight certification. Compound hulls, moving doors, detailed parking stance and every aircraft size still need packaged visual/gameplay checks. Geometry export is opt-in (`-SSWBayMeshExport`) and writes evidence only beneath Saved/Analysis. No real pilot save is modified.

Build **20261127-050721Z**: Editor and Windows Development client builds passed with zero C++ diagnostics. All **442 regressions passed** with **zero test warnings, failures or skips**, independently in both configurations:

- `Saved/Automation/Facilities-Full-Default`
- `Saved/Automation/Facilities-Full-Worker` (`-SSWThreadedFlight`)

Focused facility tests also passed (3/3). Include-order checks passed for all 15 changed native files; `git diff --check` and `Tools/Checks/check_flight_facilities.py` passed. `Launcher/Build-Cook-Deploy.ps1 -PlanOnly` passed; no cook, package, deployment or real pilot save modification was performed.

The optional physics worker remains disabled by default because its dispatch cost was slower in the measured bridge benchmark. The validated serial controls/flight remain the normal gameplay path. See Flight-Physics-Worker.md for the separate timing evidence and remaining controller/AI/combat boundaries.

## Deferred assets

The Babylon 5 shipyard/Orion station and EA transport model research/imports are deferred until after F, G and H, per user direction. No model or texture from that collection is added in this checkpoint.
