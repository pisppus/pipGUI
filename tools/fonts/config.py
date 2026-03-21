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

def _kp(pair: str, adjust256: int) -> dict:
    """Kerning pair helper. adjust256 is 1/256 em units."""
    if not isinstance(pair, str) or len(pair) != 2:
        raise ValueError("pair must be 2 characters")
    return {
        "left": ord(pair[0]),
        "right": ord(pair[1]),
        "adjust256": int(adjust256),
    }

WIX_KERNING = [
    _kp("AV", -22),
    _kp("AW", -20),
    _kp("AY", -24),
    _kp("FA", -10),
    _kp("Fo", -12),
    _kp("La", -8),
    _kp("LT", -16),
    _kp("LV", -14),
    _kp("LW", -12),
    _kp("LY", -18),
    _kp("PA", -12),
    _kp("Ta", -16),
    _kp("Te", -16),
    _kp("To", -18),
    _kp("Ty", -16),
    _kp("VA", -24),
    _kp("Ve", -18),
    _kp("Vo", -20),
    _kp("WA", -28),
    _kp("Wo", -16),
    _kp("YA", -26),
]

FONTS = {
    # tools/fonts/TTF/krona-one.ttf
    "krona-one": {
        "size": 48,
        "distanceRange": 8,
        "tracking128": 1,
        "glyphs": [
            _chars("PISPPUS"),
        ],
    },
    # tools/fonts/TTF/WixMadeforDisplay.ttf
    "WixMadeforDisplay": {
        "size": 48,
        "distanceRange": 8,
        "tracking128": 2,
        "kerningPairs": WIX_KERNING,
        "glyphs": [
            LATIN,
            CYRILLIC,
            EXTRA,
        ],
    },
}
