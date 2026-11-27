#include "epd_host.h"

#include <stdio.h>
#include <stdlib.h>

static void le16(FILE *f, unsigned v) { fputc(v & 0xFF, f); fputc((v >> 8) & 0xFF, f); }
static void le32(FILE *f, unsigned v) { le16(f, v & 0xFFFF); le16(f, v >> 16); }

/* Approximate colours of the BWRY ink, indexed by epd_color_t */
static const unsigned char PALETTE[4][3] = {
    { 0x22, 0x22, 0x22 },   /* black  */
    { 0xEE, 0xEC, 0xE4 },   /* white (paper) */
    { 0xE8, 0xC2, 0x1C },   /* yellow */
    { 0xB8, 0x2A, 0x2A },   /* red    */
};

int epd_host_save_bmp(const epd_canvas_t *c, const char *path, int scale)
{
    if (scale < 1) scale = 1;
    int w = EPD_WIDTH * scale, h = EPD_HEIGHT * scale;
    int stride = (w + 3) & ~3;
    unsigned data_size = (unsigned)stride * h, offset = 14 + 40 + 256 * 4;

    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    fputc('B', f); fputc('M', f);
    le32(f, offset + data_size); le32(f, 0); le32(f, offset);
    le32(f, 40); le32(f, w); le32(f, h); le16(f, 1); le16(f, 8);
    le32(f, 0); le32(f, data_size); le32(f, 4724); le32(f, 4724); le32(f, 256); le32(f, 4);
    for (int i = 0; i < 256; i++) {
        const unsigned char *p = PALETTE[i < 4 ? i : 0];
        fputc(p[2], f); fputc(p[1], f); fputc(p[0], f); fputc(0, f);
    }
    unsigned char *row = calloc(1, stride);
    for (int y = h - 1; y >= 0; y--) {              /* BMP rows go bottom-up */
        int py = y / scale;
        for (int x = 0; x < w; x++) {
            int px = x / scale;
            size_t idx = ((size_t)py * EPD_WIDTH + px) >> 2;
            row[x] = (c->fb[idx] >> (6 - 2 * (px & 3))) & 0x3;
        }
        fwrite(row, 1, stride, f);
    }
    free(row);
    fclose(f);
    return 0;
}
