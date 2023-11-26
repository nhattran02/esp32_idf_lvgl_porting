
# file(GLOB_RECURSE SRC_UI ${CMAKE_SOURCE_DIR} "ui/*.c")
file(GLOB_RECURSE SRC_GUI ${CMAKE_SOURCE_DIR} "gui/*.c")


idf_component_register(SRCS "main.c" "lvgl_setting.c" "button.c" ${SRC_GUI}
                    INCLUDE_DIRS "." "/home/nhat/esp/myGUI/components/lvgl__lvgl/src/hal/" "/home/nhat/esp/myGUI/components/lvgl__lvgl/src/core/" "gui" 
                    REQUIRES lvgl driver esp_lcd espressif__esp_lvgl_port espressif__esp_lcd_ili9341 espressif__esp_lcd_touch lvgl__lvgl espressif__button)
