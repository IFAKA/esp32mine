idf_component_register(
    SRCS "main.cpp" "../../src/game.cpp"
    INCLUDE_DIRS "." "../../src"
    REQUIRES esp_lcd esp_timer esp_driver_uart
)

target_compile_options(${COMPONENT_LIB} PRIVATE -ffunction-sections -fdata-sections)
