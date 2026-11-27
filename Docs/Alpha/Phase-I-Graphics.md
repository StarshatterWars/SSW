# Phase I: graphics policy and measurement

Five native presets are available: Integrated Graphics, Low, Medium, High and Ultra, with Custom retained for individual settings. Output is at least 1920x1080; reduced internal 3D resolution leaves UI output sharp. Integrated defaults to 75% internal rendering, 30 FPS cap, no Lumen GI/reflections and reduced effects. Presets change presentation budgets only, never simulation population, sensor range or OOB membership. Existing higher output resolutions remain available.

Use `-SSWGraphicsPreset=0` through `4` for a repeatable startup preset. Command-line policy takes precedence over saved visual-budget selection. High and Ultra currently differ in foliage quality; Ultra does not silently enable cinematic settings.

Opt-in capture: `-SSWPerfCapture -SSWPerfSeconds=60 -SSWPerfWarmup=10 -SSWPerfScenario=OrionLaunch`. Captures go to Saved/Profiling/ClientPerformance-*.json. Record the same pilot, scene, camera, resolution and flight actions when comparing builds. Repeat separately for Orion launch and combat. The scenario label does not automate flight. Do not run compiles or other benchmarks concurrently.

Captured measurements include observed frame, game-thread, render-thread and available GPU timings; process RAM, texture allocation, actor count and native worker task averages. Texture allocation is not total VRAM use. Missing GPU timing is reported as unavailable. `-SSWPerfEditorWorld` is an explicit instrumentation-test option; editor/harness numbers are not packaged flight performance.

30 FPS on integrated graphics, 60 FPS on mainstream hardware and the memory targets remain acceptance goals, not measured results. Flight worker dispatch remains opt-in pending evidence of a benefit. The previous offscreen game-start profiling attempt did not reach a valid capture and supplies no FPS evidence.

Verification: Editor and Windows Development client build 20261127-073023Z passed; 449 tests passed, zero test warnings/failures/skips (Saved/Automation/Phase-HI-Final). The opt-in Editor harness capture produced 193 samples on the Intel Iris Xe adapter. This validates recorder operation, not gameplay targets.
