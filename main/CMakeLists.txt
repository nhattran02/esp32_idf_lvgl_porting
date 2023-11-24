idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "." "/home/nhat/esp/myGUI/components/lvgl__lvgl/src/hal/" "/home/nhat/esp/myGUI/components/lvgl__lvgl/src/core/"
                    REQUIRES lvgl driver esp_lcd espressif__esp_lvgl_port espressif__esp_lcd_ili9341 espressif__esp_lcd_touch lvgl__lvgl)
