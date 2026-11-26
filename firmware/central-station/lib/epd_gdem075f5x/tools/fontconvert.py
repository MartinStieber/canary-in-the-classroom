#!/usr/bin/env python3
"""
Convert a TTF/OTF font into a 1-bit epd_font_t C source for the epd_gdem075f5x component.

    pip install freetype-py uharfbuzz      (uharfbuzz is optional, it adds kerning)
    python3 fontconvert.py MyFont.ttf 28 my_font_28 --chars cz -o my_font_28.c

Then add the .c file to your project (or the component's fonts/ folder) and declare it:

    #include "epd_font.h"
    extern const epd_font_t my_font_28;

Character sets (--chars, can be combined with commas, e.g. "cz,arrows"):
    ascii    printable ASCII 0x20-0x7E
    cz       ascii + Czech and Slovak letters + common symbols (° ± × · – — − … € µ ² ³ ‰ ₀-₉)
    arrows   ← ↑ → ↓ ↗ ↘ ▲ ▼ ● ○ ✓ ✗ ⚠
    digits   0-9 space . , : - + % ° / (for big numbers)
    any other value is taken literally as the list of characters to include

Size is the font size in pixels (em size), the same number you would give a desktop
app at 72 dpi. The real height of capitals is about 0.73x this for Inter.

Rasterization (below 60 px): FreeType's auto-hinter in "light" mode snaps every glyph's baseline,
x-height and cap height to the same pixel rows (only vertically, stem widths keep the
designed weight), the glyph is rendered antialiased and thresholded to 1 bit. This keeps
letters on one line and of one height; plain monochrome hinting of CFF/OTF fonts
(the previous method) lifted or dropped single glyphs by a pixel. From 60 px up the
outlines are rasterized unhinted, which is already exact there (round glyphs keep their
designed ~1 px overshoot).
"""
import argparse
import os
import re
import sys

ASCII = "".join(chr(c) for c in range(0x20, 0x7F))
CZSK = "áčďéěíňóřšťúůýžÁČĎÉĚÍŇÓŘŠŤÚŮÝŽäôľĺŕÄÔĽĹŔ"
SYMBOLS = "°±×·–—−…€µ²³‰" + "".join(chr(c) for c in range(0x2080, 0x208A))  # subscripts ₀-₉
ARROWS = "←↑→↓↗↘▲▼●○✓✗⚠"
DIGITS = "0123456789 .,:-+%°/"

PRESETS = {
    "ascii": ASCII,
    "cz": ASCII + CZSK + SYMBOLS,
    "arrows": ARROWS,
    "digits": DIGITS,
}


def _ft():
    try:
        import freetype
    except ImportError:
        sys.exit("needs freetype-py: pip install freetype-py")
    return freetype


def render_glyph(face, ch, threshold, hinting="light"):
    """Returns (width, height, x_off, y_off, advance, rows) with rows as lists of 0/1."""
    ft = _ft()
    flags = ft.FT_LOAD_NO_HINTING if hinting == "none" else \
        ft.FT_LOAD_TARGET_LIGHT | ft.FT_LOAD_FORCE_AUTOHINT
    face.load_char(ch, flags)
    face.glyph.render(ft.FT_RENDER_MODE_LIGHT)
    slot, bm = face.glyph, face.glyph.bitmap
    advance = (slot.advance.x + 32) >> 6
    buf = bm.buffer                     # freetype-py rebuilds this list on every access
    rows = [[1 if buf[j * bm.pitch + i] >= threshold else 0 for i in range(bm.width)]
            for j in range(bm.rows)]
    x_off, y_off = slot.bitmap_left, -slot.bitmap_top
    # trim empty border (thresholding can leave blank rows/columns)
    while rows and not any(rows[0]):
        rows.pop(0)
        y_off += 1
    while rows and not any(rows[-1]):
        rows.pop()
    if not rows:
        return 0, 0, 0, 0, advance, []
    while not any(r[0] for r in rows):
        rows = [r[1:] for r in rows]
        x_off += 1
    while not any(r[-1] for r in rows):
        rows = [r[:-1] for r in rows]
    return len(rows[0]), len(rows), x_off, y_off, advance, rows


_KERN_CACHE = {}


def font_kerning(path, chars):
    """({(left, right): adjustment in font units}, units_per_em) from GPOS/kern via HarfBuzz.

    Returns None when uharfbuzz is not installed (the font is then built without kerning).
    """
    try:
        import uharfbuzz as hb
    except ImportError:
        return None
    key = (path, "".join(chars))
    if key in _KERN_CACHE:
        return _KERN_CACHE[key]
    face = hb.Face(hb.Blob.from_file_path(path))
    font = hb.Font(face)
    features = {"kern": True, "liga": False, "calt": False, "dlig": False}

    def first_advance(text):
        buf = hb.Buffer()
        buf.add_str(text)
        buf.guess_segment_properties()
        hb.shape(font, buf, features)
        return buf.glyph_positions[0].x_advance

    single = {c: first_advance(c) for c in chars}
    pairs = {}
    for a in chars:
        for b in chars:
            k = first_advance(a + b) - single[a]
            if k:
                pairs[(a, b)] = k
    _KERN_CACHE[key] = (pairs, face.upem)
    return _KERN_CACHE[key]


def pack_bits(rows):
    out, acc, n = [], 0, 0
    for row in rows:
        for b in row:
            acc = (acc << 1) | b
            n += 1
            if n == 8:
                out.append(acc)
                acc, n = 0, 0
    if n:
        out.append(acc << (8 - n))
    return out


def make_ranges(codepoints):
    ranges, start, prev = [], None, None
    for cp in codepoints:
        if start is None:
            start = prev = cp
        elif cp == prev + 1:
            prev = cp
        else:
            ranges.append((start, prev))
            start = prev = cp
    if start is not None:
        ranges.append((start, prev))
    return ranges


def c_char_comment(ch):
    if ch == "\\":
        return "backslash"
    if ch == " ":
        return "space"
    return ch.replace("*/", "* /")


def convert(path, size, ident, chars_spec, threshold=None, display_name=None, hinting=None):
    if hinting is None:
        # The auto-hinter is what keeps small text even; above ~60 px it starts moving
        # single glyphs by a pixel or two, while plain outlines already land cleanly.
        hinting = "light" if size < 60 else "none"
    if threshold is None:
        threshold = 128
    face = _ft().Face(path)
    face.set_pixel_sizes(0, size)
    chars = sorted(set("".join(PRESETS.get(p, p) for p in chars_spec.split(","))) | {"?"},
                   key=ord)
    if any(ord(c) > 0xFFFF for c in chars):
        sys.exit("only code points up to U+FFFF are supported")
    not_in_font = [c for c in chars if face.get_char_index(ord(c)) == 0]
    chars = [c for c in chars if c not in not_in_font]        # those fall back to '?'

    ascent = (face.size.ascender + 63) >> 6
    descent = (-face.size.descender + 63) >> 6
    line_height = ascent + descent
    cap_ref = "H" if "H" in chars else ("0" if "0" in chars else chars[0])
    cap_height = -render_glyph(face, cap_ref, threshold, hinting)[3]

    glyphs, bitmap = [], []
    for ch in chars:
        w, h, xo, yo, adv, rows = render_glyph(face, ch, threshold, hinting)
        if w > 255 or h > 255 or adv > 255 or not -128 <= xo <= 127:
            sys.exit(f"glyph {ch!r} too large for the format (size {size} too big)")
        glyphs.append(dict(ch=ch, w=w, h=h, xo=xo, yo=yo, adv=adv, off=len(bitmap)))
        bitmap.extend(pack_bits(rows))

    kern = []
    kd = font_kerning(path, [c for c in chars if not c.isspace()])
    if kd is not None:
        units, upem = kd
        for (a, b), k in units.items():
            px = int(round(k * size / upem))
            if px and -128 <= px <= 127:
                kern.append((ord(a), ord(b), px))
        kern.sort()

    ranges = []
    idx = 0
    for first, last in make_ranges([ord(c) for c in chars]):
        ranges.append((first, last, idx))
        idx += last - first + 1

    base = os.path.splitext(os.path.basename(path))[0]
    name = display_name or f"{base} {size}"
    lines = []
    lines.append(f"/* Generated by tools/fontconvert.py - do not edit.")
    lines.append(f" * Source: {os.path.basename(path)}, {size} px, {len(chars)} glyphs, "
                 f"{len(bitmap)} B bitmap")
    lines.append(f" * Metrics: line height {line_height}, ascent {ascent}, descent {descent}, "
                 f"cap height {cap_height} px */")
    lines.append('#include "epd_font.h"\n')
    lines.append(f"static const uint8_t {ident}_bitmap[{max(len(bitmap), 1)}] = {{")
    for i in range(0, len(bitmap), 16):
        lines.append("    " + ", ".join(f"0x{b:02X}" for b in bitmap[i:i + 16]) + ",")
    if not bitmap:
        lines.append("    0x00,")
    lines.append("};\n")
    lines.append(f"static const epd_glyph_t {ident}_glyphs[{len(glyphs)}] = {{")
    for g in glyphs:
        lines.append(f"    {{ {g['off']:6d}, {g['w']:3d}, {g['h']:3d}, {g['adv']:3d}, "
                     f"{g['xo']:4d}, {g['yo']:5d} }},  /* U+{ord(g['ch']):04X} "
                     f"{c_char_comment(g['ch'])} */")
    lines.append("};\n")
    lines.append(f"static const epd_font_range_t {ident}_ranges[{len(ranges)}] = {{")
    for first, last, gi in ranges:
        lines.append(f"    {{ 0x{first:04X}, 0x{last:04X}, {gi:4d} }},")
    lines.append("};\n")
    if kern:
        lines.append(f"static const epd_kern_pair_t {ident}_kern[{len(kern)}] = {{")
        for a, b, px in kern:
            lines.append(f"    {{ 0x{a:04X}, 0x{b:04X}, {px:3d} }},  /* "
                         f"{c_char_comment(chr(a))}{c_char_comment(chr(b))} */")
        lines.append("};\n")
    lines.append(f"const epd_font_t {ident} = {{")
    lines.append(f'    .name = "{name}",')
    lines.append(f"    .bitmap = {ident}_bitmap,")
    lines.append(f"    .glyphs = {ident}_glyphs,")
    lines.append(f"    .ranges = {ident}_ranges,")
    lines.append(f"    .range_count = {len(ranges)},")
    lines.append(f"    .line_height = {line_height},")
    lines.append(f"    .ascent = {ascent},")
    lines.append(f"    .descent = {descent},")
    lines.append(f"    .cap_height = {cap_height},")
    if kern:
        lines.append(f"    .kern = {ident}_kern,")
        lines.append(f"    .kern_count = {len(kern)},")
    lines.append("};\n")

    info = dict(ident=ident, name=name, size=size, glyphs=len(glyphs), bitmap_bytes=len(bitmap),
                flash_bytes=len(bitmap) + 12 * len(glyphs) + 6 * len(ranges) + 6 * len(kern) + 40,
                kern_pairs=len(kern), kerning=kd is not None,
                line_height=line_height, ascent=ascent, descent=descent, cap_height=cap_height,
                missing=not_in_font)
    return "\n".join(lines), info


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("font", help="TTF/OTF file")
    ap.add_argument("size", type=int, help="size in px")
    ap.add_argument("ident", help="C identifier, e.g. my_font_28")
    ap.add_argument("--chars", default="cz", help="character set(s), see above (default: cz)")
    ap.add_argument("--threshold", type=int, default=None,
                    help="coverage 0-255 a pixel needs to become ink (default 128; "
                         "lower, e.g. 100, gives slightly bolder strokes)")
    ap.add_argument("--hinting", choices=["light", "none"], default=None,
                    help="light = FreeType auto-hinter, vertical only (default below 60 px); "
                         "none = plain outlines (default from 60 px)")
    ap.add_argument("--name", default=None, help="display name stored in the font")
    ap.add_argument("-o", "--output", default=None, help="output .c file (default: <ident>.c)")
    a = ap.parse_args()
    if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", a.ident):
        sys.exit("ident must be a valid C identifier")

    src, info = convert(a.font, a.size, a.ident, a.chars, a.threshold, a.name, a.hinting)
    out = a.output or f"{a.ident}.c"
    with open(out, "w", encoding="utf-8") as f:
        f.write(src)
    print(f"{out}: {info['glyphs']} glyphs, {info['kern_pairs']} kerning pairs, "
          f"~{info['flash_bytes']} B flash, line {info['line_height']} px, "
          f"caps {info['cap_height']} px")
    if not info["kerning"]:
        print("note: uharfbuzz not installed, font built without kerning", file=sys.stderr)
    if info["missing"]:
        print("warning: not in the font (will show as '?'): " + "".join(info["missing"]),
              file=sys.stderr)


if __name__ == "__main__":
    main()
