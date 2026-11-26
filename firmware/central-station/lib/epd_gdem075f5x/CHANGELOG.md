# Changelog

## 1.1.0 – 2026-11-26

- Fonts are rasterized with FreeType's light auto-hinter (vertical snapping only) and then
  thresholded, so every glyph shares one baseline, x-height and cap height. The old
  monochrome hinting lifted single glyphs (`i`, `t`, `f`, `9`) by a pixel. Fonts from 60 px
  up are rasterized unhinted. All bundled fonts regenerated from Inter 4.1.
- Kerning: optional `epd_kern_pair_t` table in `epd_font_t` (`kern`, `kern_count`),
  `epd_font_kern()`, applied when measuring and drawing text. Fonts without a table work as before.
- `fontconvert.py` now needs `freetype-py` instead of Pillow, `uharfbuzz` (optional) adds
  kerning; new `--hinting light|none`.
- `library.json` so PlatformIO also compiles `fonts/` when the component lives in `lib/`.

## 1.0.0 – 2026-11-26

- Driver for GDEM075F52 / GDEM075F53 (JD79665), ported from the Good Display / LaskaKit
  Arduino demo to ESP-IDF (`spi_master`, DMA bounce buffer, BUSY timeout, error codes).
- `epd_show_color()`, `epd_last_refresh_ms()`, `epd_prepare_deep_sleep()`, shared SPI bus support.
- Hardware-independent graphics layer: rotation, clipping, lines, dashed and thick lines,
  rectangles, rounded rectangles, circles, triangles, 2-colour patterns, 1-bit and 2-bit bitmaps.
- UTF-8 text with alignment, text boxes, multi-line text, `printf` variant.
- 14 bundled fonts based on Inter (Czech + Slovak, symbols, arrows) incl. big digit fonts.
- Tools: `fontconvert.py`, `gen_bundled_fonts.py`, `img2epd.py`.
- Host preview (renders screens to BMP on a PC), examples `hello`, `showcase`.
