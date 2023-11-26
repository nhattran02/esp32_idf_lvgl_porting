#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_check.h"
#include "lvgl.h"
#include "ui_main.h"
#include "esp_lvgl_port.h"
#include "lv_symbol_extra_def.h"
#include "ui_boot_animate.h"


static const char *TAG = "ui_main";

// static void ui_help(void (*fn)(void))
// {
//     ui_hint_start(hint_end_cb);
// }



typedef struct {
    char *name;
    void *img_src;
    void (*start_fn)(void (*fn)(void));
    void (*end_fn)(void);
} item_desc_t;

LV_IMG_DECLARE(icon_about_us)
LV_IMG_DECLARE(icon_dev_ctrl)
LV_IMG_DECLARE(icon_media_player)
LV_IMG_DECLARE(icon_help)
LV_IMG_DECLARE(icon_network)

static item_desc_t item[] = {
    { "Device Control", (void *) &icon_dev_ctrl,        NULL/*ui_device_ctrl_start*/, /*dev_ctrl_end_cb*/NULL},
    { "Network",        (void *) &icon_network,         NULL/*ui_net_config_start*/, /*net_end_cb*/NULL},
    { "Media Player",   (void *) &icon_media_player,    NULL/*ui_media_player*/, /*player_end_cb*/NULL}, 
    { "Help",           (void *) &icon_help,            NULL/*ui_help*/, NULL},
    { "About Us",       (void *) &icon_about_us,        NULL/*ui_about_us_start*/, NULL},
};

static int g_item_index = 0;
static button_style_t g_btn_styles;
static lv_obj_t *g_status_bar = NULL;
static lv_obj_t *g_lab_wifi = NULL;
static lv_obj_t *g_page_menu = NULL;
static lv_obj_t *g_img_btn, *g_img_item = NULL;
static lv_obj_t *g_lab_item = NULL;
static lv_obj_t *g_led_item[5];
static size_t g_item_size = sizeof(item) / sizeof(item[0]);
static lv_obj_t *g_group_list[3] = {0};


void ui_acquire(void)
{
    lvgl_port_lock(0);
}

void ui_release(void)
{
    lvgl_port_unlock();
}
static void clock_run_cb(lv_timer_t *timer)
{
    lv_obj_t *lab_time = (lv_obj_t *) timer->user_data;
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    lv_label_set_text_fmt(lab_time, "%02u:%02u", timeinfo.tm_hour, timeinfo.tm_min);
}
static void ui_button_style_init(void)
{
    /*Init the style for the default state*/

    lv_style_init(&g_btn_styles.style);

    lv_style_set_radius(&g_btn_styles.style, 5);
    lv_style_set_bg_color(&g_btn_styles.style, lv_color_make(255, 255, 255));

    lv_style_set_border_opa(&g_btn_styles.style, LV_OPA_30);
    lv_style_set_border_width(&g_btn_styles.style, 2);
    lv_style_set_border_color(&g_btn_styles.style, lv_palette_main(LV_PALETTE_GREY));

    lv_style_set_shadow_width(&g_btn_styles.style, 7);
    lv_style_set_shadow_color(&g_btn_styles.style, lv_color_make(0, 0, 0));
    lv_style_set_shadow_ofs_x(&g_btn_styles.style, 0);
    lv_style_set_shadow_ofs_y(&g_btn_styles.style, 0);

    lv_style_init(&g_btn_styles.style_pr);

    lv_style_set_border_opa(&g_btn_styles.style_pr, LV_OPA_40);
    lv_style_set_border_width(&g_btn_styles.style_pr, 2);
    lv_style_set_border_color(&g_btn_styles.style_pr, lv_palette_main(LV_PALETTE_GREY));

    lv_style_init(&g_btn_styles.style_focus);
    lv_style_set_outline_color(&g_btn_styles.style_focus, lv_color_make(255, 0, 0));

    lv_style_init(&g_btn_styles.style_focus_no_outline);
    lv_style_set_outline_width(&g_btn_styles.style_focus_no_outline, 0);
}
static void ui_status_bar_set_visible(bool visible)
{
    if (visible) {
        // update all state
        ui_main_status_bar_set_wifi(1); /* app_wifi_is_connected()) */
        lv_obj_clear_flag(g_status_bar, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(g_status_bar, LV_OBJ_FLAG_HIDDEN);
    }
}

void ui_main_status_bar_set_wifi(bool is_connected)
{
    if (g_lab_wifi) {
        lv_label_set_text_static(g_lab_wifi, is_connected ? LV_SYMBOL_WIFI : LV_SYMBOL_EXTRA_WIFI_OFF);
        lv_obj_set_style_text_font(g_lab_wifi, &lv_font_montserrat_12, LV_PART_MAIN);
    }
}
button_style_t *ui_button_styles(void)
{
    return &g_btn_styles;
}

lv_obj_t *ui_main_get_status_bar(void)
{
    return g_status_bar;
}
static void menu_prev_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (LV_EVENT_RELEASED == code) {
        lv_led_off(g_led_item[g_item_index]);
        if (0 == g_item_index) {
            g_item_index = g_item_size;
        }
        g_item_index--;
        lv_led_on(g_led_item[g_item_index]);
        lv_img_set_src(g_img_item, item[g_item_index].img_src);
        lv_label_set_text_static(g_lab_item, item[g_item_index].name);
    }
}
static void menu_next_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (LV_EVENT_RELEASED == code) {
        lv_led_off(g_led_item[g_item_index]);
        g_item_index++;
        if (g_item_index >= g_item_size) {
            g_item_index = 0;
        }
        lv_led_on(g_led_item[g_item_index]);
        lv_img_set_src(g_img_item, item[g_item_index].img_src);
        lv_label_set_text_static(g_lab_item, item[g_item_index].name);
    }
}
static void ui_main_menu(int32_t index_id)
{
    if (!g_page_menu) {
        g_page_menu = lv_obj_create(lv_scr_act());
        lv_obj_set_size(g_page_menu, lv_obj_get_width(lv_obj_get_parent(g_page_menu)), lv_obj_get_height(lv_obj_get_parent(g_page_menu)) - lv_obj_get_height(ui_main_get_status_bar()));
        lv_obj_set_style_border_width(g_page_menu, 0, LV_PART_MAIN);
        lv_obj_set_style_bg_color(g_page_menu, lv_obj_get_style_bg_color(lv_scr_act() , LV_STATE_DEFAULT), LV_PART_MAIN);
        lv_obj_clear_flag(g_page_menu, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_align_to(g_page_menu, ui_main_get_status_bar(), LV_ALIGN_OUT_BOTTOM_RIGHT, 10, 0);
    }
    ui_status_bar_set_visible(true);

    lv_obj_t *obj = lv_obj_create(g_page_menu);
    lv_obj_set_size(obj, 145, 100);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(obj, 15, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(obj, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(obj, 20, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(obj, LV_OPA_30, LV_PART_MAIN);
    lv_obj_align(obj, LV_ALIGN_TOP_MID, 0, -13);

    g_img_btn = lv_btn_create(obj);
    lv_obj_set_size(g_img_btn, 60, 60);
    lv_obj_add_style(g_img_btn, &ui_button_styles()->style_pr, LV_STATE_PRESSED);
    lv_obj_add_style(g_img_btn, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(g_img_btn, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUSED);
    lv_obj_set_style_bg_color(g_img_btn, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(g_img_btn, LV_ALIGN_CENTER, 0, -10);


    g_img_item = lv_img_create(g_img_btn);
    lv_img_set_src(g_img_item, item[index_id].img_src);
    lv_obj_set_style_shadow_color(g_img_item, lv_color_make(0, 0, 0), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(g_img_item, 15, LV_PART_MAIN);
    lv_obj_set_style_shadow_ofs_x(g_img_item, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_ofs_y(g_img_item, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(g_img_item, LV_OPA_50, LV_PART_MAIN);    
    lv_obj_center(g_img_item);

    g_lab_item = lv_label_create(obj);
    lv_label_set_text_static(g_lab_item, item[index_id].name);
    lv_obj_set_style_text_font(g_lab_item, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(g_lab_item, LV_ALIGN_CENTER, 0, 35);
    lv_obj_set_style_text_color(g_lab_item, lv_color_make(0,0,0), LV_STATE_DEFAULT);

    for (size_t i = 0; i < sizeof(g_led_item) / sizeof(g_led_item[0]); i++) {
        int gap = 5;
        if (NULL == g_led_item[i]) {
            g_led_item[i] = lv_led_create(g_page_menu);
        } else {
            lv_obj_clear_flag(g_led_item[i], LV_OBJ_FLAG_HIDDEN);
        }
        lv_led_off(g_led_item[i]);
        lv_obj_set_size(g_led_item[i], 5, 5);
        lv_obj_align_to(g_led_item[i], g_page_menu, LV_ALIGN_BOTTOM_MID, 2 * gap * i - (g_item_size - 1) * gap, 10);
    }
    lv_led_on(g_led_item[index_id]);

    lv_obj_t *btn_prev = lv_btn_create(obj);
    lv_obj_add_style(btn_prev, &ui_button_styles()->style_pr, LV_STATE_PRESSED);
    lv_obj_remove_style(btn_prev, NULL, LV_STATE_PRESSED);
    lv_obj_add_style(btn_prev, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(btn_prev, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUSED);

    lv_obj_set_size(btn_prev, 20, 20);
    lv_obj_set_style_bg_color(btn_prev, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_shadow_color(btn_prev, lv_color_make(0, 0, 0), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn_prev, 15, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(btn_prev, LV_OPA_50, LV_PART_MAIN);
    lv_obj_set_style_shadow_ofs_x(btn_prev, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_ofs_y(btn_prev, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(btn_prev, 10, LV_PART_MAIN);
    lv_obj_align_to(btn_prev, obj, LV_ALIGN_LEFT_MID, -8, 0);
    lv_obj_t *label = lv_label_create(btn_prev);
    lv_label_set_text_static(label, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(label, lv_color_make(5, 5, 5), LV_PART_MAIN);
    lv_obj_center(label);
    lv_obj_add_event_cb(btn_prev, menu_prev_cb, LV_EVENT_ALL, btn_prev);

    lv_obj_t *btn_next = lv_btn_create(obj);
    lv_obj_add_style(btn_next, &ui_button_styles()->style_pr, LV_STATE_PRESSED);
    lv_obj_remove_style(btn_next, NULL, LV_STATE_PRESSED);
    lv_obj_add_style(btn_next, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(btn_next, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUSED);
    lv_obj_set_size(btn_next, 20, 20);
    lv_obj_set_style_bg_color(btn_next, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_shadow_color(btn_next, lv_color_make(0, 0, 0), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn_next, 15, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(btn_next, LV_OPA_50, LV_PART_MAIN);
    lv_obj_set_style_shadow_ofs_x(btn_next, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_ofs_y(btn_next, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(btn_next, 10, LV_PART_MAIN);
    lv_obj_align_to(btn_next, obj, LV_ALIGN_RIGHT_MID, 8, 0);
    label = lv_label_create(btn_next);
    lv_label_set_text_static(label, LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_color(label, lv_color_make(5, 5, 5), LV_PART_MAIN);
    lv_obj_center(label);
    lv_obj_add_event_cb(btn_next, menu_next_cb, LV_EVENT_ALL, btn_next);    


    
}

static void ui_after_boot(void)
{
    ui_main_menu(g_item_index);
}

esp_err_t ui_main_start(void)
{
    ui_acquire();
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_make(237, 238, 239), LV_STATE_DEFAULT);
    ui_button_style_init();
    lv_indev_t *indev = lv_indev_get_next(NULL);

    if (lv_indev_get_type(indev) == LV_INDEV_TYPE_BUTTON) {
        ESP_LOGI(TAG, "Input device type have button");
    }

    // Create status bar
    g_status_bar = lv_obj_create(lv_scr_act());
    lv_obj_set_size(g_status_bar, lv_obj_get_width(lv_obj_get_parent(g_status_bar)), 15);
    lv_obj_clear_flag(g_status_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(g_status_bar, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(g_status_bar, lv_obj_get_style_bg_color(lv_scr_act(), LV_STATE_DEFAULT), LV_PART_MAIN);
    lv_obj_set_style_border_width(g_status_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(g_status_bar, 0, LV_PART_MAIN);
    lv_obj_align(g_status_bar, LV_ALIGN_TOP_LEFT, -10, 0);

    lv_obj_t *lab_time = lv_label_create(g_status_bar);
    lv_label_set_text_static(lab_time, "23:59");
    lv_obj_set_style_text_font(lab_time, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_align(lab_time, LV_ALIGN_LEFT_MID, 0, 0);
    lv_timer_t *timer = lv_timer_create(clock_run_cb, 1000, (void *) lab_time);
    clock_run_cb(timer);
    
    g_lab_wifi = lv_label_create(g_status_bar);
    lv_obj_align_to(g_lab_wifi, lab_time, LV_ALIGN_OUT_RIGHT_MID, 8, 0);

    ui_status_bar_set_visible(0);
    boot_animate_start(ui_after_boot);

    ui_release();
    return ESP_OK;
}