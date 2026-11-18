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


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--campaign", type=int, default=1)
    parser.add_argument("--scene", default="Intro")
    parser.add_argument("--all", action="store_true")
    parser.add_argument("--list", action="store_true")
    parser.add_argument("--visible", action="store_true", help="Show the separate renderer window for debugging")
    parser.add_argument("--editor", default=r"C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe")
    parser.add_argument("--ffmpeg")
    parser.add_argument("--fps", type=int, choices=range(24, 61), default=30)
    parser.add_argument("--timeout", type=int, default=600)
    args = parser.parse_args()
    available = scenes()
    if args.list:
        for number, scene, _ in available:
            print(f"{number:02d}: {scene}")
        return
    chosen = available if args.all else [s for s in available if s[0] == args.campaign and s[1].casefold() == args.scene.casefold()]
    if not chosen:
        raise SystemExit("Scene not found; use --list")
    ffmpeg = encoder(args.ffmpeg)
    for number, scene, source in chosen:
        safe = re.sub(r'[^\w. -]', '_', scene)
        run = ROOT / "Saved/CutsceneRenders" / f"Campaign{number:02d}" / safe / time.strftime("%Y%m%d_%H%M%S")
        run.mkdir(parents=True, exist_ok=False)
        command = [args.editor, str(ROOT / "StarshatterWars.uproject"), "-game", "-windowed", "-ResX=1920", "-ResY=1080", "-ForceRes", "-nosplash", "-DisableAllScreenMessages", "-SSWLogging", f"-SSWRenderScene={scene}", f"-SSWRenderCampaign={number}", f"-SSWRenderOutput={run.as_posix()}", f"-SSWRenderFPS={args.fps}", f"-UserDir={(run / 'Profile').as_posix()}/", "-ini:Engine:[OnlineSubsystem]:DefaultPlatformService=NULL", "-ini:Game:[/Script/StarshatterWars.SSWDiagnosticsSettings]:bRequirePlatformSignIn=False", f"-abslog={(run / 'render.log').as_posix()}"]
        if not args.visible:
            command.append("-RenderOffscreen")
        print(f"Recording Campaign {number:02d} / {scene}", flush=True)
        with (run / "process.log").open("w", encoding="utf-8") as output:
            subprocess.run(command, stdout=output, stderr=subprocess.STDOUT, timeout=args.timeout, check=True)
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
        destination = ROOT / "Content/Movies" / f"Campaign{number:02d}" / (safe + ".mp4")
        destination.parent.mkdir(parents=True, exist_ok=True)
        temporary = run / "encoded.mp4"
        subprocess.run([ffmpeg, "-hide_banner", "-loglevel", "warning", "-y", "-f", "concat", "-safe", "0", "-i", str(playlist), "-i", str(audio), "-map", "0:v:0", "-map", "1:a:0", "-c:v", "libx264", "-preset", "medium", "-crf", "18", "-pix_fmt", "yuv420p", "-r", str(rate), "-fps_mode", "cfr", "-c:a", "aac", "-b:a", "192k", "-movflags", "+faststart", "-t", str(duration), str(temporary)], check=True)
        subprocess.run([ffmpeg, "-hide_banner", "-loglevel", "error", "-i", str(temporary), "-f", "null", "-"], check=True)
        temporary.replace(destination)
        metadata.update({"campaign": number, "source_def": str(source), "source_sha256": hashlib.sha256(source.read_bytes()).hexdigest(), "mp4_sha256": hashlib.sha256(destination.read_bytes()).hexdigest(), "captured_frames": len(frames), "render_directory": str(run)})
        destination.with_suffix(".render.json").write_text(json.dumps(metadata, indent=2), encoding="utf-8")
        print(f"Exported {destination} ({duration:.2f}s, {len(frames)} frames)", flush=True)


if __name__ == "__main__":
    main()
