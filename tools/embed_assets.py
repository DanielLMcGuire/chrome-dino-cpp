#!/usr/bin/env python3
import argparse
import io
import os
from PIL import Image

SOUND_ASSETS = [
    ("SND_HIT",   "resources/sounds/hit.wav"),
    ("SND_PRESS", "resources/sounds/button-press.wav"),
    ("SND_SCORE", "resources/sounds/score-reached.wav"),
]

PS2_SPRITE_SHEET_PATH = "resources/images/ps2_100_percent/offline/100-offline-sprite.png"
DEFAULT_SPRITE_SHEET_PATH = "resources/images/default_100_percent/offline/100-offline-sprite.png"
TILE1_END   = 700
TILE2_START = 602

DEFAULT_OUT = "src/ps2/ps2_assets.h"


def parse_args():
    parser = argparse.ArgumentParser(description="Embed console assets into a header.")
    parser.add_argument(
        "--output",
        default=DEFAULT_OUT,
        help="Path to the generated header file. Relative paths are resolved "
             "against the project root (default: %(default)s).",
    )
    parser.add_argument(
        "--pcsx2",
        action="store_true",
        help="Build for the PCSX2 emulator. Uses the higher-quality "
             "default_100_percent sprite sheet instead of the ps2_100_percent one.",
    )
    parser.add_argument(
        "--xbox",
        action="store_true",
        help="Build for original Xbox (nxdk). Uses the higher-quality "
             "default_100_percent sprite sheet and embeds it as a single, "
             "unsplit texture (the NV2A can address up to 2048x2048, well "
             "above the 1233px-wide sprite sheet, so no tiling is needed).",
    )
    return parser.parse_args()


def emit_array(f, name, data):
    f.write(f"alignas(16) inline const unsigned char {name}[] = {{\n")
    for i in range(0, len(data), 20):
        chunk = data[i:i + 20]
        f.write("    " + ",".join(str(b) for b in chunk) + ",\n")
    f.write("};\n")
    f.write(f"inline const unsigned int {name}_len = {len(data)}u;\n\n")


def png_bytes(img):
    buf = io.BytesIO()
    img.save(buf, format="PNG")
    return buf.getvalue()


def main():
    args = parse_args()

    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

    out_path = args.output
    if not os.path.isabs(out_path):
        out_path = os.path.join(root, out_path)
    os.makedirs(os.path.dirname(out_path), exist_ok=True)

    if args.xbox:
        sheet = Image.open(os.path.join(root, DEFAULT_SPRITE_SHEET_PATH)).convert("RGBA")
        with open(out_path, "w") as f:
            f.write("// Auto-generated, do not edit by hand.\n")
            f.write("#pragma once\n\n")
            emit_array(f, "SPRITE_SHEET", png_bytes(sheet))
            for name, relpath in SOUND_ASSETS:
                path = os.path.join(root, relpath)
                with open(path, "rb") as src:
                    data = src.read()
                emit_array(f, name, data)
        return

    sprite_sheet_path = DEFAULT_SPRITE_SHEET_PATH if args.pcsx2 else PS2_SPRITE_SHEET_PATH
    sheet = Image.open(os.path.join(root, sprite_sheet_path)).convert("RGBA")
    w, h = sheet.size
    tile1 = sheet.crop((0, 0, TILE1_END, h))
    tile2 = sheet.crop((TILE2_START, 0, w, h))

    with open(out_path, "w") as f:
        f.write("// Auto-generated, do not edit by hand.\n")
        f.write("#pragma once\n\n")
        f.write(f"constexpr int PS2_SPRITE_TILE_SPLIT = {TILE2_START};\n\n")

        total = 0
        for name, data in (("SPRITE_SHEET_TILE1", png_bytes(tile1)),
                            ("SPRITE_SHEET_TILE2", png_bytes(tile2))):
            emit_array(f, name, data)
            total += len(data)

        for name, relpath in SOUND_ASSETS:
            path = os.path.join(root, relpath)
            with open(path, "rb") as src:
                data = src.read()
            total += len(data)
            emit_array(f, name, data)

if __name__ == "__main__":
    main()