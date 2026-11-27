idf_component_register(
    SRCS "src/dashboard_ui.cpp"         # třída Dashboard, obsluha displeje
         "src/dashboard_data.cpp"       # hodnoty učeben
         "src/dashboard_render.cpp"     # kreslení do framebufferu
         "src/dashboard_demo.cpp"       # testovací data
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "src"
    REQUIRES epd_gdem075f5x
)
