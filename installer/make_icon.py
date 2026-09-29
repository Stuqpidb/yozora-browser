#!/usr/bin/env python3
"""Generates the Yozora application icon (PNG set + Windows .ico).

The icon is drawn here rather than exported from a vector tool so the source of
truth is reviewable in text and the whole set can be regenerated with one
command:

    python installer/make_icon.py

Outputs resources/icons/yozora-<size>.png and installer/yozora.ico.
"""

import math
import pathlib

from PIL import Image, ImageDraw

ROOT = pathlib.Path(__file__).resolve().parent.parent
ICON_DIR = ROOT / "resources" / "icons"

# Yozora night-sky palette, kept in sync with src/core/Theme.cpp
SKY_TOP = (10, 14, 22)
SKY_BOTTOM = (18, 23, 36)
MOON_LIGHT = (207, 224, 255)
MOON_DARK = (110, 168, 254)
STAR = (200, 214, 245)

# Fixed star field: same points every time the icon is generated.
STARS = [
    (0.18, 0.19, 0.010), (0.34, 0.13, 0.007), (0.77, 0.16, 0.009),
    (0.87, 0.34, 0.007), (0.13, 0.41, 0.007), (0.25, 0.77, 0.008),
    (0.74, 0.83, 0.009), (0.49, 0.17, 0.006), (0.83, 0.63, 0.007),
    (0.61, 0.30, 0.005), (0.08, 0.60, 0.006), (0.44, 0.68, 0.005),
]

SIZES = [16, 24, 32, 48, 64, 128, 256]


def draw_icon(size: int) -> Image.Image:
    scale = 4  # supersample, then downscale for clean edges
    s = size * scale
    image = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)

    # Rounded-square background with a vertical gradient.
    radius = int(s * 0.22)
    for y in range(s):
        t = y / max(s - 1, 1)
        color = tuple(
            round(SKY_TOP[i] + (SKY_BOTTOM[i] - SKY_TOP[i]) * t) for i in range(3)
        )
        draw.line([(0, y), (s, y)], fill=color + (255,))

    mask = Image.new("L", (s, s), 0)
    ImageDraw.Draw(mask).rounded_rectangle([0, 0, s - 1, s - 1], radius, fill=255)
    image.putalpha(mask)
    draw = ImageDraw.Draw(image)

    for x, y, r in STARS:
        alpha = 90 if r > 0.007 else 150
        cx, cy, rad = x * s, y * s, max(1, round(r * s))
        draw.ellipse(
            [cx - rad, cy - rad, cx + rad, cy + rad], fill=STAR + (alpha,)
        )

    # Crescent moon: a light disc with an offset disc punched out of it. The
    # punch-out is done with a mask so the background gradient shows through
    # instead of a flat disc of colour.
    moon_r = s * 0.24
    moon_cx, moon_cy = s * 0.5, s * 0.49

    crescent = Image.new("L", (s, s), 0)
    crescent_draw = ImageDraw.Draw(crescent)
    crescent_draw.ellipse(
        [moon_cx - moon_r, moon_cy - moon_r, moon_cx + moon_r, moon_cy + moon_r], fill=255
    )
    cut = moon_r * 0.92
    cut_cx, cut_cy = moon_cx + moon_r * 0.34, moon_cy - moon_r * 0.28
    crescent_draw.ellipse(
        [cut_cx - cut, cut_cy - cut, cut_cx + cut, cut_cy + cut], fill=0
    )

    moon = Image.new("RGBA", (s, s), MOON_LIGHT + (255,))
    arc = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    ImageDraw.Draw(arc).arc(
        [moon_cx - moon_r, moon_cy - moon_r, moon_cx + moon_r, moon_cy + moon_r],
        start=105,
        end=250,
        fill=MOON_DARK + (255,),
        width=max(1, round(s * 0.022)),
    )
    moon.alpha_composite(arc)
    image.paste(moon, (0, 0), crescent)

    return image.resize((size, size), Image.LANCZOS)


def main() -> None:
    ICON_DIR.mkdir(parents=True, exist_ok=True)
    images = [draw_icon(size) for size in SIZES]

    for size, image in zip(SIZES, images):
        image.save(ICON_DIR / f"yozora-{size}.png")
    images[-1].save(ROOT / "resources" / "icons" / "yozora.png")

    ico_path = ROOT / "installer" / "yozora.ico"
    ico_path.parent.mkdir(parents=True, exist_ok=True)
    images[-1].save(ico_path, format="ICO",
                    sizes=[(s, s) for s in SIZES if s <= 256])

    print(f"wrote {len(SIZES)} PNG files to {ICON_DIR}")
    print(f"wrote {ico_path}")


if __name__ == "__main__":
    main()
