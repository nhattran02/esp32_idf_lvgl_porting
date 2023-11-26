#include <stdio.h>
#include <sys/time.h>
#include <math.h>
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "lvgl_setting.h"
#include "button.h"
#include "gui/ui_main.h"

// static const char *TAG = "IoT Gateway";

void app_main(void)
{
    initGUI();
    ui_main_start();
}







