"""Export original SSW cutscenes in an isolated editor-game process, then encode MP4."""
from __future__ import annotations
import argparse
import hashlib
import json
import re
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def scenes():
    found = []
    for folder in sorted((ROOT / "Content/GameData/Campaigns").iterdir()):
        if not folder.name.isdigit():
            continue
        for source in sorted((folder / "Scenes").glob("*.def")):
            text = source.read_text(encoding="utf-8-sig", errors="replace")
            match = re.search(r'^\s*scene\s*:\s*"([^"]+)"', text, re.M | re.I)
            if match and re.search(r'type\s*:\s*"?(?:BeginScene|EndScene)\b', text, re.I):
                found.append((int(folder.name), match[1], source))
    return found


def encoder(explicit):
    if explicit:
        return str(Path(explicit).resolve())
    sys.path.insert(0, str(Path(__file__).parent / "_deps"))
    try:
        import imageio_ffmpeg
        return imageio_ffmpeg.get_ffmpeg_exe()
    except ImportError as exc:
        raise SystemExit("Supply --ffmpeg PATH or install imageio-ffmpeg into Tools/Cutscenes/_deps") from exc


def select_video_encoder(ffmpeg, requested):
    hardware = ["-c:v", "h264_nvenc", "-preset", "p7", "-tune", "hq", "-rc", "vbr", "-cq", "18", "-b:v", "0", "-spatial-aq", "1", "-temporal-aq", "1"]
    if requested != "cpu":
        probe = subprocess.run([ffmpeg, "-hide_banner", "-loglevel", "error", "-f", "lavfi", "-i", "color=black:s=1920x1080:r=30", "-frames:v", "1", *hardware, "-f", "null", "-"], capture_output=True, text=True, timeout=20)
        if probe.returncode == 0:
            return "h264_nvenc", hardware
        if requested == "nvenc":
            raise SystemExit("NVIDIA video encoding could not initialize: " + probe.stderr[:500])
        print("NVIDIA video encoding unavailable; using CPU encoder.", flush=True)
    return "libx264", ["-c:v", "libx264", "-preset", "fast", "-crf", "18"]


def cinematic_filters(duration, scene, notices_suppressed=False):
    filters = "eq=contrast=1.035:brightness=0.003:saturation=1.025,vignette=angle=PI/9"
    if scene.casefold()=="battleattract" and not notices_suppressed:
        filters = "delogo=x=24:y=88:w=470:h=108:show=0," + filters
    if scene.casefold() not in {"intro", "battleattract"}:
        # A small documentary channel bug; safe margins and modest opacity.
        filters += ",drawbox=x=iw-163:y=ih-80:w=125:h=44:color=black@0.3:t=fill,drawtext=fontfile='C\\:/Windows/Fonts/arialbd.ttf':text=ISN:fontsize=30:fontcolor=white@0.72:x=w-147:y=h-73"
    return filters + f",fade=t=in:st=0:d=0.65,fade=t=out:st={max(0,duration-0.75):.3f}:d=0.75"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--campaign", type=int, default=1)
    parser.add_argument("--scene", default="Intro")
    parser.add_argument("--finish-capture", type=Path, help="Re-encode an existing completed capture without running Unreal")
    parser.add_argument("--source", type=Path, help="Standalone scene outside campaign progression")
    parser.add_argument("--movie-output", type=Path)
    parser.add_argument("--all", action="store_true")
    parser.add_argument("--skip-existing", action="store_true", help="Resume only exports from this renderer build and unchanged scene definition")
    parser.add_argument("--continue-on-error", action="store_true", help="Finish other scenes and report all failures")
    parser.add_argument("--list", action="store_true")
    parser.add_argument("--visible", action="store_true", help="Show the separate renderer window for debugging")
    parser.add_argument("--editor", default=r"C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe")
    parser.add_argument("--ffmpeg")
    parser.add_argument("--video-encoder", choices=["auto", "cpu", "nvenc"], default="auto")
    parser.add_argument("--fps", type=int, choices=range(24, 61), default=30)
    parser.add_argument("--timeout", type=int, default=600)
    args = parser.parse_args()
    available = scenes()
    if args.list:
        for number, scene, _ in available:
            print(f"{number:02d}: {scene}")
        return
    chosen = available if args.all else [s for s in available if s[0] == args.campaign and s[1].casefold() == args.scene.casefold()]
    if args.source:
        if args.all: raise SystemExit("--source cannot be combined with --all")
        chosen = [(args.campaign, args.scene, args.source.resolve())]
    if not chosen:
        raise SystemExit("Scene not found; use --list")
    ffmpeg = encoder(args.ffmpeg)
    args.selected_encoder, args.encoder_options = select_video_encoder(ffmpeg, args.video_encoder)
    print("Video encoder: " + args.selected_encoder, flush=True)
    module = ROOT / "Binaries/Win64/UnrealEditor-StarshatterWars.dll"
    build_hash = hashlib.sha256(module.read_bytes() + (ROOT / "Content/GameData/Ships/ships.json").read_bytes() + Path(__file__).read_bytes()).hexdigest()
    results = []
    summary = ROOT / "Saved/CutsceneRenders/export-summary.json"
    for number, scene, source in chosen:
        destination = args.movie_output or ROOT / "Content/Movies" / f"Campaign{number:02d}" / (re.sub(r'[^\w. -]', '_', scene) + ".mp4")
        stamp = destination.with_suffix(".render.json")
        if args.skip_existing and destination.is_file() and stamp.is_file():
            record = json.loads(stamp.read_text(encoding="utf-8-sig"))
            if record.get("render_inputs_sha256") == build_hash and record.get("source_sha256") == hashlib.sha256(source.read_bytes()).hexdigest() and record.get("mp4_sha256") == hashlib.sha256(destination.read_bytes()).hexdigest():
                print(f"Verified existing Campaign {number:02d} / {scene}", flush=True)
                results.append({"campaign":number,"scene":scene,"status":"verified-existing"})
                continue
        try:
            render_scene(args, ffmpeg, number, scene, source, build_hash)
            results.append({"campaign":number,"scene":scene,"status":"exported"})
        except (Exception, SystemExit) as error:
            results.append({"campaign":number,"scene":scene,"status":"failed","error":str(error)})
            print(f"FAILED Campaign {number:02d} / {scene}: {error}", flush=True)
            if not args.continue_on_error:
                summary.parent.mkdir(parents=True,exist_ok=True)
                summary.write_text(json.dumps(results,indent=2),encoding="utf-8")
                raise
        summary.parent.mkdir(parents=True,exist_ok=True)
        summary.write_text(json.dumps(results,indent=2),encoding="utf-8")
    summary.parent.mkdir(parents=True,exist_ok=True)
    summary.write_text(json.dumps(results,indent=2),encoding="utf-8")
    if any(item["status"]=="failed" for item in results):
        raise SystemExit("Some scenes failed; see Saved/CutsceneRenders/export-summary.json")


def render_scene(args, ffmpeg, number, scene, source, build_hash):
    source_hash = hashlib.sha256(source.read_bytes()).hexdigest()
    safe = re.sub(r'[^\w. -]', '_', scene)
    run = args.finish_capture.resolve() if args.finish_capture else ROOT / "Saved/CutsceneRenders" / f"Campaign{number:02d}" / safe / time.strftime("%Y%m%d_%H%M%S")
    if not args.finish_capture: run.mkdir(parents=True, exist_ok=False)
    command = [args.editor, str(ROOT / "StarshatterWars.uproject"), "-game", "-windowed", "-ResX=1920", "-ResY=1080", "-ForceRes", "-nosplash", "-DisableAllScreenMessages", "-SSWLogging", f"-SSWRenderScene={scene}", f"-SSWRenderCampaign={number}", f"-SSWRenderOutput={run.as_posix()}", f"-SSWRenderFPS={args.fps}", f"-UserDir={(run / 'Profile').as_posix()}/", "-ini:Engine:[OnlineSubsystem]:DefaultPlatformService=NULL", "-ini:Game:[/Script/StarshatterWars.SSWDiagnosticsSettings]:bRequirePlatformSignIn=False", f"-abslog={(run / 'render.log').as_posix()}"]
    if args.source:
        command.append(f"-SSWRenderSource={source.as_posix()}")
    if not args.visible:
        command.append("-RenderOffscreen")
    if not args.finish_capture:
        print(f"Recording Campaign {number:02d} / {scene}", flush=True)
        with (run / "process.log").open("w", encoding="utf-8") as output:
            subprocess.run(command, stdout=output, stderr=subprocess.STDOUT, timeout=args.timeout, check=True)
    else:
        print(f"Finishing existing capture {run}", flush=True)
    metadata_path = run / "capture.json"
    if not metadata_path.is_file():
        raise SystemExit(f"Renderer did not complete the original scene. Check {run / 'render.log'}")
    metadata = json.loads(metadata_path.read_text(encoding="utf-8-sig"))
    frames = sorted(run.glob("frame_*.png"))
    audio = run / "audio.wav"
    if len(frames) < args.fps or not audio.is_file() or audio.stat().st_size <= 44:
        raise SystemExit(f"Incomplete picture/audio capture: {run}")
    duration = float(metadata["Duration"])
    rate = int(metadata["FrameRate"])
    frame_numbers = [int(re.search(r'(\d+)\.png$', f.name)[1]) for f in frames]
    # Real-time capture may skip frame numbers. Preserve those elapsed times rather than speeding up the scene.
    lines = ["ffconcat version 1.0"]
    for index, frame in enumerate(frames):
        start = frame_numbers[index] / rate
        end = frame_numbers[index + 1] / rate if index + 1 < len(frames) else duration
        if start >= duration:
            break
        filename = frame.as_posix().replace("'", "'\\''")
        lines += [f"file '{filename}'", f"duration {max(1/rate, end-start):.9f}"]
    lines.append(f"file '{frames[-1].as_posix()}'")
    playlist = run / "frames.ffconcat"
    playlist.write_text("\n".join(lines) + "\n", encoding="utf-8")
    destination = args.movie_output or ROOT / "Content/Movies" / f"Campaign{number:02d}" / (safe + ".mp4")
    destination.parent.mkdir(parents=True, exist_ok=True)
    temporary = run / "encoded.mp4"
    logo = ROOT / "Content/UI/Campaigns/01/StarshatterWars.png"
    inputs, visual, video_map = [], ["-vf", cinematic_filters(duration, scene, metadata.get("ScreenMessagesSuppressed", False))], "0:v:0"
    if scene.casefold() == "intro" and logo.is_file():
        # Match the original authored closing artwork, without stale live HUD captions.
        # Imported UI PNGs can carry non-image padding after IEND. Preserve every
        # image chunk, but keep that padding out of the looping video decoder.
        image = logo.read_bytes()
        offset = 8
        while offset + 12 <= len(image):
            size = int.from_bytes(image[offset:offset+4], "big")
            tag = image[offset+4:offset+8]
            offset += size + 12
            if tag == b"IEND": break
        if offset > len(image) or tag != b"IEND": raise SystemExit("Closing card PNG is incomplete")
        card_image = run / "closing-card.png"
        card_image.write_bytes(image[:offset])
        inputs = ["-f", "image2", "-loop", "1", "-framerate", str(rate), "-i", str(card_image)]
        card_start = max(0, duration - 14)
        graph = f"[0:v]{cinematic_filters(duration,scene)},fade=t=out:st={max(0,duration-16):.3f}:d=1[base];[2:v]scale=1920:1080,format=rgba,fade=t=in:st=0:d=1:alpha=1,setpts=PTS+{card_start:.3f}/TB[card];[base][card]overlay=0:0:enable='gte(t,{card_start:.3f})',fade=t=out:st={max(0,duration-.75):.3f}:d=0.75[finished]"
        visual = ["-filter_complex_threads", "2", "-filter_complex", graph]
        video_map = "[finished]"
    subprocess.run([ffmpeg, "-hide_banner", "-loglevel", "warning", "-y", "-f", "concat", "-safe", "0", "-i", str(playlist), "-i", str(audio), *inputs, *visual, "-map", video_map, "-map", "1:a:0", "-af", f"afade=t=out:st={max(0,duration-0.75):.3f}:d=0.75", *args.encoder_options, "-pix_fmt", "yuv420p", "-r", str(rate), "-fps_mode", "cfr", "-c:a", "aac", "-b:a", "192k", "-movflags", "+faststart", "-t", str(duration), "-stats_period", "2", "-progress", str(run / "encode-progress.txt"), str(temporary)], check=True)
    subprocess.run([ffmpeg, "-hide_banner", "-loglevel", "error", "-i", str(temporary), "-f", "null", "-"], check=True)
    if args.finish_capture and destination.with_suffix(".render.json").is_file():
        original = json.loads(destination.with_suffix(".render.json").read_text(encoding="utf-8-sig"))
        if original.get("render_directory") == str(run):
            source_hash = original.get("source_sha256", source_hash)
            build_hash = original.get("render_inputs_sha256", build_hash)
    temporary.replace(destination)
    if scene.casefold()=="intro" and logo.is_file():
        metadata["clean_closing_card_sha256"]=hashlib.sha256(logo.read_bytes()).hexdigest()
    metadata.update({"cinematic_finish": "contrast, subtle vignette and fades; ISN on documentary clips", "video_encoder": args.selected_encoder, "render_inputs_sha256": build_hash, "campaign": number, "source_def": str(source), "source_sha256": source_hash, "mp4_sha256": hashlib.sha256(destination.read_bytes()).hexdigest(), "captured_frames": len(frames), "render_directory": str(run)})
    destination.with_suffix(".render.json").write_text(json.dumps(metadata, indent=2), encoding="utf-8")
    print(f"Exported {destination} ({duration:.2f}s, {len(frames)} frames)", flush=True)

if __name__ == "__main__":
    main()
