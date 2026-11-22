# Integrated graphics presentation

The LIGHT preset retains high hull textures and 8x anisotropic filtering, with TAA at 90% render scale and a 30 FPS target. UI remains native resolution. Lumen GI and expensive reflections are disabled; ambient skylight stays at full strength. Motion blur, depth of field and chromatic aberration are disabled at medium post-processing quality, with restrained bloom and sharpening.

The anti-aliasing method is selectable (Off, FXAA, TAA, TSR), saved per device and restored by the video subsystem. Main and in-flight options share the video page. These settings affect presentation only.

Hardware performance on integrated GPUs has not been measured. This is a quality-focused baseline, not a guaranteed frame-rate claim. If GPU-bound, lower render scale or shadow quality before reducing texture quality.

Validation (2026-11-21): Windows Development client and Editor builds passed. Full Starshatter automation: 351 tests, zero failures; one test succeeded with an external engine warning. Report: Saved/Automation/GraphicsAndPricingFinal/index.json. No integrated-GPU frame-rate claim has been validated.

Follow-up validation (2026-11-21): disabled the unused web-backed Unreal Editor Home screen via HomeScreen.EnableHomeScreen=0 in DefaultEngine.ini. This prevents its Google generate_204 connectivity probe; HTTP and EOS remain enabled. Full regression report Saved/Automation/NoHomeScreenNetworkRegression/index.json: 351 succeeded, zero failed, zero tests with warnings. No generate_204 requests appeared anywhere in the run log.
