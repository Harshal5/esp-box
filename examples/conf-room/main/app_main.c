/* conf-room Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <esp_log.h>
#include <esp_event.h>
#include <nvs_flash.h>

#include "bsp_board.h"
#include "bsp_btn.h"
#include "bsp_storage.h"
#include "lv_port.h"
#include "lv_port_fs.h"

#include "rmaker/rmaker_main.h"
#include "gui/ui_main.h"
#include "cJSON.h"

static const char *TAG = "app_main";

void app_main()
{
    /* Initialize NVS. */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );


    ESP_ERROR_CHECK(bsp_board_init());
    ESP_ERROR_CHECK(lv_port_init());

    app_rmaker_start();

    ESP_ERROR_CHECK(app_lvgl_start());

}
