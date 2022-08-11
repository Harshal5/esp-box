#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "freertos/semphr.h"
#include <esp_log.h>
#include <esp_event.h>
#include <esp_heap_caps.h>
#include "app_wifi.h"
#include "app_insights.h"

#include "rmaker_main.h"
#include "ui_main.h"
#include "cJSON.h"

esp_rmaker_device_t *conf_room_device;

static const char *TAG = "rmaker_main";

/* Callback to handle commands received from the RainMaker cloud */
esp_err_t write_cb(const esp_rmaker_device_t *device, const esp_rmaker_param_t *param,
            const esp_rmaker_param_val_t val, void *priv_data, esp_rmaker_write_ctx_t *ctx)
{
    if (ctx) {
        ESP_LOGI(TAG, "Received write request via : %s", esp_rmaker_device_cb_src_to_str(ctx->src));
    }

    if (strcmp(esp_rmaker_param_get_name(param), "Calendar") == 0)
    {
        ESP_LOGI(TAG, "Received value = %s ", val.val.s);

        ui_acquire();

        ESP_LOGI(TAG, "Before clear: free heap = %d\n", heap_caps_get_free_size(MALLOC_CAP_8BIT));
        clear_screen();
        ESP_LOGI(TAG, "After clear: free heap = %d\n", heap_caps_get_free_size(MALLOC_CAP_8BIT));
        
        // Calendar parameter values parsing using cJSON
        cJSON *json = cJSON_Parse(val.val.s);
        const cJSON *meetings = cJSON_GetObjectItemCaseSensitive(json, "value");
        const cJSON *meeting = NULL; 
        cJSON_ArrayForEach(meeting, meetings) {
            const cJSON *startTime = cJSON_GetObjectItemCaseSensitive(meeting, "startTime");
            const cJSON *endTime = cJSON_GetObjectItemCaseSensitive(meeting, "endTime");
            const cJSON *organizer = cJSON_GetObjectItemCaseSensitive(meeting, "organizer");

            char time[14];
            memcpy(time, startTime->valuestring, strlen(startTime->valuestring));
            memcpy(&time[5], " - ", 3);
            strcpy(&time[8], endTime->valuestring);

            generate_list_items(time, organizer->valuestring);
            // ESP_LOGI(TAG, "StartTime: %s, EndTime: %s, Organizer: %s", startTime->valuestring, endTime->valuestring, organizer->valuestring);
        } 
        cJSON_Delete(json);
        ui_release();
        ESP_LOGI(TAG, "After new items: free heap = %d\n", heap_caps_get_free_size(MALLOC_CAP_8BIT));
    }
    esp_rmaker_param_update_and_report(param, val);
    return ESP_OK;
}

/* Event handler for catching RainMaker events */
void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
    if (event_base == RMAKER_EVENT) {
        switch (event_id) {
            case RMAKER_EVENT_INIT_DONE:
                ESP_LOGI(TAG, "RainMaker Initialised.");
                break;
            case RMAKER_EVENT_CLAIM_STARTED:
                ESP_LOGI(TAG, "RainMaker Claim Started.");
                break;
            case RMAKER_EVENT_CLAIM_SUCCESSFUL:
                ESP_LOGI(TAG, "RainMaker Claim Successful.");
                break;
            case RMAKER_EVENT_CLAIM_FAILED:
                ESP_LOGI(TAG, "RainMaker Claim Failed.");
                break;
            default:
                ESP_LOGW(TAG, "Unhandled RainMaker Event: %d", event_id);
        }
    } else if (event_base == RMAKER_COMMON_EVENT) {
        switch (event_id) {
            case RMAKER_EVENT_REBOOT:
                ESP_LOGI(TAG, "Rebooting in %d seconds.", *((uint8_t *)event_data));
                break;
            case RMAKER_EVENT_WIFI_RESET:
                ESP_LOGI(TAG, "Wi-Fi credentials reset.");
                break;
            case RMAKER_EVENT_FACTORY_RESET:
                ESP_LOGI(TAG, "Node reset to factory defaults.");
                break;
            case RMAKER_MQTT_EVENT_CONNECTED:
                ESP_LOGI(TAG, "MQTT Connected.");
                break;
            case RMAKER_MQTT_EVENT_DISCONNECTED:
                ESP_LOGI(TAG, "MQTT Disconnected.");
                break;
            case RMAKER_MQTT_EVENT_PUBLISHED:
                ESP_LOGI(TAG, "MQTT Published. Msg id: %d.", *((int *)event_data));
                break;
            default:
                ESP_LOGW(TAG, "Unhandled RainMaker Common Event: %d", event_id);
        }
    } else {
        ESP_LOGW(TAG, "Invalid event received!");
    }
}

void app_rmaker_start(void)
{
    /* Initialize Wi-Fi. Note that, this should be called before esp_rmaker_node_init() */
    app_wifi_init();

    /* Register an event handler to catch RainMaker events */
    ESP_ERROR_CHECK(esp_event_handler_register(RMAKER_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));

    /* Initialize the ESP RainMaker Agent. */
    esp_rmaker_config_t rainmaker_cfg = {
        .enable_time_sync = false,
    };
    esp_rmaker_node_t *node = esp_rmaker_node_init(&rainmaker_cfg, "ESP RainMaker Device", "conf-room");
    if (!node) {
        ESP_LOGE(TAG, "Could not initialise node. Aborting!!!");
        vTaskDelay(5000/portTICK_PERIOD_MS);
        abort();
    }
    
    /* Create a conf-room device. */
    conf_room_device = esp_rmaker_device_create("conf-room", ESP_RMAKER_DEVICE_OTHER, NULL);
    esp_rmaker_device_add_cb(conf_room_device, write_cb, NULL);
    esp_rmaker_device_add_param(conf_room_device, esp_rmaker_name_param_create(ESP_RMAKER_DEF_NAME_PARAM, "conf-room"));
    
    /* Adding the Calendar Parameter */
    esp_rmaker_param_t *calendar_param = esp_rmaker_param_create("Calendar", NULL, esp_rmaker_obj("{}"), 0);
    esp_rmaker_device_add_param(conf_room_device, calendar_param);
    esp_rmaker_device_assign_primary_param(conf_room_device, calendar_param);
    esp_rmaker_node_add_device(node, conf_room_device);

    /* Enable OTA */
    esp_rmaker_ota_config_t ota_config = {
        .server_cert = ESP_RMAKER_OTA_DEFAULT_SERVER_CERT,
    };
    esp_rmaker_ota_enable(&ota_config, OTA_USING_PARAMS);

    /* Start the ESP RainMaker Agent */
    esp_rmaker_start();

    /* Start the Wi-Fi. */
    esp_err_t err = app_wifi_start(POP_TYPE_RANDOM);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Could not start Wifi. Aborting!!!");
        vTaskDelay(5000/portTICK_PERIOD_MS);
        abort();
    }

    /* Enable Insights. Requires CONFIG_ESP_INSIGHTS_ENABLED=y */
    app_insights_enable();
}
