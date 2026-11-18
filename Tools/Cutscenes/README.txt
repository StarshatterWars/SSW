Offline cutscene export

Run from D:\SSW:
  python Tools\Cutscenes\render_cutscenes.py --campaign 1 --scene Intro
  python Tools\Cutscenes\render_cutscenes.py --list
  python Tools\Cutscenes\render_cutscenes.py --all --skip-existing --continue-on-error

Requires the built StarshatterWarsEditor module and the installed Unreal Editor.
The exporter launches an isolated offscreen game process, plays the original
campaign scene and records its picture and master audio. It does not use or
change the player's normal save. Output is a 1920x1080 H.264/AAC MP4 under
Content\Movies\CampaignXX. Intel replays these files; briefing rehearsal remains
interactive 3D. Export videos before packaging so they are staged with the game.

Each MP4 is decoded for verification before publishing. Its .render.json records
the source definition hash, video hash, duration, frame count and capture folder.
Intermediate PNGs and WAV are kept in Saved\CutsceneRenders for inspection.
--visible shows the separate renderer for troubleshooting.

The first validated export is Campaign01\Intro. Other scenes must be exported
and visually checked before claiming their Intel replay is available.

Batch exports write Saved/CutsceneRenders/export-summary.json. Failures are
reported individually and cause a nonzero final exit code. Resume validates the
video hash, scene definition hash and combined renderer/ship-catalog hash;
omit --skip-existing to force regeneration after other visual asset changes.
The encoder uses H.264 CRF 18, fast preset, AAC 192 kbit/s and web fast-start.

The exporter probes the NVIDIA H.264 encoder and uses it when available, with
quality target 18 and the high-quality preset. Otherwise it uses CPU H.264 CRF18.
Use --video-encoder cpu or --video-encoder nvenc to select explicitly.
Per-scene encode-progress.txt records encoding progress.
