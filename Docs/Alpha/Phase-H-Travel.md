# Phase H: travel presentation

The native travel actor builds a bounded, non-colliding tunnel with the installed Procedural Vortex Tunnel pack's generator and plasma material. Departure is orange/yellow; arrival is blue with green accents. Quantum camera presentation follows the existing envelope and resets on camera/mode changes. Farcaster presentation uses the same actor, attached without inheriting an extra hull scale.

The simulation retains destination selection, arrival, travel duration and AI routing. The loading cover remains when the destination is unavailable; cosmetic geometry cannot complete travel. This pass does not lengthen the simulation journey or alter existing controls.

The actor does not tick. Generation occurs once, synchronously, with bounded segments, avoiding the pack's asynchronous owner-lifetime path. Repeated configuration does not append geometry. Material references are native hard references for cooking.

The pack supplies Windows/Mac support; Linux retains existing effects without a dependency on an unsupported module. Rendered appearance and packaged transitions require the next cooked build. Automated tests verify the real material parameters, bounded geometry, visibility and teardown, plus existing player/AI travel regressions.

Verification: Editor and Windows Development client build 20261127-073023Z passed. All 449 Starshatter regressions passed with threaded flight enabled, zero test warnings/failures/skips (Saved/Automation/Phase-HI-Final). The pack-specific test uses its actual generator and material. Next user cook supplies packaged visual verification.
