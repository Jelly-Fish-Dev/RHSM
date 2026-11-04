"""Generate app/icon.png for the Onion Apps list.

Run once on the host (needs Pillow):  python3 scripts/make_icon.py
The generated PNG is committed, so the build itself does not need Python.
"""
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parent.parent
SIZE = 80
SCALE = 4  # draw large, then downsample for smooth edges

big = SIZE * SCALE
img = Image.new("RGBA", (big, big), (0, 0, 0, 0))
d = ImageDraw.Draw(img)

d.rounded_rectangle((0, 0, big - 1, big - 1), radius=18 * SCALE, fill=(34, 37, 52, 255))
# "save slot" stripe
d.rounded_rectangle((12 * SCALE, 52 * SCALE, 68 * SCALE, 62 * SCALE),
                    radius=4 * SCALE, fill=(255, 196, 60, 255))

font = ImageFont.truetype(str(ROOT / "res/fonts/Exo2-Bold.ttf"), 26 * SCALE)
d.text((big // 2, 30 * SCALE), "RHSM", font=font, fill=(240, 240, 245, 255), anchor="mm")

out = ROOT / "app/icon.png"
img.resize((SIZE, SIZE), Image.LANCZOS).save(out, optimize=True)
print(f"wrote {out}")
