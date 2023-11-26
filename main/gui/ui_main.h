#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_style_t style;
    lv_style_t style_focus_no_outline;
    lv_style_t style_focus;
    lv_style_t style_pr;
} button_style_t;


esp_err_t ui_main_start(void);
void ui_main_status_bar_set_wifi(bool is_connected);
button_style_t *ui_button_styles(void);
lv_obj_t *ui_main_get_status_bar(void);






#ifdef __cplusplus
}
#endif