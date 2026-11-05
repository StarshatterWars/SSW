from PIL import Image
import sys
import os

def convert_pcx_to_png(input_path, threshold=0):
    img = Image.open(input_path).convert("RGBA")
    pixels = img.load()
    w, h = img.size

    for y in range(h):
        for x in range(w):
            r, g, b, a = pixels[x, y]
            # black (or near-black if threshold > 0) becomes transparent
            if r <= threshold and g <= threshold and b <= threshold:
                pixels[x, y] = (0, 0, 0, 0)
            else:
                pixels[x, y] = (r, g, b, 255)

    out_path = os.path.splitext(input_path)[0] + ".png"
    img.save(out_path)
    print("Converted:", out_path)

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: py pcx_to_png_black_transparent.py <file.pcx> [threshold]")
        print("Example: py pcx_to_png_black_transparent.py button.pcx 8")
        raise SystemExit(1)

    pcx = sys.argv[1]
    thr = int(sys.argv[2]) if len(sys.argv) >= 3 else 0
    convert_pcx_to_png(pcx, threshold=thr)