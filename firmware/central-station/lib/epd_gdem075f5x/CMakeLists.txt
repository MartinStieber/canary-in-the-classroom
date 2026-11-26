file(GLOB EPD_FONT_SRCS "${CMAKE_CURRENT_LIST_DIR}/fonts/*.c")

idf_component_register(
    SRCS "src/epd_gdem075f5x.c"
         "src/epd_gfx.c"
         "src/epd_text.c"
         ${EPD_FONT_SRCS}
    INCLUDE_DIRS "include"
    REQUIRES driver
    PRIV_REQUIRES esp_timer
)
