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
import glob
import re
import codecs

def decode_c_string(c_str):
    """
    Safely decode C string literals, focusing on \\xNN, \\uNNNN and common escapes.
    Ignores broken escapes.
    """
    res = ""
    i = 0
    while i < len(c_str):
        if c_str[i] == '\\' and i + 1 < len(c_str):
            i += 1
            c = c_str[i]
            if c == 'n': res += '\n'
            elif c == 't': res += '\t'
            elif c == 'r': res += '\r'
            elif c == '"': res += '"'
            elif c == '\\': res += '\\'
            elif c == 'x' and i + 2 < len(c_str):
                try:
                    val = int(c_str[i+1:i+3], 16)
                    res += chr(val)
                    i += 2
                except ValueError:
                    res += c_str[i-1:i+1] # fallback
            elif c == 'u' and i + 4 < len(c_str):
                try:
                    val = int(c_str[i+1:i+5], 16)
                    res += chr(val)
                    i += 4
                except ValueError:
                    res += c_str[i-1:i+1] # fallback
            else:
                res += c
        else:
            res += c_str[i]
        i += 1

    return res

def extract_strings_from_file(filepath):
    """
    Extracts all C string literals from a given C/C++ source file.
    It concatenates adjoining string literals.
    """
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()

    # Remove C-style block comments
    content = re.sub(r'/\*.*?\*/', '', content, flags=re.DOTALL)
    # Remove C-style line comments
    content = re.sub(r'//.*', '', content)

    # Match string literals: "..."
    # This regex handles escaped quotes \" inside the string.
    str_regex = re.compile(r'"((?:\\.|[^"\\])*)"')

    unique_chars = set()
    for match in str_regex.finditer(content):
        raw_str = match.group(1)
        decoded_str = decode_c_string(raw_str)
        # Add all characters to the set
        unique_chars.update(decoded_str)

    return unique_chars

def scan_all_i18n(directories):
    """
    Scans all i18n files in the given directories returning a set of all unique codepoints.
    """
    all_chars = set()
    for directory in directories:
        # Find all .c, .cpp, .h files
        for ext in ('*.c', '*.cpp', '*.h'):
            for filepath in glob.glob(os.path.join(directory, '**', ext), recursive=True):
                print(f"Scanning {filepath}...")
                chars = extract_strings_from_file(filepath)
                all_chars.update(chars)
    return all_chars

if __name__ == '__main__':
    # Test stub
    chars = scan_all_i18n(["src/app/i18n"])
    print(f"Extracted {len(chars)} unique character(s).")
    print("".join(sorted(chars)))
