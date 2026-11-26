#include "showcase_ui.h"
#include "epd_fonts.h"

static const char *SAMPLE = "Příliš žluťoučký kůň úpěl ďábelské ódy";
static const char *SAMPLE_SHORT = "Příliš žluťoučký kůň ÚPĚL";

void showcase_draw_fonts(epd_canvas_t *c)
{
    static const struct { const epd_font_t *f; epd_color_t col; } list[] = {
        { &epd_font_inter_regular_12, EPD_BLACK },
        { &epd_font_inter_regular_16, EPD_BLACK },
        { &epd_font_inter_regular_20, EPD_BLACK },
        { &epd_font_inter_regular_24, EPD_BLACK },
        { &epd_font_inter_regular_32, EPD_BLACK },
        { &epd_font_inter_bold_16,    EPD_RED },
        { &epd_font_inter_bold_20,    EPD_BLACK },
        { &epd_font_inter_bold_24,    EPD_BLACK },
        { &epd_font_inter_bold_32,    EPD_BLACK },
        { &epd_font_inter_bold_48,    EPD_BLACK },
    };

    epd_gfx_clear(c, EPD_WHITE);
    epd_gfx_fill_rect(c, 0, 0, c->width, 34, EPD_BLACK);
    epd_gfx_text_box(c, 12, 0, c->width - 24, 34, "epd_gdem075f5x · fonty", &epd_font_inter_bold_20,
                     EPD_WHITE, EPD_ALIGN_LEFT, EPD_ALIGN_MIDDLE);
    epd_gfx_text_box(c, 12, 0, c->width - 24, 34, "CO₂ 1 250 ppm · 22,4 °C · ↑ ↗ → ↘ ↓ ✓ ✗ ⚠",
                     &epd_font_inter_regular_16, EPD_YELLOW, EPD_ALIGN_RIGHT, EPD_ALIGN_MIDDLE);

    int y = 44;
    for (unsigned i = 0; i < sizeof(list) / sizeof(list[0]); i++) {
        const epd_font_t *f = list[i].f;
        epd_gfx_text(c, 12, y, f->name, &epd_font_inter_regular_12, EPD_RED);
        epd_gfx_text(c, 170, y, f->line_height > 35 ? SAMPLE_SHORT : SAMPLE, f, list[i].col);
        y += f->line_height + 2;
    }
}

void showcase_draw_shapes(epd_canvas_t *c)
{
    epd_gfx_clear(c, EPD_WHITE);

    /* colour swatches */
    static const struct { epd_color_t col, txt; const char *name; } sw[] = {
        { EPD_BLACK, EPD_WHITE, "BLACK" }, { EPD_WHITE, EPD_BLACK, "WHITE" },
        { EPD_YELLOW, EPD_BLACK, "YELLOW" }, { EPD_RED, EPD_WHITE, "RED" },
    };
    for (int i = 0; i < 4; i++) {
        int x = 12 + i * 100;
        epd_gfx_fill_rect(c, x, 12, 92, 60, sw[i].col);
        epd_gfx_rect(c, x, 12, 92, 60, EPD_BLACK);
        epd_gfx_text_box(c, x, 12, 92, 60, sw[i].name, &epd_font_inter_bold_16, sw[i].txt,
                         EPD_ALIGN_CENTER, EPD_ALIGN_MIDDLE);
    }

    /* patterns */
    static const char *pn[] = { "CHECKER", "DOTS", "SPARSE", "HLINES", "VLINES", "DIAGONAL" };
    for (int i = 0; i < 6; i++) {
        int x = 12 + (i % 3) * 132, y = 84 + (i / 3) * 78;
        epd_gfx_fill_rect_pattern(c, x, y, 124, 54, EPD_BLACK, EPD_YELLOW, (epd_pattern_t)i);
        epd_gfx_rect(c, x, y, 124, 54, EPD_BLACK);
        epd_gfx_text(c, x, y + 56, pn[i], &epd_font_inter_regular_12, EPD_BLACK);
    }

    /* shapes */
    epd_gfx_circle(c, 470, 50, 36, EPD_BLACK);
    epd_gfx_fill_circle(c, 560, 50, 36, EPD_RED);
    epd_gfx_fill_round_rect(c, 612, 14, 80, 72, 16, EPD_YELLOW);
    epd_gfx_round_rect(c, 612, 14, 80, 72, 16, EPD_BLACK);
    epd_gfx_fill_triangle(c, 710, 86, 750, 14, 790, 86, EPD_BLACK);
    epd_gfx_rect_thick(c, 430, 104, 120, 60, 6, EPD_RED);
    epd_gfx_triangle(c, 570, 164, 620, 104, 670, 164, EPD_RED);
    for (int i = 0; i < 6; i++)
        epd_gfx_line_thick(c, 690, 104 + i * 12, 790, 164 - i * 12, 1 + i / 2, EPD_BLACK);

    /* chart-ish lines */
    epd_gfx_hline_dashed(c, 430, 190, 360, 6, 4, EPD_RED);
    epd_gfx_hline_dashed(c, 430, 210, 360, 2, 2, EPD_BLACK);
    epd_gfx_vline_dashed(c, 790, 180, 40, 4, 4, EPD_BLACK);

    /* text alignment inside a box */
    int bx = 12, by = 250, bw = 396, bh = 96;
    epd_gfx_rect(c, bx, by, bw, bh, EPD_BLACK);
    epd_gfx_text_box(c, bx + 6, by + 4, bw - 12, bh - 8, "vlevo nahoře", &epd_font_inter_regular_16,
                     EPD_BLACK, EPD_ALIGN_LEFT, EPD_ALIGN_TOP);
    epd_gfx_text_box(c, bx, by, bw, bh, "STŘED", &epd_font_inter_bold_32, EPD_RED,
                     EPD_ALIGN_CENTER, EPD_ALIGN_MIDDLE);
    epd_gfx_text_box(c, bx + 6, by + 4, bw - 12, bh - 8, "vpravo dole", &epd_font_inter_regular_16,
                     EPD_BLACK, EPD_ALIGN_RIGHT, EPD_ALIGN_BOTTOM);
    epd_gfx_text_aligned(c, 210, 356, "Víc řádků\nzarovnaných na střed\n'\\n' funguje",
                         &epd_font_inter_regular_20, EPD_BLACK, EPD_ALIGN_CENTER);

    /* big digits */
    epd_gfx_fill_rect(c, 420, 236, 370, 232, EPD_YELLOW);
    epd_gfx_text_box(c, 420, 236, 370, 170, "1250", &epd_font_digits_128, EPD_BLACK,
                     EPD_ALIGN_CENTER, EPD_ALIGN_MIDDLE);
    epd_gfx_text_box(c, 420, 400, 370, 60, "ppm CO₂ · digits_128", &epd_font_inter_bold_24, EPD_BLACK,
                     EPD_ALIGN_CENTER, EPD_ALIGN_TOP);
}
