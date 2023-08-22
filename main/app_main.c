/* Webinar demo

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "string.h"
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"

#include "esp_insights.h"
#include "esp_rmaker_utils.h"

#include <esp_console.h>
#include <esp_insights_console.h>

#include <esp_diagnostics_metrics.h>

#define BAT_TAG "battery"
#define BAT_KEY "b_level"
#define BAT_LABEL "Battery Level"
#define BAT_PATH "Battery.Level"

/* Note: Wi-Fi station credentials can be changed using CONFIG_EXAMPLE_WIFI_SSID and CONFIG_EXAMPLE_WIFI_PASSWORD */

#define METRICS_DUMP_INTERVAL_TICKS         (pdMS_TO_TICKS(5 * 1000))

#ifdef CONFIG_ESP_INSIGHTS_TRANSPORT_HTTPS
extern const char insights_auth_key_start[] asm("_binary_insights_auth_key_txt_start");
extern const char insights_auth_key_end[] asm("_binary_insights_auth_key_txt_end");
#endif

static const char *TAG = "app_main";
static uint32_t battery_level = 100; // start at 100%

// examples of custom metric
void register_battery_level()
{
    esp_diag_metrics_register(BAT_TAG, BAT_KEY, BAT_LABEL, BAT_PATH, ESP_DIAG_DATA_TYPE_UINT);
}

// add battery level to the insights data for reporting
void stage_battery_level()
{
    esp_diag_metrics_add_uint(BAT_KEY, battery_level);
    ESP_LOGI(TAG, "Battery Percent: %" PRIu32, battery_level);
}

static int battery_level_cli_handler(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Usage: battery_set [0, 100]\n");
        return -1;
    }
    uint32_t percent = atoi(argv[1]);
    if (percent > 100) {
        percent = 100; //cap this to 100
    }
    battery_level = percent;
    ESP_LOGI(TAG, "Battery Percent set to: %" PRIu32, battery_level);

    return 0;
}

static void register_set_battery_level_cli()
{
    const esp_console_cmd_t battery_cmd = {
        .command = "battery_set",
        .help = "Set battery level",
        .func = &battery_level_cli_handler,
    };
    ESP_LOGI(TAG, "Registering command: %s", battery_cmd.command);
    esp_console_cmd_register(&battery_cmd);
}

void app_main(void)
{
    /* Initialize NVS */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * esp-idf/examples/protocols/README.md for more information about this function.
     * Use CONFIG_EXAMPLE_WIFI_SSID and CONFIG_EXAMPLE_WIFI_PASSWORD to change
     * Wi-Fi credentials.
     */
    ESP_ERROR_CHECK(example_connect());


    /* This initializes SNTP for time synchronization.
     * ESP Insights uses relative time since bootup if time is not synchronized and
     * epoch since 1970 if time is synsynchronized.
     */
    esp_rmaker_time_sync_init(NULL);

    esp_insights_config_t config = {
        .log_type = ESP_DIAG_LOG_TYPE_ERROR | ESP_DIAG_LOG_TYPE_WARNING | ESP_DIAG_LOG_TYPE_EVENT,
#ifdef CONFIG_ESP_INSIGHTS_TRANSPORT_HTTPS
        .auth_key = insights_auth_key_start,
#endif
        .alloc_ext_ram = true,
    };
    ret = esp_insights_init(&config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize ESP Insights, err:0x%x", ret);
    }
    ESP_ERROR_CHECK(ret);

    /* Following code generates an example error and logs it */
    nvs_handle_t handle;
    ret = nvs_open("unknown", NVS_READONLY, &handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Test error: API nvs_open() failed, error:0x%x", ret);
    }

#if 1 // custom metrics
    // initialize console commands for debugging
    esp_insights_console_init();
    register_set_battery_level_cli(); // set battery %

    // register a custom metric
    register_battery_level();

    /**
     * Periodically add custom metric data
     */
    int staged_cnt = 0;
    while (true) {
        stage_battery_level();
        uint32_t to_reduce = rand() % 5;
        if (staged_cnt++ % 5) {
            if (battery_level >= to_reduce) {
                battery_level -= to_reduce; // reduce level after 5 reportings
            } else {
                battery_level = 0;
            }
        }
        if (battery_level <= 5) {
            ESP_LOGW(TAG, "battery is running low (at %" PRIu32 " %%), please charge", battery_level);
            // esp_rmaker_reboot(4);
        }
        vTaskDelay(METRICS_DUMP_INTERVAL_TICKS);
    }
#endif

}
