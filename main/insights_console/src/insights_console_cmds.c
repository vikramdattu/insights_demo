/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <ctype.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_timer.h>
// #include <nvs_flash.h>
#include <esp_log.h>
#include <esp_console.h>
#include <esp_system.h>
#include <argtable3/argtable3.h>
#include <esp_heap_caps.h>
#ifdef CONFIG_HEAP_TRACING
#include <esp_heap_trace.h>
#endif

#include <esp_rmaker_common_console.h>
#include <esp_rmaker_utils.h>
#include <esp_rmaker_cmd_resp.h>

static const char *TAG = "esp_rmaker_commands";

static int cmd_resp_cli_handler(int argc, char *argv[])
{
    if (argc != 5) {
        printf("Usage: cmd <req_id> <user_role> <cmd> <data>\n");
        return -1;
    }
    char *req_id = argv[1];
    uint8_t user_role = atoi(argv[2]);
    uint16_t cmd = atoi(argv[3]);
    // esp_rmaker_cmd_resp_test_send(req_id, user_role, cmd, (void *)argv[4], strlen(argv[4]), esp_rmaker_test_cmd_resp, NULL);
    return 0;
}

static int crash_cli_handler(int argc, char *argv[])
{
    (void) argc;
    (void) argv;

    *(int *)0x00 = 0x10; // invoke crash by writing at illegal address

    return ESP_OK; // control never reaches here
}

static void register_insights_dbg_commands()
{
    const esp_console_cmd_t crash_cmd = {
        .command = "crash",
        .help = "Crash the application",
        .func = &crash_cli_handler,
    };
    ESP_LOGI(TAG, "Registering command: %s", crash_cmd.command);
    esp_console_cmd_register(&crash_cmd);
}

static void register_cmd_resp_command()
{
    const esp_console_cmd_t cmd_resp_cmd = {
        .command = "cmd",
        .help = "Send command to command-response module. Usage cmd <req_id> <cmd> <user_role> <data>",
        .func = &cmd_resp_cli_handler,
    };
    ESP_LOGI(TAG, "Registering command: %s", cmd_resp_cmd.command);
    esp_console_cmd_register(&cmd_resp_cmd);
}

void esp_insights_console_init()
{
    // initialize rmakre console and register general debug commands
    esp_rmaker_common_console_init();

    // register commands specific commands for `esp-insights`
    // register_cmd_resp_command();
    register_insights_dbg_commands();
}
