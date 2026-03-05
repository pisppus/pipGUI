"""Font generator config.

Keys in FONTS must match the basename of the .ttf file in tools/fonts/TTF.
Example: tools/fonts/TTF/krona-one.ttf -> key "krona-one".
"""

# Preset glyph sets - uncomment what you need in FONTS below
LATIN   = [32, 126]           # Basic ASCII (space ~)
CYRILLIC = [0x0410, 0x044F]   # Russian А-я
EXTRA   = [0x0401, 0x0451, 0x2116, 0x20BD, 0x00B0]  # Ёё№₽°

# Other available sets (uncomment to use):
# LATIN_EXT = [160, 255]      # Extended Latin (accents, etc)
# CYRILLIC_FULL = [0x0400, 0x04FF]  # All Cyrillic
# GREEK = [0x0370, 0x03FF]    # Greek alphabet
# NUMBERS = [48, 57]          # 0-9 only
# UPPERCASE = [65, 90]        # A-Z only
# LOWERCASE = [97, 122]       # a-z only

def _chars(s: str) -> list:
    """Convert string to list of unicode codepoints."""
    return [ord(c) for c in s]

FONTS = {
    # tools/fonts/TTF/krona-one.ttf
    "krona-one": {
        "size": 48,
        "distanceRange": 8,
        "glyphs": [
            _chars("PISPPUS"),
        ],
    },
    # tools/fonts/TTF/WixMadeforDisplay.ttf
    "WixMadeforDisplay": {
        "size": 48,
        "distanceRange": 8,
        "glyphs": [
            LATIN,
            CYRILLIC,
            EXTRA,
        ],
    },
}
