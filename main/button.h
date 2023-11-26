#ifndef _BUTTON_H_
#define _BUTTON_H_

#include "driver/gpio.h"

#define BUTTON_DOWN     34
#define BUTTON_UP       35
#define BUTTON_ENTER    39
#define DEBOUNCE        50
#define NO_WAIT_TIME    0
extern QueueHandle_t buttonMessageQueue;

void initButton(void);
// void buttonTask(void * pvParameters);
gpio_num_t detectButton(void);


#endif // _BUTTON_H_