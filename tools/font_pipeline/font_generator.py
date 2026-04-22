"""
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
"""

import os
import sys
import json
import struct
import subprocess

from symbol_merger import generate_symbols_with_fallback

MSDF_EXE = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "msdf-atlas-gen.exe")
FONTS_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "fonts")

FONT_MAP = {
    "latin_cyrillic": [("latin_cyrillic", "NotoSans-Medium.ttf")],
    # symbols uses multi-font fallback: see SYMBOLS_FONT_PRIORITY
    "symbols":        [("symbols", "NotoSansSymbols2-Regular.ttf")],
    "cjk": [
        ("cjk_jp", "NotoSansCJKjp-Medium.otf"),
        ("cjk_sc", "NotoSansCJKsc-Medium.otf")
    ]
}

# Fonts tried in order for the symbols atlas.
# Chars are distributed: primary first, then remaining go to secondary, then tertiary.
SYMBOLS_FONT_PRIORITY = [
    "NotoSansSymbols2-Regular.ttf",  # Primary: broad symbol coverage
    "NotoSansCJK-Regular.ttc",       # Secondary: CJK + some extra symbols
    "arial.ttf"                      # Tertiary: final fallback
]

def run_msdf_gen(charset_file, font_file, out_png, out_json, pxrange="2", dimensions=None):
    """
    Calls msdf-atlas-gen to generate SDF atlas and JSON metadata.
    Uses -yorigin top to match standard PSP GU UV coordinates (0,0 at top-left).
    """
    cmd = [
        MSDF_EXE,
        "-font", font_file,
        "-charset", charset_file,
        "-type", "sdf",       # Standard SDF
        "-format", "png",
        "-imageout", out_png,
        "-json", out_json,
        "-pxrange", str(pxrange),
        "-size", "24",        # High resolution target
        "-yorigin", "top"
    ]
    if dimensions:
        cmd += ["-dimensions", str(dimensions[0]), str(dimensions[1])]
    else:
        # Default to PSP max texture size
        cmd += ["-dimensions", "512", "512"]

    print(f"Running msdf-atlas-gen for {os.path.basename(charset_file)}...")
    try:
        subprocess.run(cmd, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    except subprocess.CalledProcessError as e:
        print("Error running msdf-atlas-gen:")
        print(e.stderr.decode('utf-8', errors='ignore'))
        return False
    return True

def convert_json_to_bin(json_file, bin_file):
    """
    Packs the msdf-atlas-gen JSON into an O(1) binary format for PSP.
    Format:
    [Header]
      char magic[4] = "SDF\0"
      uint32_t atlas_width;
      uint32_t atlas_height;
      float line_height;
      float base_size;
      uint32_t glyph_count;
    [Lookup Table]
      uint16_t lookup[65536] (0 = not found/fallback, else index + 1)
    [Glyphs Array]
      struct Glyph {
          uint32_t codepoint;
          float u, v, w, h;
          float xoff, yoff, xadv;
      } glyphs[glyph_count];
    """
    with open(json_file, "r") as f:
        data = json.load(f)

    atlas_data = data.get("atlas", data)
    width = atlas_data["width"]
    height = atlas_data["height"]
    base_size = float(atlas_data.get("size", 32.0))
    metrics = data["metrics"]
    line_height = metrics["lineHeight"]

    glyphs_data = data.get("glyphs", [])
    glyph_count = len(glyphs_data)

    # +1 because index 0 in lookup means not found.
    # So we write the 'unknown' or first glyph at index 0 in array, but lookup stores index in array + 1.

    with open(bin_file, "wb") as f:
        # 1. Header (24 bytes)
        f.write(struct.pack("<4sIIffI", b"SDF\0", width, height, line_height, base_size, glyph_count))

        # 2. Lookup table: 65536 uint16s initialized to 0
        lookup = [0] * 65536
        for i, g in enumerate(glyphs_data):
            u = g.get("unicode", 0)
            if 0 <= u < 65536:
                # Store i + 1
                lookup[u] = i + 1

        f.write(struct.pack(f"<{65536}H", *lookup))

        # 3. Glyphs array
        for g in glyphs_data:
            utf32 = g.get("unicode", 0)
            adv = g.get("advance", 0.0)

            pb = g.get("planeBounds")
            ab = g.get("atlasBounds")

            if pb and ab:
                u = ab["left"] / width
                v = ab["bottom"] / height # because yorigin=top, bottom is technically the top Y coordinate in texture terms, or vice versa?
                # Actually, if yorigin=top, msdf-atlas-gen outputs 'top' as the smaller Y value and 'bottom' as larger?
                # Let's map it cleanly:

                # In msdf json with yorigin=top:
                # atlasBounds: left/top are the top-left corner in the image.
                gw = (ab["right"] - ab["left"]) / width
                gh = (ab["bottom"] - ab["top"]) / height

                # 'planeBounds' represents geometric coords.
                xoff = pb["left"]
                # For yorigin=top, Y goes down. Usually we just use top.
                yoff = pb["top"]

                f.write(struct.pack("<I7f", utf32, u, ab["top"]/height, gw, gh, xoff, yoff, adv))
            else:
                # Empty glyph (like space)
                f.write(struct.pack("<I7f", utf32, 0, 0, 0, 0, 0, 0, adv))

def generate_fonts(manifest, tmp_dir, out_dir, forced_symbols=None):
    """
    Generates fonts based on the manifest (group_name -> list of charset filenames).
    """
    os.makedirs(out_dir, exist_ok=True)

    for group_name, charset_files in manifest.items():
        if group_name not in FONT_MAP:
            continue

        # Special treatment for symbols merger
        if group_name == "symbols" and forced_symbols:
            # For now, icons merger still produces one unified atlas
            png, bin_ = generate_symbols_with_fallback(
                msdf_exe      = MSDF_EXE,
                fonts_dir     = FONTS_DIR,
                forced_symbols = forced_symbols,
                tmp_dir       = tmp_dir,
                out_dir       = out_dir,
                font_priority = SYMBOLS_FONT_PRIORITY,
                pxrange       = "2"
            )
            if png:
                print(f"Successfully built {bin_} and {png}")
            continue

        # Handle pages for the group
        for font_alias, font_filename in FONT_MAP[group_name]:
            font_path = os.path.join(FONTS_DIR, font_filename)

            for charset_filename in charset_files:
                # charset_filename is e.g. "cjk_0.txt" or "latin_cyrillic.txt"
                atlas_base_name = os.path.splitext(charset_filename)[0] # "cjk_0"

                # We need to distinguish between languages if they use same group (like CJK SC vs JP)
                # If alias is "cjk_jp", and file is "cjk_0.txt", output as "cjk_jp_0"
                page_suffix = ""
                if "_" in atlas_base_name:
                    page_suffix = "_" + atlas_base_name.split("_")[-1]

                atlas_id = f"{font_alias}{page_suffix}" # "cjk_jp_0"

                charset_path = os.path.join(tmp_dir, charset_filename)
                out_png   = os.path.join(out_dir, f"font_{atlas_id}.png")
                out_json  = os.path.join(tmp_dir, f"font_{atlas_id}.json")
                out_bin   = os.path.join(out_dir, f"font_{atlas_id}.bin")

                success = run_msdf_gen(charset_path, font_path, out_png, out_json)
                if success:
                    convert_json_to_bin(out_json, out_bin)
                    print(f"Successfully built {out_bin} and {out_png}")
