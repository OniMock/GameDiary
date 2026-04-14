import os

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

def get_group_for_char(char):
    codepoint = ord(char)
    for group, ranges in RANGES.items():
        for r_start, r_end in ranges:
            if r_start <= codepoint <= r_end:
                return group
    # Default fallback
    return "symbols"

def build_charsets(characters, out_dir):
    """
    Groups characters and writes a .txt charset per group.
    Returns a dictionary of group_name -> set_of_characters
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
        
    for c in characters:
        group = get_group_for_char(c)
        groups[group].add(c)
        
    # Write to files
    for group_name, chars in groups.items():
        if not chars:
            continue
        
        filepath = os.path.join(out_dir, f"{group_name}.txt")
        # msdf-atlas-gen expects comma-separated list of hex values, or strings, etc.
        # Safest is comma-separated hex (e.g. 0x0041, 0x0042)
        with open(filepath, "w", encoding="utf-8") as f:
            sorted_chars = sorted(list(chars))
            hex_list = [f"0x{ord(c):X}" for c in sorted_chars]
            f.write(", ".join(hex_list))
            
    return groups

if __name__ == '__main__':
    # Test stub
    st = set("Hello 世界! Привет мир 😊")
    res = build_charsets(st, "tmp/fonts")
    for k, v in res.items():
        print(f"{k}: {''.join(sorted(v))}")
