import os
import sys

from scan_i18n import scan_all_i18n
from charset_builder import build_charsets
from font_cache import FontCache
from font_generator import generate_fonts
from embed_generator import generate_embeds

EXTRA_CHARS = set(
    "®°©™€£¥¢§¶†‡•…‰′″←↑→↓↔⇒⇐⇔♠♣♥♦★☆○●◎◊□■△▲▼"
)

def main():
    base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    workspace_dir = os.path.dirname(base_dir)

    src_i18n_dir   = os.path.join(workspace_dir, "src", "app", "i18n")
    tmp_fonts_dir  = os.path.join(workspace_dir, "tmp", "fonts")
    assets_dir     = os.path.join(workspace_dir, "assets", "fonts")
    src_render_dir = os.path.join(workspace_dir, "src", "app", "render")
    inc_dir        = os.path.join(workspace_dir, "include")

    os.makedirs(tmp_fonts_dir, exist_ok=True)
    os.makedirs(assets_dir, exist_ok=True)

    print("--- GameDiary SDF Font Pipeline ---")

    # 1. Scan
    print("1. Scanning i18n files...")
    all_chars = scan_all_i18n([src_i18n_dir])
    print(f"   Found {len(all_chars)} unique characters.")
    all_chars.update(EXTRA_CHARS)
    print(f"   Found {len(all_chars)} unique characters (including {len(EXTRA_CHARS)} special extras).")

    # 2. Build Charsets
    print("2. Grouping characters...")
    groups = build_charsets(all_chars, tmp_fonts_dir)
    for g, chars in groups.items():
        print(f"   [{g}] -> {len(chars)} chars")

    # 3. Cache & Generate
    print("3. Generating missing/updated atlases...")
    cache_path = os.path.join(tmp_fonts_dir, "cache.json")
    cache = FontCache(cache_path)

    atlases_changed = False
    groups_to_generate = {}
    for g, chars in groups.items():
        if not chars:
            continue

        charset_text = "".join(sorted(list(chars)))
        if cache.has_changed(g, charset_text):
            print(f"   [!] '{g}' has changed. Will generate.")
            groups_to_generate[g] = chars
            atlases_changed = True
        else:
            print(f"   [v] '{g}' is unchanged. Skipping generation.")

    if groups_to_generate:
        generate_fonts(groups_to_generate, tmp_fonts_dir, assets_dir)

        # Update cache on success
        for g, chars in groups_to_generate.items():
            cache.update(g, "".join(sorted(list(chars))))
        cache.save()
    else:
        print("   Atlases are up to date.")

    # 4. Embed — always regenerate C arrays so the build is always fresh.
    # Even if atlases didn't change, the .c files might be missing (e.g. clean checkout).
    print("4. Embedding font data as C arrays...")
    atlas_names = ["latin_cyrillic", "cjk", "symbols"]
    generate_embeds(assets_dir, src_render_dir, inc_dir, atlas_names)

    print("--- Font Pipeline Complete ---")
    print(f"   Assets: {assets_dir}")
    print(f"   Embedded C sources: {src_render_dir}/font_*_embed.c")
    print(f"   Rebuild EBOOT to include embedded fonts.")

if __name__ == '__main__':
    main()
