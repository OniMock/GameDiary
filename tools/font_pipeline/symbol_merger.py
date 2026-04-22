"""
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------

 symbol_merger.py
   Generates a single symbols atlas by trying multiple fonts in priority order.

   Strategy:
   1. Use fonttools to check which codepoints each font actually supports.
   2. Partition EXTRA_CHARS across fonts: primary → secondary → tertiary,
      so each char is rendered by the best available font.
   3. For each font subset, call msdf-atlas-gen to generate a partial atlas.
   4. Stack all partial PNGs vertically using Pillow into one final image.
   5. Re-map all glyph UV coordinates to the merged atlas space.
   6. Write the unified .bin using the same format as convert_json_to_bin.
"""

import os
import json
import struct
import subprocess
import sys
import tempfile
import math

# Optional: graceful import of fonttools and Pillow
try:
    from fontTools.ttLib import TTFont
    HAS_FONTTOOLS = True
except ImportError:
    HAS_FONTTOOLS = False

try:
    from PIL import Image
    HAS_PILLOW = True
except ImportError:
    HAS_PILLOW = False


def get_supported_codepoints(font_path):
    """
    Returns the set of Unicode codepoints the given font file supports.
    Falls back to empty set if fonttools is unavailable or the file can't be read.
    """
    if not HAS_FONTTOOLS:
        return set()  # caller will treat all chars as supported
    try:
        tt = TTFont(font_path, fontNumber=0)
        cmap = tt.getBestCmap()
        if cmap:
            return set(cmap.keys())
    except Exception as e:
        print(f"  [font-check] Warning: could not read {os.path.basename(font_path)}: {e}")
    return set()


def write_charset_file(codepoints, path):
    """Write a sorted hex-codepoint list file for msdf-atlas-gen."""
    hex_list = [f"0x{cp:X}" for cp in sorted(codepoints)]
    with open(path, "w", encoding="utf-8") as f:
        f.write(", ".join(hex_list))


def run_msdf_gen(msdf_exe, charset_file, font_path, out_png, out_json, pxrange="3",
                 dimensions=None):
    """Run msdf-atlas-gen for a single font/charset pair.

    dimensions: optional (width, height) tuple. If None, msdf-atlas-gen
    auto-packs the glyphs into the smallest valid texture. For the symbols
    merger we omit this so small subsets don't waste 512x512 each.
    """
    cmd = [
        msdf_exe,
        "-font", font_path,
        "-charset", charset_file,
        "-type", "sdf",
        "-format", "png",
        "-imageout", out_png,
        "-json", out_json,
        "-pxrange", str(pxrange),
        "-yorigin", "top",
    ]
    if dimensions:
        cmd += ["-dimensions", str(dimensions[0]), str(dimensions[1])]

    try:
        result = subprocess.run(cmd, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        return True
    except subprocess.CalledProcessError as e:
        stderr = e.stderr.decode("utf-8", errors="ignore")
        print(f"  [msdf] Error: {stderr.strip()}")
        return False


def next_power_of_2(n):
    """Return the smallest power of 2 >= n."""
    p = 1
    while p < n:
        p <<= 1
    return p


def merge_atlases(partial_results, final_png, final_json, max_width=512):
    """
    Stacks partial PNG atlases vertically into one image and produces a merged
    JSON with re-mapped UV coordinates.

    partial_results: list of (png_path, json_data) for each font subset.
    final_png: output PNG path.
    final_json: output JSON path.
    max_width: must match -dimensions passed to msdf-atlas-gen (512).

    Returns a merged JSON data dict or None on failure.
    """
    if not HAS_PILLOW:
        print("  [merge] Error: Pillow not installed. Run: pip install Pillow")
        return None

    images = []
    partial_heights = []

    for png_path, _json in partial_results:
        img = Image.open(png_path).convert("RGBA")
        images.append(img)
        partial_heights.append(img.height)

    total_height = sum(partial_heights)

    # PSP hardware limit: textures cannot exceed 512x512.
    # If the combined height of all partial atlases exceeds 512, we have a problem.
    # With small symbol counts (typ. 41 glyphs) and auto-sized partials this
    # should never happen — but we guard explicitly to catch it early.
    if total_height > 512:
        print(f"  [merge] WARNING: merged symbol height {total_height}px exceeds PSP 512px limit!")
        print(f"          Partial heights: {partial_heights}")
        print(f"          Consider reducing EXTRA_CHARS or splitting into another atlas.")
        # Clamp to 512 — glyphs beyond row 512 will be invisible but won't crash.
        total_height = 512

    # PSP textures must be power-of-2 in both dimensions
    final_height = next_power_of_2(total_height)

    # Compose the merged image
    merged = Image.new("RGBA", (max_width, final_height), (0, 0, 0, 0))
    y_cursor = 0
    for img in images:
        merged.paste(img, (0, y_cursor))
        y_cursor += img.height
    merged.save(final_png)

    # Build merged JSON: collect all glyphs, remapping their atlas UVs
    merged_glyphs = []
    y_offsets = []
    acc = 0
    for h in partial_heights:
        y_offsets.append(acc)
        acc += h

    # We take atlas meta from the first partial (line_height, metrics, etc.)
    base_json = partial_results[0][1]

    for (png_path, jdata), y_off, part_h in zip(partial_results, y_offsets, partial_heights):
        part_w = jdata["atlas"]["width"]
        for g in jdata.get("glyphs", []):
            ab = g.get("atlasBounds")
            if ab:
                # Remap pixel coords to the new merged atlas
                # We do NOT scale left/right because the partial images are pasted at X=0
                # without any horizontal stretching. We only shift the Y coordinates.
                new_ab = {
                    "left":   ab["left"],
                    "right":  ab["right"],
                    "top":    ab["top"]    + y_off,
                    "bottom": ab["bottom"] + y_off,
                }
                g = dict(g)
                g["atlasBounds"] = new_ab
            merged_glyphs.append(g)

    merged_json = {
        "atlas": {
            "type": "sdf",
            "width": max_width,
            "height": final_height,
            "size": base_json["atlas"].get("size", 32.0),
            "pxRange": 3,
            "yOrigin": "top"
        },
        "metrics": base_json["metrics"],
        "glyphs": merged_glyphs
    }

    with open(final_json, "w", encoding="utf-8") as f:
        json.dump(merged_json, f)

    return merged_json


def convert_merged_json_to_bin(merged_json, bin_file):
    """
    Writes the merged JSON to the same binary format as convert_json_to_bin
    in font_generator.py.
    """
    atlas_data = merged_json["atlas"]
    width      = atlas_data["width"]
    height     = atlas_data["height"]
    base_size  = float(atlas_data.get("size", 32.0))
    metrics    = merged_json["metrics"]
    line_height = metrics["lineHeight"]
    glyphs_data = merged_json.get("glyphs", [])
    glyph_count = len(glyphs_data)

    with open(bin_file, "wb") as f:
        # Header
        f.write(struct.pack("<4sIIffI",
                            b"SDF\0", width, height, line_height, base_size, glyph_count))

        # Lookup table: 65536 uint16s
        lookup = [0] * 65536
        for i, g in enumerate(glyphs_data):
            u = g.get("unicode", 0)
            if 0 <= u < 65536:
                lookup[u] = i + 1
        f.write(struct.pack(f"<{65536}H", *lookup))

        # Glyphs
        for g in glyphs_data:
            utf32 = g.get("unicode", 0)
            adv   = g.get("advance", 0.0)
            pb    = g.get("planeBounds")
            ab    = g.get("atlasBounds")

            if pb and ab:
                gw   = (ab["right"]  - ab["left"]) / width
                gh   = (ab["bottom"] - ab["top"])  / height
                xoff = pb["left"]
                yoff = pb["top"]
                f.write(struct.pack("<I7f",
                                    utf32,
                                    ab["left"] / width,
                                    ab["top"]  / height,
                                    gw, gh, xoff, yoff, adv))
            else:
                f.write(struct.pack("<I7f", utf32, 0, 0, 0, 0, 0, 0, adv))


def generate_symbols_with_fallback(
    msdf_exe, fonts_dir, forced_symbols,
    tmp_dir, out_dir,
    font_priority=None,
    pxrange="3"
):
    """
    Generates a single unified symbols atlas from multiple fonts in priority order.

    font_priority: list of font filenames to try in order.
    forced_symbols: set of characters that must appear in the symbols atlas.

    Returns (out_png, out_bin) paths on success, or (None, None) on failure.
    """
    if font_priority is None:
        font_priority = [
          "NotoSansCJK-Regular.ttc",
          "NotoSansSymbols2-Regular.ttf",
          "arial.ttf"
        ]

    if not HAS_PILLOW:
        print("  [symbols] Pillow not installed — falling back to single-font generation.")
        print("            Install with: pip install Pillow")
        return None, None

    target_codepoints = {ord(c) for c in forced_symbols}
    remaining = set(target_codepoints)
    partial_results = []  # list of (png_path, json_data)

    os.makedirs(tmp_dir, exist_ok=True)

    for i, font_filename in enumerate(font_priority):
        if not remaining:
            break  # all chars covered

        font_path = os.path.join(fonts_dir, font_filename)
        if not os.path.exists(font_path):
            print(f"  [symbols] Font not found, skipping: {font_filename}")
            continue

        # Determine which of the remaining codepoints this font supports
        supported = get_supported_codepoints(font_path)
        if supported:
            # Only attempt chars the font actually has glyphs for
            subset = remaining & supported
        else:
            # fonttools unavailable: try all remaining (msdf-atlas-gen will skip missing ones)
            subset = remaining

        if not subset:
            print(f"  [symbols] {font_filename}: no additional glyphs to contribute.")
            continue

        # Write subset charset file
        charset_path = os.path.join(tmp_dir, f"symbols_part{i}.txt")
        write_charset_file(subset, charset_path)

        out_png  = os.path.join(tmp_dir, f"symbols_part{i}.png")
        out_json = os.path.join(tmp_dir, f"symbols_part{i}.json")

        print(f"  [symbols] {font_filename}: generating {len(subset)} glyphs...")
        # No -dimensions: let msdf-atlas-gen auto-size each partial atlas.
        # Small subsets (e.g. 13 glyphs) will produce tiny textures (e.g. 64x64)
        # so that the merged final always fits within 512x512.
        success = run_msdf_gen(msdf_exe, charset_path, font_path, out_png, out_json, pxrange)

        if not success or not os.path.exists(out_json):
            print(f"  [symbols] Failed for {font_filename}, continuing to next font.")
            continue

        with open(out_json, "r") as jf:
            jdata = json.load(jf)

        # Track which codepoints were actually produced
        produced = {g["unicode"] for g in jdata.get("glyphs", []) if g.get("unicode")}
        remaining -= produced

        partial_results.append((out_png, jdata))
        print(f"  [symbols] {font_filename}: produced {len(produced)} glyphs. Still missing: {len(remaining)}")

    if not partial_results:
        print("  [symbols] No partial atlases produced.")
        return None, None

    if remaining:
        print(f"  [symbols] Warning: {len(remaining)} codepoints still missing after all fonts:")
        print(f"            {[chr(cp) for cp in sorted(remaining)]}")

    # Merge all partial atlases into one
    final_png  = os.path.join(out_dir, "font_symbols.png")
    final_json = os.path.join(tmp_dir, "font_symbols.json")
    final_bin  = os.path.join(out_dir, "font_symbols.bin")

    if len(partial_results) == 1:
        # Only one font contributed — no merging needed, just copy
        import shutil
        shutil.copy(partial_results[0][0], final_png)
        merged_json = {
            "atlas": partial_results[0][1]["atlas"],
            "metrics": partial_results[0][1]["metrics"],
            "glyphs": partial_results[0][1]["glyphs"]
        }
    else:
        print(f"  [symbols] Merging {len(partial_results)} partial atlases...")
        merged_json = merge_atlases(partial_results, final_png, final_json)
        if merged_json is None:
            return None, None

    convert_merged_json_to_bin(merged_json, final_bin)
    print(f"  [symbols] Done: {final_png} ({os.path.getsize(final_png)//1024}KB), {final_bin}")
    return final_png, final_bin
