#include "lvgl.h"
#include "bsp_lcd.h"


void ui_acquire(void);
void ui_release(void);
esp_err_t app_lvgl_start(void);
void generate_list_items(char*, char*);
void clear_screen(void);
