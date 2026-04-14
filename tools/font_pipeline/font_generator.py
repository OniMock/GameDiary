import os
import sys
import json
import struct
import subprocess

MSDF_EXE = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "msdf-atlas-gen.exe")
FONT_FILE = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "NotoSansCJK-Regular.ttc")

def run_msdf_gen(charset_file, out_png, out_json):
    """
    Calls msdf-atlas-gen to generate SDF atlas and JSON metadata.
    Uses -yorigin top to match standard PSP GU UV coordinates (0,0 at top-left).
    """
    cmd = [
        MSDF_EXE,
        "-font", FONT_FILE,
        "-charset", charset_file,
        "-type", "sdf",       # Standard SDF
        "-format", "png",
        "-imageout", out_png,
        "-json", out_json,
        "-size", "32",        # Base size 32
        "-pxrange", "2",      # Distance field radius in pixels
        "-yorigin", "top",    # Matches standard 3D library UVs
        "-dimensions", "512", "512" # Enforce dimensions
    ]
    
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
    metrics = data["metrics"]
    line_height = metrics["lineHeight"]
    base_size = metrics["emSize"]
    
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
                
def generate_fonts(groups, tmp_dir, out_dir):
    os.makedirs(out_dir, exist_ok=True)
    
    for group_name in groups.keys():
        charset_file = os.path.join(tmp_dir, f"{group_name}.txt")
        if not os.path.exists(charset_file):
            continue
            
        out_png = os.path.join(out_dir, f"font_{group_name}.png")
        out_json = os.path.join(tmp_dir, f"font_{group_name}.json")
        out_bin = os.path.join(out_dir, f"font_{group_name}.bin")
        
        success = run_msdf_gen(charset_file, out_png, out_json)
        if success:
            convert_json_to_bin(out_json, out_bin)
            print(f"Successfully built {out_bin} and {out_png}")
