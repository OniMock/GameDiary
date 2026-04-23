"""
* -------------------------------------------------------------
*  GameDiary
*  Playtime Tracking System for the PlayStation Portable (PSP)
*
*  Developed by OniMock
*  © 2026 OniMock. All rights reserved.
* -------------------------------------------------------------
"""

import re
import os
import sys
from pathlib import Path


def find_project_root(start_path):
    """Find the project root by looking for known markers"""
    current = Path(start_path).resolve()

    # Look for markers that indicate project root
    markers = [".git", "include", "src", "CMakeLists.txt", "Makefile"]

    for parent in [current] + list(current.parents):
        for marker in markers:
            if (parent / marker).exists():
                return parent
    return current


def extract_enum_keys(header_path):
    """Extracts all message keys from i18n.h enum"""
    with open(header_path, "r", encoding="utf-8", errors="ignore") as f:
        content = f.read()

    # Strip comments
    content = re.sub(r"//.*", "", content)
    content = re.sub(r"/\*.*?\*/", "", content, flags=re.DOTALL)

    # Find the MessageId enum block
    enum_match = re.search(
        r"typedef\s+enum\s*\{([^}]+)\}\s*MessageId\s*;", content, re.DOTALL
    )

    if not enum_match:
        # Try alternative pattern without typedef
        enum_match = re.search(r"enum\s+MessageId\s*\{([^}]+)\}", content, re.DOTALL)

    if not enum_match:
        print(f"Error: Could not find the MessageId enum in {header_path}")
        return set()

    enum_content = enum_match.group(1)

    # Match ALL uppercase identifiers (with underscores) that could be enum constants
    keys = re.findall(r"\b([A-Z][A-Z0-9_]+)\b", enum_content)

    # Exclude common non-message constants
    exclude = {
        "MSG_COUNT",  # Counter, not a message
        "LANG_COUNT",  # Counter, not a message
        "LANG_AUTO",  # Special value, not a message
    }

    return {k for k in keys if k not in exclude}


def extract_c_keys(c_path):
    """Extracts all keys from i18n language data files (any uppercase identifier)"""
    with open(c_path, "r", encoding="utf-8", errors="ignore") as f:
        content = f.read()

    # Look for array initialization like: [KEY_NAME] = "..."
    # Only match if it's inside brackets at the start of a logical line (ignoring whitespace)
    all_keys = re.findall(r"^\s*\[\s*([A-Z][A-Z0-9_]+)\s*\]", content, re.MULTILINE)

    # Exclude array size constants and language enums
    exclude = {
        "MSG_COUNT",  # Used for array size
        "LANG_COUNT",  # Used for array size
        "LANG_AUTO",
    }

    return {k for k in all_keys if k not in exclude}


def is_language_data_file(filepath):
    """Check if the file is a language data file (not implementation)"""
    # Language data files follow pattern: i18n_XX.c where XX is language code
    # Or could be i18n_XXX.c for 3-letter codes
    return re.match(r"i18n_[a-z]{2,3}\.c$", filepath.name) is not None


def find_unused_keys(project_root, keys):
    """Finds keys that are not used in the codebase (excluding i18n data files)"""
    unused_keys = set(keys)
    protected_prefixes = set()

    # Extensions to search in
    extensions = {".c", ".h", ".cpp", ".hpp"}

    # Files to exclude from the usage search (definitions)
    # These contain the definitions, not the consumption of the keys
    exclude_files = {"i18n.h"}

    print(f"\n--- Checking for unused keys across codebase ---")
    print(f"Scanning .c, .h, .cpp, .hpp files (excluding language data files)...")

    # Scan all files in project
    for path in project_root.rglob("*"):
        if path.suffix in extensions:
            # Skip if it's a language data file (i18n_XX.c) or the i18n header
            if is_language_data_file(path) or path.name in exclude_files:
                continue

            try:
                with open(path, "r", encoding="utf-8", errors="ignore") as f:
                    content = f.read()

                    # 1. Advanced check: Handle ranges (e.g., MSG_DAY_SUN + i)
                    # If we see a key followed by '+', we protect its group
                    arithmetic_matches = re.findall(r"\b(MSG_[A-Z0-9_]+)\s*\+", content)
                    for base_key in arithmetic_matches:
                        if "_" in base_key:
                            # Use prefix as protection (e.g. MSG_DAY_SUN -> MSG_DAY_)
                            prefix = base_key.rsplit("_", 1)[0] + "_"
                            protected_prefixes.add(prefix)

                    # 2. Simple check: Direct literal usage
                    for key in list(unused_keys):
                        if key in content:
                            unused_keys.remove(key)

                    # Early exit only if both unused and protected prefixes are empty? 
                    # No, we only exit if unused_keys is empty.
                    if not unused_keys:
                        break
            except Exception as e:
                print(f"Warning: Could not read {path}: {e}")

    # Apply protection for ranges
    if protected_prefixes:
        # print(f"INFO: Detected range arithmetic for: {', '.join(sorted(protected_prefixes))}")
        for prefix in protected_prefixes:
            for key in list(unused_keys):
                if key.startswith(prefix):
                    unused_keys.remove(key)

    return unused_keys


def main():
    # Find the project root
    script_dir = Path(__file__).parent.resolve()
    project_root = find_project_root(script_dir)

    print(f"Project root detected at: {project_root}")

    header_path = project_root / "include" / "app" / "i18n" / "i18n.h"
    i18n_dir = project_root / "src" / "app" / "i18n"
    en_path = i18n_dir / "i18n_en.c"

    if not header_path.exists():
        print(f"Error: {header_path} not found.")
        sys.exit(1)

    if not en_path.exists():
        print(f"Error: {en_path} not found.")
        sys.exit(1)

    print("--- Validating i18n.h vs i18n_en.c ---")
    header_keys = extract_enum_keys(header_path)
    en_keys = extract_c_keys(en_path)

    print(f"\nFound {len(header_keys)} keys in i18n.h:")
    if header_keys:
        print(f"  Sample: {', '.join(list(sorted(header_keys))[:5])}...")

    print(f"Found {len(en_keys)} keys in i18n_en.c:")
    if en_keys:
        print(f"  Sample: {', '.join(list(sorted(en_keys))[:5])}...")

    failed = False

    missing_in_en = header_keys - en_keys
    if missing_in_en:
        print(
            f"\n[FAIL]: {len(missing_in_en)} keys in i18n.h but missing in i18n_en.c:"
        )
        for k in sorted(missing_in_en):
            print(f"  - {k}")
        failed = True

    missing_in_header = en_keys - header_keys
    if missing_in_header:
        print(
            f"\n[FAIL]: {len(missing_in_header)} keys in i18n_en.c but missing in i18n.h:"
        )
        for k in sorted(missing_in_header):
            print(f"  - {k}")
        failed = True

    if not failed:
        print("\n[OK]: i18n.h and i18n_en.c are in sync.")

    print("\n--- Validating other languages vs i18n_en.c ---")
    # Look for language data files only (i18n_XX.c where XX is language code)
    # Exclude implementation files (i18n.c) and the base English file
    lang_files = [
        f
        for f in i18n_dir.glob("i18n_*.c")
        if is_language_data_file(f) and f.name != "i18n_en.c"
    ]

    if not lang_files:
        print("No other language files found.")
        print(f"  (Looking for files matching pattern: i18n_??.c or i18n_???.c)")
    else:
        print(
            f"Found {len(lang_files)} language data files: {', '.join(f.name for f in lang_files)}"
        )

    for lang_file in lang_files:
        lang_keys = extract_c_keys(lang_file)

        missing_in_lang = en_keys - lang_keys
        if missing_in_lang:
            print(
                f"\n[FAIL]: {lang_file.name} is missing {len(missing_in_lang)} keys from i18n_en.c:"
            )
            missing_list = sorted(missing_in_lang)
            for k in missing_list[:15]:
                print(f"  - {k}")
            if len(missing_list) > 15:
                print(f"  ... and {len(missing_list) - 15} more")
            failed = True
        else:
            print(
                f"\n[OK]: {lang_file.name} has all {len(en_keys)} keys from i18n_en.c"
            )

        extra_in_lang = lang_keys - en_keys
        if extra_in_lang:
            print(
                f"[WARN]: {lang_file.name} has {len(extra_in_lang)} keys NOT in i18n_en.c:"
            )
            for k in sorted(extra_in_lang):
                print(f"  - {k}")

    # Optional: Check if there's an implementation file (just for info, not warning)
    impl_file = i18n_dir / "i18n.c"
    if impl_file.exists():
        print(f"\n[INFO]: Found implementation file: {impl_file}")
        print("   (This is the i18n system implementation, not a language data file)")

    # NEW: Check for unused keys
    unused_keys = find_unused_keys(project_root, header_keys)
    if unused_keys:
        print(f"\n[WARN]: Found {len(unused_keys)} unused keys in the codebase:")
        for k in sorted(unused_keys):
            print(f"  - {k}")
        print("   (These keys are defined in i18n.h but never referenced in other files)")
    else:
        print("\n[OK]: All keys defined in i18n.h are used in the codebase.")

    if failed:
        print("\n[FAIL]: Validation FAILED. Please fix the missing keys before building.")
        sys.exit(1)
    else:
        print("\n[OK]: SUCCESS: All language files are consistent with i18n.h")
        sys.exit(0)


if __name__ == "__main__":
    main()
