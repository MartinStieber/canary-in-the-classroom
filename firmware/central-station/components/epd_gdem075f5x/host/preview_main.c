/*
 * Host preview: renders the example screens to BMP files, so you can design the
 * layout on a PC without waiting 20 s for every panel refresh.
 *
 *   cd host && make && ./preview            -> preview_*.bmp
 *
 * To preview your own screen, add your drawing .c file to SRC in the Makefile and
 * call it from here.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "epd_gfx.h"
#include "epd_host.h"
#include "showcase_ui.h"

static int save(epd_canvas_t *c, const char *name, int scale)
{
    if (epd_host_save_bmp(c, name, scale) != 0) {
        fprintf(stderr, "cannot write %s\n", name);
        return 1;
    }
    printf("wrote %s\n", name);
    return 0;
}

int main(int argc, char **argv)
{
    int scale = argc > 1 ? atoi(argv[1]) : 1;
    epd_canvas_t *c = epd_canvas_create(EPD_ROT_0);
    if (!c) return 1;
    int err = 0;

    showcase_draw_fonts(c);
    err |= save(c, "preview_fonts.bmp", scale);

    showcase_draw_shapes(c);
    err |= save(c, "preview_shapes.bmp", scale);

    epd_canvas_free(c);
    return err;
}
