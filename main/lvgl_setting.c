#include "lvgl_setting.h"
#include "iot_button.h"
#include "esp_lcd_types.h"
#include "esp_check.h"
#include "esp_err.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch.h"
#include "esp_lcd_ili9341.h"
#include "lv_hal.h"
#include "lv_disp.h"
#include "button.h"

/* LCD size */
#define LCD_H_RES                   (160)
#define LCD_V_RES                   (128)

/* LCD settings */
#define LCD_SPI_NUM                 (SPI2_HOST)
#define LCD_PIXEL_CLK_HZ            (40 * 1000 * 1000)
#define LCD_CMD_BITS                (8)
#define LCD_PARAM_BITS              (8)
#define LCD_COLOR_SPACE             (ESP_LCD_COLOR_SPACE_BGR)
#define LCD_BITS_PER_PIXEL          (16)
#define LCD_DRAW_BUFF_DOUBLE        (1)
#define LCD_DRAW_BUFF_HEIGHT        (50)
#define LCD_BL_ON_LEVEL             (1)

/* LCD pins */
#define LCD_GPIO_SCLK               (GPIO_NUM_19)
#define LCD_GPIO_MOSI               (GPIO_NUM_23)
#define LCD_GPIO_RST                (GPIO_NUM_18)
#define LCD_GPIO_DC                 (GPIO_NUM_21)
#define LCD_GPIO_CS                 (GPIO_NUM_22)
#define LCD_GPIO_BL                 (GPIO_NUM_NC)

#define LCD_BITS_PER_PIXEL          (16)

static const char *TAG = "LVGL Setting";

// /* Buttons */
// typedef enum {
//     BSP_BUTTON_PREV,
//     BSP_BUTTON_ENTER,
//     BSP_BUTTON_NEXT,
//     BSP_BUTTON_NUM
// } bsp_button_t;

lv_indev_t * indev_button;

// /* Buttons */
// static const button_config_t bsp_button_config[BSP_BUTTON_NUM] = {
//     {
//         .type = BUTTON_TYPE_GPIO,
//         .gpio_button_config.active_level = false,
//         .gpio_button_config.gpio_num = GPIO_NUM_0,
//     },
//     {
//         .type = BUTTON_TYPE_GPIO,
//         .gpio_button_config.active_level = false,
//         .gpio_button_config.gpio_num = GPIO_NUM_1,        
//     },
//     {
//         .type = BUTTON_TYPE_GPIO,
//         .gpio_button_config.active_level = false,
//         .gpio_button_config.gpio_num = GPIO_NUM_2,
//     },
// };


static lv_disp_t *bsp_display_lcd_init(void);
static esp_err_t bsp_display_new(int max_transfer_sz, esp_lcd_panel_handle_t *ret_panel, esp_lcd_panel_io_handle_t *ret_io);
static lv_disp_t *bsp_display_start_with_config(const lvgl_port_cfg_t *lvgl_port_cfg);
static void button_read(lv_indev_drv_t *indev, lv_indev_data_t *data);
static void bsp_port_indev_init(void);


void initGUI(void)
{
    lvgl_port_cfg_t lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_port_cfg.task_affinity = 1;
    bsp_display_start_with_config(&lvgl_port_cfg);
}

static lv_disp_t *bsp_display_start_with_config(const lvgl_port_cfg_t *lvgl_port_cfg)
{
    lv_disp_t *disp;
    assert(lvgl_port_cfg != NULL);
    if(lvgl_port_init(lvgl_port_cfg) != ESP_OK) return NULL;
    if((disp = bsp_display_lcd_init()) == NULL) return NULL;    
    bsp_port_indev_init();
    lv_disp_set_rotation(disp, LV_DISP_ROT_180);
    return disp;
}

static lv_disp_t *bsp_display_lcd_init(void)
{
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_handle_t panel_handle = NULL;
    int max_transfer_sz = (LCD_H_RES * 10) * sizeof(uint16_t);
    if(bsp_display_new(max_transfer_sz, &panel_handle, &io_handle) != ESP_OK) return NULL;

    esp_lcd_panel_disp_on_off(panel_handle, true);

    /* Add LCD screen */
    ESP_LOGD(TAG, "Add LCD screen");
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = LCD_H_RES * 10,
#if CONFIG_BSP_LCD_DRAW_BUF_DOUBLE
        .double_buffer = 1,
#else
        .double_buffer = 0,
#endif
        .hres = LCD_H_RES,
        .vres = LCD_V_RES,
        .monochrome = false,
        /* Rotation values must be same as used in esp_lcd for initial settings of the screen */
        .rotation = {
            .swap_xy = true,
            .mirror_x = false,
            .mirror_y = true,
        },
        .flags = {
            .buff_dma = true,
        }
    };

    return lvgl_port_add_disp(&disp_cfg);
}

static esp_err_t bsp_display_new(int max_transfer_sz, esp_lcd_panel_handle_t *ret_panel, esp_lcd_panel_io_handle_t *ret_io)
{
    esp_err_t ret = ESP_OK;
    assert(max_transfer_sz > 0);

    ESP_LOGD(TAG, "Initialize SPI bus");
    const spi_bus_config_t buscfg = {
        .sclk_io_num = LCD_GPIO_SCLK,
        .mosi_io_num = LCD_GPIO_MOSI,
        .miso_io_num = GPIO_NUM_NC,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = max_transfer_sz,
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(LCD_SPI_NUM, &buscfg, SPI_DMA_CH_AUTO), TAG, "SPI init failed");

    ESP_LOGD(TAG, "Install panel IO");
    const esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = LCD_GPIO_DC, 
        .cs_gpio_num = LCD_GPIO_CS,
        .pclk_hz = LCD_PIXEL_CLK_HZ,
        .lcd_cmd_bits = LCD_CMD_BITS,
        .lcd_param_bits = LCD_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = 7,
    };
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_SPI_NUM, &io_config, ret_io), err, TAG, "New panel IO failed");

    ESP_LOGD(TAG, "Install LCD driver");
    const esp_lcd_panel_dev_config_t panel_config = {
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .data_endian = LCD_RGB_DATA_ENDIAN_BIG,
        .reset_gpio_num = LCD_GPIO_RST,
        .bits_per_pixel = LCD_BITS_PER_PIXEL,
    };
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_ili9341(*ret_io, &panel_config, ret_panel), err, TAG, "New panel failed");

    esp_lcd_panel_reset(*ret_panel);
    esp_lcd_panel_init(*ret_panel);
    esp_lcd_panel_mirror(*ret_panel, false, true);
    esp_lcd_panel_swap_xy(*ret_panel, true);
    esp_lcd_panel_invert_color(*ret_panel, false);
    return ret;

err:
    if (*ret_panel) {
        esp_lcd_panel_del(*ret_panel);
    }
    if (*ret_io) {
        esp_lcd_panel_io_del(*ret_io);
    }
    spi_bus_free(LCD_SPI_NUM);
    return ret;
}

static void bsp_port_indev_init(void)
{
    static lv_indev_drv_t indev_drv;
    /*Initialize button */
    initButton();

    /*Register a button input device*/
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_BUTTON;
    indev_drv.read_cb = button_read;
    indev_button = lv_indev_drv_register(&indev_drv);

    /*Assign buttons to points on the screen*/
    static const lv_point_t btn_points[3] = {
        {20, 55},   
        {80, 64},  
        {135, 55},   
    };
    lv_indev_set_button_points(indev_button, btn_points);
}

static void button_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
    static uint32_t last_btn = 0;   
    int btn_pr = detectButton();     
    if(btn_pr >= 0) {               
       last_btn = btn_pr;           
       data->state = LV_INDEV_STATE_PRESSED;  
    } else {
       data->state = LV_INDEV_STATE_RELEASED; 
    }

    data->btn_id = last_btn;        
}
