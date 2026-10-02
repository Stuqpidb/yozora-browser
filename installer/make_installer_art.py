#!/usr/bin/env python3
"""Generates the NSIS installer banners from the application artwork.

The installer is a stock NSIS "Modern UI" wizard, and its default bitmaps are
the grey Windows-era placeholders. This script replaces them with banners built
from the real Yozora mark and the bundled typefaces, so the first thing someone
sees when installing the browser already looks like the browser:

    python installer/make_installer_art.py

Outputs (BMP, because NSIS cannot read PNG for these):

    resources/installer/welcome.bmp        164x314, welcome + finish pages
    resources/installer/welcome-uninstall.bmp   the same, uninstaller side
    resources/installer/header.bmp         150x57, top-right of inner pages

The sizes are fixed by NSIS: the welcome/finish bitmap has to be exactly
164x314 and the header exactly 150x57. The images are committed, the same way
the static font instances are, so a checkout can build the installer without
running this step; run it again after the mark or the typefaces change.
"""

import pathlib
import random

from PIL import Image, ImageDraw, ImageFilter, ImageFont

ROOT = pathlib.Path(__file__).resolve().parent.parent
SOURCE = ROOT / "resources" / "icons" / "yozora-source.png"
FONT_DIR = ROOT / "resources" / "fonts"
OUT_DIR = ROOT / "resources" / "installer"

# Fixed by the NSIS Modern UI.
WELCOME_SIZE = (164, 314)
HEADER_SIZE = (150, 57)

# Night-sky palette, matching the start page.
SKY_TOP = (9, 13, 40)
SKY_BOTTOM = (27, 34, 72)
HEADER_TOP = (250, 251, 255)
HEADER_BOTTOM = (233, 237, 250)

# Alpha below this is treated as empty when cropping the artwork to its content.
ALPHA_CUTOFF = 8


def vertical_gradient(size, top, bottom):
    """A plain two-stop vertical gradient as an RGB image."""
    width, height = size
    image = Image.new("RGB", size)
    pixels = image.load()
    for y in range(height):
        t = y / (height - 1)
        row = tuple(round(top[i] + (bottom[i] - top[i]) * t) for i in range(3))
        for x in range(width):
            pixels[x, y] = row
    return image


def cropped_to_content(image):
    """The part of the artwork that is actually visible."""
    alpha = image.getchannel("A")
    mask = alpha.point(lambda value: 255 if value > ALPHA_CUTOFF else 0)
    box = mask.getbbox()
    if box is None:
        raise SystemExit(f"{SOURCE} is fully transparent")
    return image.crop(box)


def squared(image):
    """Fit the artwork into a square transparent canvas, without distorting it."""
    side = max(image.size)
    canvas = Image.new("RGBA", (side, side), (0, 0, 0, 0))
    canvas.alpha_composite(image, ((side - image.width) // 2,
                                   (side - image.height) // 2))
    return canvas


def star_field(draw, size, count, seed):
    """Small deterministic stars, so rebuilding does not reshuffle them."""
    rng = random.Random(seed)
    width, height = size
    for _ in range(count):
        x = rng.randint(0, width - 1)
        y = rng.randint(0, height - 1)
        radius = rng.choice([0, 0, 0, 1])
        alpha = rng.randint(40, 190)
        draw.ellipse([x - radius, y - radius, x + radius, y + radius],
                     fill=(255, 255, 255, alpha))


def load_font(name, size):
    return ImageFont.truetype(str(FONT_DIR / name), size)


def welcome_banner(seed):
    """The tall 164x314 banner shown beside the welcome and finish pages."""
    image = vertical_gradient(WELCOME_SIZE, SKY_TOP, SKY_BOTTOM).convert("RGBA")

    star_field(ImageDraw.Draw(image), WELCOME_SIZE, 110, seed)

    mark = squared(cropped_to_content(Image.open(SOURCE).convert("RGBA")))
    mark = mark.resize((132, 132), Image.LANCZOS)

    mark_x = (WELCOME_SIZE[0] - mark.width) // 2
    mark_y = 40

    # A soft, widened copy of the mark behind it gives the planet a glow
    # instead of pasting a hard-edged sticker onto the sky.
    glow = mark.filter(ImageFilter.GaussianBlur(9))
    image.alpha_composite(glow, (mark_x, mark_y))
    image.alpha_composite(mark, (mark_x, mark_y))

    draw = ImageDraw.Draw(image)
    wordmark = load_font("SpaceGrotesk-Bold.ttf", 34)
    tagline = load_font("Inter-SemiBold.ttf", 12)

    draw.text((WELCOME_SIZE[0] // 2, 214), "Yozora", font=wordmark,
              fill=(255, 255, 255, 255), anchor="mm")
    draw.text((WELCOME_SIZE[0] // 2, 244), "NIGHT SKY BROWSER",
              font=tagline, fill=(150, 162, 220, 255), anchor="mm")

    return image.convert("RGB")


def header_banner():
    """The 150x57 strip shown at the top-right of the inner pages."""
    image = vertical_gradient(HEADER_SIZE, HEADER_TOP, HEADER_BOTTOM).convert("RGBA")

    mark = squared(cropped_to_content(Image.open(SOURCE).convert("RGBA")))
    mark = mark.resize((HEADER_SIZE[1] - 12, HEADER_SIZE[1] - 12), Image.LANCZOS)
    image.alpha_composite(mark, (6, 6))

    draw = ImageDraw.Draw(image)
    wordmark = load_font("SpaceGrotesk-Bold.ttf", 22)
    draw.text((mark.width + 12, HEADER_SIZE[1] // 2 + 1), "Yozora", font=wordmark,
              fill=(31, 36, 76, 255), anchor="lm")

    return image.convert("RGB")


def main():
    OUT_DIR.mkdir(parents=True, exist_ok=True)

    welcome = welcome_banner(seed=17)
    welcome.save(OUT_DIR / "welcome.bmp", format="BMP")
    welcome.save(OUT_DIR / "welcome-uninstall.bmp", format="BMP")
    header_banner().save(OUT_DIR / "header.bmp", format="BMP")

    for name in ("welcome.bmp", "welcome-uninstall.bmp", "header.bmp"):
        path = OUT_DIR / name
        with Image.open(path) as check:
            print(f"{path.relative_to(ROOT)}: {check.size[0]}x{check.size[1]} "
                  f"{check.mode}")


if __name__ == "__main__":
    main()
