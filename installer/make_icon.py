#!/usr/bin/env python3
"""Generates the Yozora application icon set from the master artwork.

The artwork itself lives in resources/icons/yozora-source.png and is not drawn
here. It used to be: the icon used to be a hand-coded crescent moon, which was
reviewable as text, and this script drew it. The mark is now a real piece of
artwork, and re-drawing it in code would only produce a worse copy of it, so
this script's whole job is to turn that one file into the sizes the product
actually needs:

    python installer/make_icon.py

Outputs:
    resources/icons/yozora-<size>.png   for 16, 24, 32, 48, 64, 128, 256
    resources/icons/yozora.png          the 256px one, the "main" icon
    installer/yozora.ico                Windows: the exe and the NSIS installer
    resources/icons/yozora.svg          an SVG wrapper around the raster

Two things are worth knowing about the result:

  * The master file has a lot of transparent margin around it and the planet is
    off-centre, so the artwork is cropped to its own content and then squared
    up. Cropping without squaring would stretch the rings into ellipses.

  * The SVG is not a vector drawing. Nothing in the project uses it - the
    application reads the PNGs - but it used to be the hand-coded moon, so
    leaving it would have shipped a second, contradictory icon. It is now a
    wrapper that embeds the raster, which keeps the file working for anything
    that expects an .svg while showing the same mark.
"""

import base64
import pathlib

from PIL import Image

ROOT = pathlib.Path(__file__).resolve().parent.parent
ICON_DIR = ROOT / "resources" / "icons"
SOURCE = ICON_DIR / "yozora-source.png"

SIZES = [16, 24, 32, 48, 64, 128, 256]

# The icon sizes every target really uses. The others are there for installers
# and desktop environments that pick their own.
MAIN_SIZE = 256

# The raster size embedded in yozora.svg. See write_svg_wrapper().
SVG_EMBED_SIZE = 128

# Alpha below this is treated as empty when finding the artwork's edges, so the
# soft glow around the planet does not drag the crop outwards.
ALPHA_CUTOFF = 8

# Breathing room between the artwork and the edge of the icon, as a fraction of
# its own size. Windows puts its own padding around a 16px icon, and a mark that
# touches the edge looks cropped.
PADDING = 0.04


def cropped_to_content(image: Image.Image) -> Image.Image:
    """The part of the artwork that is actually visible."""
    alpha = image.getchannel("A")
    mask = alpha.point(lambda value: 255 if value > ALPHA_CUTOFF else 0)
    box = mask.getbbox()
    if box is None:
        raise SystemExit(f"{SOURCE} is fully transparent - nothing to make an icon from")
    return image.crop(box)


def squared(image: Image.Image) -> Image.Image:
    """Fit the artwork into a square with padding, without distorting it."""
    side = max(image.size)
    padded = round(side * (1 + 2 * PADDING))
    canvas = Image.new("RGBA", (padded, padded), (0, 0, 0, 0))
    canvas.alpha_composite(image, ((padded - image.width) // 2,
                                   (padded - image.height) // 2))
    return canvas


def main() -> None:
    if not SOURCE.exists():
        raise SystemExit(
            f"missing {SOURCE}\n"
            "The master artwork has to be in the repository; the icon set is "
            "generated from it."
        )

    ICON_DIR.mkdir(parents=True, exist_ok=True)
    master = squared(cropped_to_content(Image.open(SOURCE).convert("RGBA")))

    # Every size is taken from the master rather than from the 256px one, so a
    # 16px icon is not a downscaled 256px icon.
    for size in SIZES:
        master.resize((size, size), Image.LANCZOS).save(
            ICON_DIR / f"yozora-{size}.png")

    master.resize((MAIN_SIZE, MAIN_SIZE), Image.LANCZOS).save(
        ICON_DIR / "yozora.png")

    # Windows wants one .ico holding every size; the master is handed over at
    # full size so nothing is resampled twice.
    ico_path = ROOT / "installer" / "yozora.ico"
    ico_path.parent.mkdir(parents=True, exist_ok=True)
    master.save(ico_path, format="ICO", sizes=[(s, s) for s in SIZES])

    write_svg_wrapper(master)

    print(f"source:      {SOURCE.relative_to(ROOT)}")
    print(f"squared to:  {master.size[0]}px")
    print(f"wrote {len(SIZES)} PNG files to {ICON_DIR.relative_to(ROOT)}")
    print(f"wrote {ico_path.relative_to(ROOT)}")
    print(f"wrote {(ICON_DIR / 'yozora.svg').relative_to(ROOT)}")


def write_svg_wrapper(master: Image.Image) -> None:
    """An SVG that embeds the raster, for anything that insists on .svg.

    The embed is at 128px rather than 256 on purpose: the file is compiled into
    the executable through the resource bundle, and a full-size PNG turns a
    1 KB placeholder into 340 KB of binary. Nothing in the project uses the SVG,
    so crispness past 128px is not worth that.
    """
    encoded = base64.b64encode(
        master.resize((SVG_EMBED_SIZE, SVG_EMBED_SIZE), Image.LANCZOS)
        .convert("RGBA").tobytes()
    ).decode("ascii")

    svg = (
        '<?xml version="1.0" encoding="UTF-8"?>\n'
        "<!--\n"
        "  The Yozora mark, embedded as a raster.\n"
        "\n"
        "  This file used to be a hand-coded crescent moon that had drifted away\n"
        "  from the real icon. It is kept only so that anything expecting an\n"
        "  .svg keeps working; the application itself uses the PNGs, and this\n"
        f"  file is not referenced by any of them. Embedded at {SVG_EMBED_SIZE}px so it\n"
        "  does not bloat the resource bundle.\n"
        "-->\n"
        f'<svg xmlns="http://www.w3.org/2000/svg" '
        f'xmlns:xlink="http://www.w3.org/1999/xlink" '
        f'viewBox="0 0 {MAIN_SIZE} {MAIN_SIZE}" '
        f'width="{MAIN_SIZE}" height="{MAIN_SIZE}">\n'
        f'  <image width="{MAIN_SIZE}" height="{MAIN_SIZE}" '
        f'xlink:href="data:image/png;base64,{encoded}"/>\n'
        "</svg>\n"
    )
    (ICON_DIR / "yozora.svg").write_text(svg, encoding="utf-8")


if __name__ == "__main__":
    main()
