/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_at_core.h"
#include "esp_at.h"
#include "esp_at_interface.h"

static const char *TAG = "at-cmd-register";

#ifdef CONFIG_AT_BASE_COMMAND_SUPPORT
ESP_AT_CMD_SET_FIRST_INIT_FN(esp_at_base_cmd_regist, 1);
#endif

#ifdef CONFIG_AT_WIFI_COMMAND_SUPPORT
ESP_AT_CMD_SET_FIRST_INIT_FN(esp_at_wifi_cmd_regist, 2);
#endif

#ifdef CONFIG_AT_SMARTCONFIG_COMMAND_SUPPORT
ESP_AT_CMD_SET_FIRST_INIT_FN(esp_at_smartconfig_cmd_regist, 3);
#endif

#ifdef CONFIG_AT_WPS_COMMAND_SUPPORT
ESP_AT_CMD_SET_FIRST_INIT_FN(esp_at_wps_cmd_regist, 4);
#endif

#ifdef CONFIG_AT_EAP_COMMAND_SUPPORT
ESP_AT_CMD_SET_FIRST_INIT_FN(esp_at_eap_cmd_regist, 5);
#endif

#ifdef CONFIG_AT_MDNS_COMMAND_SUPPORT
ESP_AT_CMD_SET_FIRST_INIT_FN(esp_at_mdns_cmd_regist, 6);
#endif

#ifdef CONFIG_AT_NET_COMMAND_SUPPORT
ESP_AT_CMD_SET_FIRST_INIT_FN(esp_at_net_cmd_regist, 7);
#endif

#ifdef CONFIG_AT_PING_COMMAND_SUPPORT
ESP_AT_CMD_SET_FIRST_INIT_FN(esp_at_ping_cmd_regist, 8);
#endif

#ifdef CONFIG_AT_MQTT_COMMAND_SUPPORT
ESP_AT_CMD_SET_FIRST_INIT_FN(esp_at_mqtt_cmd_regist, 9);
#endif

#ifdef CONFIG_AT_HTTP_COMMAND_SUPPORT
ESP_AT_CMD_SET_FIRST_INIT_FN(esp_at_http_cmd_regist, 10);
#endif

#ifdef CONFIG_AT_WS_COMMAND_SUPPORT
ESP_AT_CMD_SET_FIRST_INIT_FN(esp_at_ws_cmd_regist, 11);
#endif

#ifdef CONFIG_AT_BLE_COMMAND_SUPPORT
ESP_AT_CMD_SET_FIRST_INIT_FN(esp_at_ble_cmd_regist, 12);
#endif

#ifdef CONFIG_AT_BLE_HID_COMMAND_SUPPORT
ESP_AT_CMD_SET_FIRST_INIT_FN(esp_at_ble_hid_cmd_regist, 13);
#endif

#ifdef CONFIG_AT_BLUFI_COMMAND_SUPPORT
ESP_AT_CMD_SET_FIRST_INIT_FN(esp_at_blufi_cmd_regist, 14);
#endif

#ifdef CONFIG_AT_BT_COMMAND_SUPPORT
ESP_AT_CMD_SET_FIRST_INIT_FN(esp_at_bt_cmd_regist, 15);
#endif

#ifdef CONFIG_AT_BT_SPP_COMMAND_SUPPORT
ESP_AT_CMD_SET_FIRST_INIT_FN(esp_at_bt_spp_cmd_regist, 16);
#endif

#ifdef CONFIG_AT_BT_A2DP_COMMAND_SUPPORT
ESP_AT_CMD_SET_FIRST_INIT_FN(esp_at_bt_a2dp_cmd_regist, 17);
#endif

#ifdef CONFIG_AT_FS_COMMAND_SUPPORT
ESP_AT_CMD_SET_FIRST_INIT_FN(esp_at_fs_cmd_regist, 18);
#endif

#ifdef CONFIG_AT_DRIVER_COMMAND_SUPPORT
ESP_AT_CMD_SET_FIRST_INIT_FN(esp_at_driver_cmd_regist, 19);
#endif

#ifdef CONFIG_AT_ETHERNET_SUPPORT
ESP_AT_CMD_SET_FIRST_INIT_FN(esp_at_eth_cmd_regist, 20);
#endif

ESP_AT_CMD_SET_FIRST_INIT_FN(esp_at_hfp_ag_cmd_regist, 21);
ESP_AT_CMD_SET_FIRST_INIT_FN(esp_at_i2s_cmd_regist, 22);

void esp_at_cmd_set_register(void)
{
    at_cmd_set_register_t *p;

    // register the at command set which initialized by ESP_AT_CMD_SET_FIRST_INIT_FN
    extern at_cmd_set_register_t _at_cmd_set_first_init_fn_array_start;
    extern at_cmd_set_register_t _at_cmd_set_first_init_fn_array_end;
    for (p = &_at_cmd_set_first_init_fn_array_start; p < &_at_cmd_set_first_init_fn_array_end; ++p) {
        bool ret = (*(p->fn))();
        if (!ret) {
            ESP_LOGE(TAG, "%s failed", p->name);
        } else {
            ESP_LOGD(TAG, "%s success", p->name);
        }
    }

    // register the at command set which initialized by ESP_AT_CMD_SET_INIT_FN
    extern at_cmd_set_register_t _at_cmd_set_init_fn_array_start;
    extern at_cmd_set_register_t _at_cmd_set_init_fn_array_end;
    for (p = &_at_cmd_set_init_fn_array_start; p < &_at_cmd_set_init_fn_array_end; ++p) {
        bool ret = (*(p->fn))();
        if (!ret) {
            ESP_LOGE(TAG, "%s failed", p->name);
        } else {
            ESP_LOGD(TAG, "%s success", p->name);
        }
    }

    // register the at command set which initialized by ESP_AT_CMD_SET_LAST_INIT_FN
    extern at_cmd_set_register_t _at_cmd_set_last_init_fn_array_start;
    extern at_cmd_set_register_t _at_cmd_set_last_init_fn_array_end;
    for (p = &_at_cmd_set_last_init_fn_array_start; p < &_at_cmd_set_last_init_fn_array_end; ++p) {
        bool ret = (*(p->fn))();
        if (!ret) {
            ESP_LOGE(TAG, "%s failed", p->name);
        } else {
            ESP_LOGD(TAG, "%s success", p->name);
        }
    }
}
