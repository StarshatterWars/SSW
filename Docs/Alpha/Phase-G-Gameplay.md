# Phase G gameplay work

## Verified changes

- Free Flight resolves Ensign from the rank catalog instead of assuming rank 2; a missing catalog cannot unlock Cadet. The active pilot progress, including all four training completions, is used by launch clearance. Authored flight dispatchers have the correct Ensign minimum. Agent acceptance/start remains mandatory.
- Player Logbook reports current system from the active simulation region, separately from the saved home system and sector. Agent location logic is unchanged.
- Non-fighter player Autonav route following uses the existing elapsed-time cruise controller rather than the old 50-percent throttle branch. Fighter stall protection and Auto Track behavior remain separate. Existing physics augmented-speed cap is retained.
- Combat-debug damage credits a gun kill only on the live-to-destroyed integrity edge and rejects further damage to dying/destroyed contacts. Victim destruction and hostile points are recorded through mission statistics.
- All four recorded training lessons now set the campaign completion bit within the debrief transaction. The existing campaign-one medal ID/artwork is retained for save compatibility and named Basic Flight Training. Stats evaluation can then grant it; the debrief snapshot rolls the completion bit back if persistence fails.
- Ship Store offers a panel-size expanded viewer with an independent preview component, existing drag/wheel controls, reset and close. Hull/skin selection updates both previews. Preview controls never call commerce or grant ownership.

## Existing behavior retained

The combined linear/arcade augmented-speed cap and per-frame camera reset already have regression coverage. Explosion-pack presentation already applies a single 0.01 conversion independently of damage radii; repeating activation does not compound scale. Thunderbolt runtime engine placement uses the common measured hull projection/nozzle-radius path; its legacy authoring offsets alone are not final world positions. Final fleet coverage and visual assessment are still required before declaring the exhaust issue resolved.

The Campaign 2 Orion launch/Meridian startup recovery code is not rewritten.

## Initial checkpoint evidence

Before the subsequent viewer/debug/award/cruise changes, Editor build succeeded and all 444 tests passed with zero test warnings/failures/skips (Saved/Automation/Phase-G-Initial). This is not validation of the later changes. See Background-Economy-Validation.md for the accelerated six-hour worker test and its limits.

## Final checkpoint

Editor and Windows Development client builds passed without C++ diagnostics. All 447 regressions passed with threaded flight forced on, zero test warnings/failures/skips (Saved/Automation/Startup-G-Verified). The opt-in fleet hull-triangle propulsion audit passed separately (Saved/Automation/Phase-G-FleetGeometry), including Thunderbolt engine outlet placement. Earlier normal-path checkpoint passed 444 tests. This is native/Editor integration coverage, not a claim of a rendered FPS gain or a complete packaged flight review.

The monolithic startup failure was traced to FCommandLine::Get during the global CVarThreadedFlight initializer. Default registration is now constant zero and the launch override is applied at first runtime use after command-line initialization. A freshly linked executable with existing cooked content and an isolated UserDir initialized successfully and exited on request with code 0 (Launcher-Startup-Smoke2.log). No installed game executable or pilot save was replaced.

Launcher 1.0.19 passes 42 checks, detects early process exit and no longer treats a running StarServer as proof that the client started. Its MSI was built and installed successfully. Build-Cook-Deploy now performs an isolated packaged startup before replacing Build/Windows or publishing. The installed old game still requires the forthcoming full cook/package; only its launcher was updated.
