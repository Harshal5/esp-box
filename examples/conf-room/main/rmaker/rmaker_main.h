#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_types.h>
#include <esp_rmaker_standard_params.h>
#include <esp_rmaker_standard_devices.h>
#include <esp_rmaker_ota.h>

#include <esp_rmaker_common_events.h>


esp_err_t write_cb(const esp_rmaker_device_t *device, const esp_rmaker_param_t *param,
            const esp_rmaker_param_val_t val, void *priv_data, esp_rmaker_write_ctx_t *ctx);

void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data);

void app_rmaker_start(void);
