from pathlib import Path
from PIL import Image

# -----------------------------
# Settings
# -----------------------------
INPUT_DIR = Path.cwd()
OUTPUT_DIR = INPUT_DIR / "cleaned"

BLACK_THRESHOLD = 12
SOFT_EDGE = True


def remove_black_to_alpha(img: Image.Image, threshold: int, soft_edge: bool) -> Image.Image:
    img = img.convert("RGBA")
    pixels = img.load()
    width, height = img.size

    for y in range(height):
        for x in range(width):
            r, g, b, a = pixels[x, y]

            max_rgb = max(r, g, b)

            if max_rgb <= threshold:
                pixels[x, y] = (r, g, b, 0)
            elif soft_edge and max_rgb <= threshold * 2:
                fade = int(255 * (max_rgb - threshold) / max(1, threshold))
                pixels[x, y] = (r, g, b, min(a, fade))

    return img


def main():
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    supported = {".png", ".bmp", ".tga", ".jpg", ".jpeg", ".webp", ".pcx"}

    files = [p for p in INPUT_DIR.iterdir() if p.suffix.lower() in supported]

    if not files:
        print("No supported images found in current directory.")
        return

    for src in files:
        dst = OUTPUT_DIR / (src.stem + ".png")

        try:
            with Image.open(src) as img:
                cleaned = remove_black_to_alpha(img, BLACK_THRESHOLD, SOFT_EDGE)
                cleaned.save(dst)
                print(f"OK  {src.name} -> cleaned/{dst.name}")
        except Exception as e:
            print(f"ERR {src.name}: {e}")

    print("Done.")


if __name__ == "__main__":
    main()