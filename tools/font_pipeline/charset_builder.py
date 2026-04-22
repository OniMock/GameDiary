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

def load_extra_chars(filepath):
    if not os.path.exists(filepath):
        return set()
    with open(filepath, "r", encoding="utf-8") as f:
        return set(f.read().strip())


# Define unicode blocks for our 3 target atlases
RANGES = {
    "latin_cyrillic": [
        (0x0000, 0x03FF), # Basic Latin, Latin-1, Latin Ext-A, Latin Ext-B, IPA, Spacing Modifiers, Comb. Diacritics, Greek
        (0x0400, 0x052F), # Cyrillic & Cyrillic Supplement
        (0x1E00, 0x1EFF), # Latin Extended Additional
    ],
    "cjk": [
        (0x3000, 0x303F), # CJK Symbols
        (0x3040, 0x309F), # Hiragana
        (0x30A0, 0x30FF), # Katakana
        (0x31F0, 0x31FF), # Katakana Phonetic Ext
        (0x4E00, 0x9FFF), # CJK Unified Ideographs
        (0xAC00, 0xD7AF), # Hangul
        (0xFF00, 0xFFEF), # Halfwidth / Fullwidth Forms
    ],
    "symbols": [
        (0x2000, 0x2BFF), # Gen Punc, Currency, Box Drawing, Math, etc.
    ]
}

# For 32px quality, each glyph box is roughly 44x44px (including 6px SDF range and padding).
# 512 / 44 ~ 11 per row. 11x11 = 121 glyphs per 512x512 page.
# We'll cap at 120 chars per page to be safe.
MAX_CHARS_PER_PAGE = 120



def get_group_for_char(char):
    codepoint = ord(char)
    for group, ranges in RANGES.items():
        for r_start, r_end in ranges:
            if r_start <= codepoint <= r_end:
                return group
    # Default fallback
    return "symbols"

def build_charsets(characters, out_dir, forced_symbols=None):
    """
    Groups characters and writes a .txt charset per group.
    Returns a dictionary of group_name -> set_of_characters

    forced_symbols: an optional set of characters that should always go into
    the 'symbols' atlas, bypassing the Unicode range classifier. This is
    needed for chars like (R), (C), degree, euro that live in the Latin-1
    block but belong visually/semantically to the symbols font.
    """
    os.makedirs(out_dir, exist_ok=True)

    groups = {
        "latin_cyrillic": set(),
        "cjk": set(),
        "symbols": set()
    }

    # MANDATORY: Always include the full printable ASCII set in the latin atlas.
    # Runtime strings (formatted numbers, brackets, slashes etc.) are generated
    # at runtime and are NOT present in i18n files, so the scanner can't find them.
    for cp in range(0x20, 0x7F):  # space (0x20) through ~ (0x7E)
        groups["latin_cyrillic"].add(chr(cp))

    # Also add a fallback '?' to all atlases so the glyph-not-found path renders
    for g in groups.values():
        g.add('?')

    # Always include Hiragana + Katakana (core Japanese support)
    for cp in range(0x3040, 0x3100): # 0x30FF is the last Katakana, so range to 0x3100
        groups["cjk"].add(chr(cp))

    # Include curated Kanji list (Jōyō Kanji)
    script_dir = os.path.dirname(os.path.abspath(__file__))
    kanji_file = os.path.join(script_dir, "joyo_kanji.txt")
    extra_chars = load_extra_chars(kanji_file)
    groups["cjk"].update(extra_chars)
    if extra_chars:
        print(f"[JP] Loaded {len(extra_chars)} Kanji from file")

    # Pin explicit symbols directly into the symbols group regardless of their
    # Unicode block. Characters like (R) (0x00AE) or degree (0x00B0) live in
    # Latin-1 Supplement so the range classifier would put them in latin_cyrillic.
    if forced_symbols:
        groups["symbols"].update(forced_symbols)
        # Also remove them from latin_cyrillic so they don't duplicate there
        groups["latin_cyrillic"] -= forced_symbols

    for c in characters:
        # Skip chars already pinned to symbols so they stay in the right atlas
        if forced_symbols and c in forced_symbols:
            continue
        group = get_group_for_char(c)
        groups[group].add(c)

    # Write to files (handle multi-page splitting)
    final_group_manifest = {} # group_name -> list of charset files
 
    for group_name, chars in groups.items():
        if not chars:
            continue
 
        sorted_chars = sorted(list(chars))
        
        # Split into chunks if necessary
        chunks = [sorted_chars[i:i + MAX_CHARS_PER_PAGE] for i in range(0, len(sorted_chars), MAX_CHARS_PER_PAGE)]
        manifest_files = []
 
        for i, chunk in enumerate(chunks):
            # If multiple pages, add suffix, otherwise keep original name
            page_suffix = f"_{i}" if len(chunks) > 1 else ""
            filename = f"{group_name}{page_suffix}.txt"
            filepath = os.path.join(out_dir, filename)
            
            with open(filepath, "w", encoding="utf-8") as f:
                hex_list = [f"0x{ord(c):X}" for c in chunk]
                f.write(", ".join(hex_list))
            
            manifest_files.append(filename)
        
        final_group_manifest[group_name] = manifest_files
 
    return final_group_manifest

if __name__ == '__main__':
    # Test stub
    st = set("Hello 世界! Привет мир 😊")
    res = build_charsets(st, "tmp/fonts")
    for k, v in res.items():
        print(f"{k}: {''.join(sorted(v))}")
