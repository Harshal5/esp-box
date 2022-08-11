#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "freertos/semphr.h"
#include <esp_log.h>
#include <esp_event.h>

#include "ui_main.h"

static TaskHandle_t g_lvgl_task_handle;
SemaphoreHandle_t g_guisemaphore;

/* LVGL Static Style Variables */
static lv_style_t style_cont_obj;
static lv_style_t style_list_obj;
static lv_style_t style_organizer_label;
static lv_style_t style_time_label;

static lv_obj_t * cont = NULL;

void ui_acquire(void)
{
    TaskHandle_t task = xTaskGetCurrentTaskHandle();
    if (g_lvgl_task_handle != task) {
        xSemaphoreTake(g_guisemaphore, portMAX_DELAY);
    }
}

void ui_release(void)
{
    TaskHandle_t task = xTaskGetCurrentTaskHandle();
    if (g_lvgl_task_handle != task) {
        xSemaphoreGive(g_guisemaphore);
    }
}


static void ui_init()
{
    lv_obj_t* title_obj = lv_obj_create(lv_layer_top());
    lv_obj_set_size(title_obj, lv_pct(100), 50);
    lv_obj_set_scrollbar_mode(title_obj, LV_SCROLLBAR_MODE_OFF);

    static lv_style_t style_title_obj;
    lv_style_init(&style_title_obj);
    lv_style_set_bg_color(&style_title_obj, lv_color_hex(0xFFEA00));
    lv_style_set_bg_opa(&style_title_obj, LV_OPA_100);
    lv_obj_add_style(title_obj, &style_title_obj, 0);
    
    lv_obj_t* title = lv_label_create(title_obj);
    lv_label_set_text(title, "Gir");

    static lv_style_t style_title;
    lv_style_init(&style_title);
    lv_style_set_text_color(&style_title, lv_color_hex(0x333333));
    lv_style_set_text_font(&style_title, &lv_font_montserrat_28);
    lv_style_set_align(&style_title, LV_ALIGN_CENTER);
    lv_obj_add_style(title, &style_title, 0);

    cont = lv_scr_act();

    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_style_init(&style_cont_obj);
    lv_style_set_pad_top(&style_cont_obj, 55);
    lv_obj_add_style(cont, &style_cont_obj, 0);

    lv_style_init(&style_list_obj);
    lv_style_set_radius(&style_list_obj, 0);
    lv_style_set_text_font(&style_list_obj, &lv_font_montserrat_18);
    lv_style_set_text_color(&style_list_obj, lv_color_hex(0x333333));
    lv_style_set_pad_row(&style_list_obj, 10);
    lv_style_set_pad_top(&style_list_obj, 10);
    lv_style_set_pad_left(&style_list_obj, 20);
    lv_style_set_pad_bottom(&style_list_obj, 5);
    lv_style_set_bg_opa(&style_list_obj, LV_OPA_TRANSP);
    lv_style_set_shadow_width(&style_list_obj, 5);
    lv_style_set_shadow_opa(&style_list_obj, LV_OPA_50);

    lv_style_init(&style_organizer_label);
    lv_style_set_text_font(&style_organizer_label, &lv_font_montserrat_20);

    lv_style_init(&style_time_label);
    lv_style_set_text_font(&style_time_label, &lv_font_montserrat_18);
    lv_style_set_border_color(&style_time_label, lv_palette_main(LV_PALETTE_RED));
    lv_style_set_border_side(&style_time_label, LV_BORDER_SIDE_BOTTOM);
    lv_style_set_border_width(&style_time_label, 2);
}

static void lvgl_task(void *pvParam)
{
    bsp_lcd_set_backlight(true);
    ui_init();
    g_guisemaphore = xSemaphoreCreateMutex();
    do {
        /* Try to take the semaphore, call lvgl related function on success */
        if (pdTRUE == xSemaphoreTake(g_guisemaphore, portMAX_DELAY)) {
            lv_task_handler();
            xSemaphoreGive(g_guisemaphore);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    } while (true);
    vTaskDelete(NULL);
}

esp_err_t app_lvgl_start(void)
{
    BaseType_t ret_val = xTaskCreatePinnedToCore(
                             (TaskFunction_t)        lvgl_task,
                             (const char *const)    "lvgl Task",
                             (const uint32_t)        5 * 1024,
                             (void *const)           NULL,
                             (UBaseType_t)           ESP_TASK_PRIO_MIN + 1,
                             (TaskHandle_t *const)   &g_lvgl_task_handle,
                             (const BaseType_t)      0);
    ESP_ERROR_CHECK(ret_val == pdPASS ? ESP_OK : ESP_FAIL);
    return ESP_OK;
}

void generate_list_items(char* time, char* organizer_name)
{
    lv_obj_t *list_obj = lv_list_create(cont);
    lv_obj_align(list_obj, LV_ALIGN_CENTER, 0, 50);
    lv_obj_set_size(list_obj, lv_pct(100), lv_pct(47));
    lv_obj_center(list_obj);
    lv_obj_set_flex_flow(list_obj, LV_FLEX_FLOW_COLUMN);
    lv_obj_add_style(list_obj, &style_list_obj, 0);

    lv_obj_t * time_label = lv_label_create(list_obj);
    lv_label_set_text(time_label, time);
    lv_obj_add_style(time_label, &style_time_label, 0);

    lv_obj_t * organizer_label = lv_label_create(list_obj);
    lv_label_set_text(organizer_label, organizer_name);
    lv_label_set_long_mode(organizer_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_add_style(organizer_label, &style_organizer_label, 0);
}

void clear_screen (void)
{
    lv_obj_clean(cont);
}
