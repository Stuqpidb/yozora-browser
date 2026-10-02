#!/usr/bin/env python3
"""Builds the bundled font set from the upstream variable fonts.

Yozora ships its own typefaces so the interface never falls back to whatever
the system happens to have. The upstream releases are variable fonts; Windows
only ever picks one instance out of them, so the weights the interface actually
uses are baked into static files here. Keeping the step in a script means the
whole set can be regenerated from a clean checkout:

    python installer/make_fonts.py

Outputs resources/fonts/*.ttf and the licence texts in LICENSES/. Requires
fontTools (pip install fonttools). The upstream sources are SIL Open Font
License 1.1; each family gets its own OFL text in LICENSES/, because the licence
requires the copyright notice of the family that owns the files to travel with
them, and Inter's notice says nothing at all about Space Grotesk.
"""

import pathlib
import sys
import urllib.request

from fontTools.ttLib import TTFont
from fontTools.varLib import instancer

ROOT = pathlib.Path(__file__).resolve().parent.parent
FONT_DIR = ROOT / "resources" / "fonts"
# LICENSES/ is the single source of truth for every licence text the project
# ships: CMake installs the directory into the build, so a missing or wrong text
# here is a compliance bug in the installer, not just a documentation gap.
LICENCE_DIR = ROOT / "LICENSES"
# The upstream variable fonts are an input, not an output: they are cached
# outside the tree so only the static instances end up in the repository.
CACHE_DIR = ROOT / "build" / "fontcache"

# name -> (upstream file, {output name: weight})
SOURCES = {
    "Inter.ttf": (
        "https://raw.githubusercontent.com/google/fonts/main/ofl/inter/"
        "Inter%5Bopsz%2Cwght%5D.ttf",
        {
            "Inter-Regular.ttf": 400,
            "Inter-Medium.ttf": 500,
            "Inter-SemiBold.ttf": 600,
            "Inter-Bold.ttf": 700,
        },
    ),
    "SpaceGrotesk.ttf": (
        "https://raw.githubusercontent.com/google/fonts/main/ofl/spacegrotesk/"
        "SpaceGrotesk%5Bwght%5D.ttf",
        {
            "SpaceGrotesk-Medium.ttf": 500,
            "SpaceGrotesk-Bold.ttf": 700,
        },
    ),
}

# One OFL text per family, keyed by the file name it lands in under LICENSES/.
LICENCES = {
    "Inter-OFL-1.1.txt":
        "https://raw.githubusercontent.com/google/fonts/main/ofl/inter/OFL.txt",
    "SpaceGrotesk-OFL-1.1.txt":
        "https://raw.githubusercontent.com/google/fonts/main/ofl/"
        "spacegrotesk/OFL.txt",
}


def fetch(url: str, target: pathlib.Path) -> None:
    if target.exists() and target.stat().st_size > 0:
        return
    print(f"downloading {target.name}")
    with urllib.request.urlopen(url) as response:
        target.write_bytes(response.read())


def main() -> int:
    FONT_DIR.mkdir(parents=True, exist_ok=True)
    CACHE_DIR.mkdir(parents=True, exist_ok=True)
    LICENCE_DIR.mkdir(parents=True, exist_ok=True)
    for source_name, (url, weights) in SOURCES.items():
        source = CACHE_DIR / source_name
        fetch(url, source)
        for output_name, weight in weights.items():
            target = FONT_DIR / output_name
            font = TTFont(source)
            # Pin every axis: a leftover "opsz" axis would make the instance
            # depend on the point size at which it is loaded.
            axes = {axis.axisTag: axis.defaultValue for axis in font["fvar"].axes}
            axes["wght"] = weight
            static = instancer.instantiateVariableFont(font, axes, inplace=True)
            # OS/2.usWeightClass is what Qt matches a request for
            # QFont::SemiBold (600) against.
            static["OS/2"].usWeightClass = weight
            static["name"].setName(
                f"Yozora {output_name[:-4].replace('-', ' ')}", 4, 3, 1, 0x409
            )
            static.save(target)
            print(f"{target.name}: {target.stat().st_size} bytes")

    for name, url in LICENCES.items():
        fetch(url, CACHE_DIR / name)
        target = LICENCE_DIR / name
        text = (CACHE_DIR / name).read_bytes()
        target.write_bytes(text)
        holder = text.decode("utf-8", "replace").splitlines()[0].strip()
        print(f"{target.relative_to(ROOT)}: {holder}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
