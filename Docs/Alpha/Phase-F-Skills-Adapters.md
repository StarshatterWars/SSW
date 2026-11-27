# Phase F: skill commands and closure boundary

27 November 2026 (pilot local date).

## Skill adapter

Skill-book purchases, advanced books, queue-to-level, cancellation, paid instant
training and offline completion use immutable native commands on SSW Economy.
The shared bounded FIFO serializes them with the other economic transactions.
The worker receives copied catalog, progress, queue and wallet data. It never
receives a subsystem, pilot object, live ship, UI or persistence callback.
Training schedule JSON is initialized on the game thread before worker dispatch.

Admission and acceptance validate pilot rank/empire, catalog revision and offline
authority. Book purchases additionally retain their physical craft and dock and
recheck the station service. A dispatched request captures the freshest progress
and queue. Commit compares their original read set and the economic profile and
revision; it merges only skill progress/queue and economic results. Changes to
career, hangar and other pilot fields are preserved. Failed saves roll back the
book entitlement, queue and wallet together. Request receipts prevent replays.
Instant training cannot charge above the accepted quote. Completion is confirmed
in the panel only after saving; closed widgets are not dereferenced by callbacks.

The subsystem checks only the first saved GMT finish time once per second. When
it is due, a native command advances all eligible completed levels. It does not
scan/advance the training queue each frame. Unchanged completion commands do not
save, replace live state or grow receipts. Existing five standard and ten advanced
levels, common duration curve, thirty-day queue and SSW prices remain unchanged.

## Phase F boundary

This closes the planned incremental Phase F migration, subject to the user's
packaged T1 check. It does not finish the entire legacy simulation split. The
native simulation worker currently performs the verified collision broad phase;
flight, combat, AI, initial magazine provisioning, AI replenishment and mission/
progression projection remain serialized live-scene bridges. They invalidate
outstanding economic snapshots. Unreal visualization and persistence remain on
the game thread. No FPS or whole-server-threading gain is claimed.

Nexus REA research remains deferred until after the existing phases. Its supplied
installation is C:/Program Files (x86)/Steam/steamapps/common/Nexus The Jupiter Incident.



## Final verification

Build 20261127-014142Z passed StarshatterWarsEditor and StarshatterWars Win64
Development compilation with zero C++ warning/error diagnostics. All 422 Starshatter
regressions passed with zero test warnings/failures/skips (Saved/Automation/PhaseF-Final-Verified).
All five focused skill domain tests also passed (PhaseF-Skills-Focused). Compiler
logs are PhaseF-Skills-Editor-Verified.log and PhaseF-Skills-Client.log. Own-header
and generated-header ordering checks passed for all 11 changed/new source files.

Tests cover actual off-game-thread skill execution, no early charges, save rollback,
receipt replay, stale account/revision/read-set rejection, preservation of unrelated
career fields, offline completion, standard/advanced/rank/empire/month gates,
accepted instant-training prices and cancellation progress. A unity-build helper
name collision in ownership commerce was corrected and both builds rechecked.
Existing global Editor startup asset/dependency notices are not claimed fixed.

The user performs the cook, package and installed T1 verification using
Launcher/Build-Cook-Deploy.cmd. No installed player save was manually changed.
