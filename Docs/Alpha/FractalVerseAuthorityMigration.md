# FractalVerse authority migration

Started 20 November 2026 after the owner committed and tested Alpha 5 preparation build 20261120-073846Z.

## First slice

- `FFractalVerseAuthority` owns a native world independently of pilot saves, levels, actors and mission lifetimes.
- Validated cargo commands require trusted server access, docking, ownership, available stock, capacity and the current revision. Clients cannot supply an owner or docking authorization.
- Successful commands retain an immutable request fingerprint and receipt in the same checkpoint as inventory. An identical retry succeeds without another transfer; changed or cross-account reuse fails.
- Two checkpoint payload slots and committed indexes retain a previous generation. Hash verification happens before save-game deserialization. Incomplete indexes are ignored. An invalid latest payload falls back to the previous coherent generation. Existing unreadable worlds are not replaced by defaults.
- The runtime owns an exclusive OS-backed writer handle for its save directory. Persistence failure suspends stepping and cargo acceptance until saving recovers.
- Existing mining, production and freight controllers advance the server world without a mission. The freight graph comes from native galaxy systems, authored sectors, facilities and OOB endpoints. No display ships are required for abstract delivery.
- UE owns the connection and RPC transport. A dedicated controller sends a bounded status read model to its owning local client. No public session registration, arena lobby or EOS advertisement is used.

## Explicit limits

This is an opt-in foundation, not the completed Phase 5 migration. Normal Start Game still uses the verified career path. Use `-SSWFractalVerseServer` or `-SSWFractalVerseClient` for the isolated path; the FractalVerse server target selects the server role automatically. Existing StarServer behavior is unchanged.

The first networked projection contains status only, not the full economy or flight scene. Cargo RPCs deliberately reject requests until a trusted server pilot/ship/docking binding is implemented. Core cargo validation is exercised by native tests. No client-supplied inventory, owner, wallet, price or dock flag is imported into the authoritative world.

Authenticated pilot import and ownership, filtered inventory/market projections, UI command migration, flight and mission authority, launcher lifecycle, reconnect/retry UX and source-built dedicated-server packaging remain open. The existing pilot saves must not be silently replaced by the new universe. Custom 256x256 avatars are a separate planned Phase 5 task.

The first checkpoint is synchronous and intended as a correctness gate; staged major-area persistence and asynchronous serialization must be measured before scale-up. The receipt store currently stops accepting new cargo requests at 100,000 entries instead of evicting retry protection. This needs an archival policy for long-running servers.

## Verification

- `Starshatter.FractalVerse.Commands.OwnershipRevisionAndRetry`
- `Starshatter.FractalVerse.Persistence.InterruptionAndRecovery`
- `Starshatter.FractalVerse.Simulation.WithoutPlayerOrLevel`
- `Tools/FractalVerse/smoke_local_transport.py`: editor-hosted dedicated server plus separate receive-only client, with an isolated save directory; checks increasing confirmed revisions and a graceful server checkpoint.

The editor-hosted transport check does not substitute for building and testing the FractalVerse dedicated executable with UE 5.7.4. Both network peers must use compatible engine/network versions.

Transport follows Unreal's owned server/client RPC model: https://dev.epicgames.com/documentation/en-us/unreal-engine/remote-procedure-calls-in-unreal-engine

## Owner decisions added during the first slice

- Universe time is strictly server-owned. GMT initializes the calendar with the existing 2228 epoch, a 200-year shift and a minimum start year of 2228; the save retains both universe elapsed seconds and the server UTC anchor. Restart advances that clock from elapsed server UTC; it never rewinds on a backward wall-clock adjustment. Simulation deltas, travel speed and client clocks cannot alter it. Clients show confirmed snapshots without extrapolating time locally. Native orbital time uses the same server snapshot.
- Player pause and travel acceleration are disabled. Escape opens a live menu without pausing the world; travel-speed mappings and campaign acceleration controls are retired. Internal loading/teardown presentation gates remain distinct from player controls.
- FractalVerse admits at most eight clients, including a game-session cap and a PreLogin guard. EOS discovery/authentication will use the same cap when migrated. Arena caps remain separate.
- Main Menu gets an online badge and a server-status popup; the Player panel gets a Server tab. They expose live players, ping, uptime, revision, protocol, checkpoint availability and universe GMT, and stop claiming online when heartbeats expire.
- Launcher gets the same server indication and a Server tab. A configured healthy permanent server takes priority at startup; otherwise the launcher starts the dedicated local FractalVerse executable. It never substitutes StarServer. Once attached to a permanent universe, a disconnect must not silently create a different local world. Local authority persists beyond launcher closure; updates request a checkpoint and graceful shutdown before replacing files. Automatic game connection and complete pilot/session binding remain separate open gates.

All relevant mutable C++ game data belongs to the server. The UE client is an input and presentation layer: it may own camera state, interpolation, UI and visual effects, but cannot grant itself credits, cargo, ships, progression, mission outcomes, damage or time. This boundary must be verified on every migrated path before replacing the old career runtime.

## Dedicated executable verification — 20 November 2026

The actual source-built FractalVerse target passed the isolated transport test in `Tools/FractalVerse/smoke_dedicated_transport.py`: one separate editor client received revisions 22 and 24, an unauthenticated transfer was rejected, the server exited with code 0 and recovered the same universe at revision 25. Evidence: `Saved/Analysis/Phase5DedicatedTransportSmoke.json`. This is a receive-only transport test, not authenticated gameplay.

Use `/Game/Maps/Transition` for the packaged server; Unreal Entry was not included in the existing cook. Engine pre-exit saves before UObject shutdown; a late teardown cannot allocate save objects after UObject shutdown. Failed-map and normal restart probes exited without the previous assertion.

The source binary reported network compatible changelist 0, although its `Engine/Build/Build.version` declares 47537391, matching the installed client. Launcher `universeServer.compatibleChangelist` defaults to that verified value using Unreal's supported `networkversionoverride` setting. Engine protocol 43 and game-version checksum checks remain enabled; do not replace compatibility checks with an always-true override. Revalidate this value when changing engine versions.

## Identity and pilot boundary (in progress)

A controller submits an EOS Connect ID token, not a claimed balance, rank or profile. Only the EOS verification callback may bind its principal. The verified product, sandbox, deployment and product user ID must match; device-only identities are not accepted. Proofs are never logged or saved. One controller owns a bound pilot at a time; account views go only to that owning controller. Cargo access still requires server-owned ship and docking context and remains disabled until that context is migrated.

New server pilot records have account-specific wallet keys, Janus/Janek home defaults, a single 50,000-credit opening journal entry, and checkpoint durability before acknowledgement. Existing local career files are not uploaded, imported or overwritten by this first identity slice. Full progression/hangar migration, account logout/revocation, offline admission and live signed-in EOS verification remain required before changing normal Start Game.

Identity foundation validation: `Phase5IdentityFocused` completed six tests with zero warnings/failures. Both editor and dedicated builds passed. The final actual-server smoke (`FractalVerseDedicatedSmoke-20261120-203011`) rejected malformed identity and cargo requests, delivered increasing revisions, and recovered its universe after clean shutdown. A real signed-in EOS token has not yet been tested; the original career boot remains unchanged.

## Legacy pilot migration preparation

`FractalVersePilotMigration` now reads a server-selected account save through a read-only slot source, or directly from a trusted physical save directory. Physical reads deliberately avoid the development cooked-content sandbox. It validates the profile index, exact checkpoint slot names and all four same-generation areas, respects newer pilot revisions, and can explicitly recover a complete previous generation. An interrupted player transaction, an invalid only checkpoint, unsupported save version or changing source files blocks preparation. Sources are never repaired or overwritten by this reader.

The candidate retains the full source pilot/economy and mission-world snapshot for subsequent migration. Its separate career JSON preserves reflected progression, hangar/loadouts, skills, journals and settings while excluding world economy, traffic and legacy insurance-credit storage. Credits retain their original value (including zero); the existing one-time insurance-balance migration rule is respected without granting starter funds.

Staging creates an immutable content-addressed backup with verified source bytes, individual file digests and metadata committed last. Retrying identical preparation is idempotent; an existing damaged backup is not overwritten. The digest is an integrity check over trusted local files, not remote identity authentication.

`BindVerifiedPilot` now requires explicit `-FractalVerseAllowNewPilot` to create a new career through the prototype path. Without that opt-in, an unknown account must await migration; sign-in must not silently replace an old career with a new 50,000-credit wallet. Existing local career boot is unchanged.

Verified EOS metadata now selects the legacy slot server-side. Atomic promotion remaps account-owned cargo, reservations, contracts, jobs, blueprint proofs and receipts without copying the source NPC economy. Save failure rolls back the promotion before acknowledgement. No client RPC accepts a source path, slot or profile upload. A live Epic connection confirmed a 425,000-credit pilot using isolated copies of the installed saves; the original 17 files passed hash verification. Evidence: Saved/Analysis/Phase5LivePilotVerified.json.


## Ship-control migration (in progress)

FractalVerse now has a server-only ship-binding API, owner-scoped input RPCs, sequence validation, finite axis/throttle bounds, a 250 ms dead-man timeout and native object-lifetime observation. Client payloads cannot assign a ship or supply position, velocity, integrity or damage. Logout and identity expiry revoke control. Private ship views are read from server simulation objects.

Arena and FractalVerse share SSWNetworkShipControls for flight, primary fire, targeting and sensors. FractalVerse loads the same runtime-remapped single-player input contexts, not a separate keyboard layout. Ship physics, weapon cadence, projectile hits and damage continue to use SSW. Presentation, mission provisioning and persistent ship ownership still need integration before this transport can replace the career game; passing boundary tests does not establish playable flight readiness.

Verification: both installed-editor and source FractalVerse builds succeeded. All 13 final authority tests passed without warnings. The 37 selected authority/weapon/recovery cases pass when the two Niagara activation checks run with rendering enabled; legacy fixture/asset warnings remain. The actual dedicated executable passed separate-client transport and same-universe restart at revision25. These tests do not provision or fly a career ship. Evidence: Saved/Analysis/Phase5SharedFlightProgress.json.


## Provisioning and view integration (20 November, verification in progress)

FractalVerseController now derives from the existing network flight controller, reusing the original runtime-remapped single-player controls, streamed system scene, chase camera, visual ship actors, sensors, target MFD and miniHUD. All inherited server RPC entry points check the persistent controller's verified identity; client-supplied display names cannot overwrite it. GameMode now derives from AGameMode to match the reused AGameState. World startup uses spectator/no-pawn staging rather than a fake UE physics pawn.

The server selects the assigned or selected hangar hull from its own career JSON. It never invents a replacement for a missing/lost selection. Native PlayerShips records retain identity, position, orientation, velocity, equipment/loadout and damage condition in the existing world checkpoint. Separate resident and control maps allow reconnecting to the same live object; retirement/deletion revokes the control lease. A native mission context hosts the world without editing T1 definitions.

The isolated Epic-authenticated flight probe is development-only and does not bypass authentication. It requests ordinary flight/fire commands and measures the resulting native movement and replicated hit/damage state against a test drone. Do not record a successful flight until the live probe actually passes. Initial live startup exposed an AGameModeBase/AGameState incompatibility; it is corrected in the current rebuild. Engine/view-only assets are skipped on the dedicated server.


## Native hull bounds validation (2026-11-20)

All 115 ship-design rows now have `CollisionRadius` in simulation metres. `SSWDesignIngestion -MeasureShipHullBounds` spawns each resolved authored Blueprint in an isolated preview world, measures its hull mesh with the complete component/actor scale hierarchy, writes an audit report, updates `ships.json`, and refreshes `DT_ShipDesign`. It does not tick missions or change weapon balance. The native design registry uses this radius without constructing a server-side presentation actor. A zero value remains a backward-compatible fallback for older custom definitions.

`Saved/Analysis/Phase5MeasuredHullBounds.json` records each class, mesh, center, extent and radius. All non-collision catalog fields were checked unchanged after measurement. The full rendered regression passed 333 of 334 checks; the new bounds test exposed an isolated catalog setup dependency, now corrected. Its final focused rerun passed all four checks with zero warnings/failures, including all 115 native radii and JSON/table parity. The dedicated rebuild and authenticated live damage/reconnect test remain the final integration gate for this increment.
