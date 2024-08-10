/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "esp_mac.h"
#include "esp_at.h"
#include "esp_hf_ag_api.h"

static esp_event_loop_handle_t loop_handle = NULL;

static void at_hfp_ag_callback(esp_hf_cb_event_t event, esp_hf_cb_param_t *param)
{
    uint8_t buffer[64] = {0};
    snprintf((char *)buffer, 64, "Event Hit: %i\r\n", event);
    esp_at_port_write_data(buffer, strlen((char *)buffer));
    switch(event)
    {
        case ESP_HF_CONNECTION_STATE_EVT:              
            snprintf((char *)buffer, 64, "Connection Status Changed: %i\r\n", param->conn_stat.state);
            esp_at_port_write_data(buffer, strlen((char *)buffer));
            break;/*!< Connection state changed event */
        case ESP_HF_AUDIO_STATE_EVT:                    break;/*!< Audio connection state change event */
        case ESP_HF_BVRA_RESPONSE_EVT:                  break;/*!< Voice recognition state change event */
        case ESP_HF_VOLUME_CONTROL_EVT:                 break;/*!< Audio volume control command from HF Client: provided by +VGM or +VGS message */
        case ESP_HF_UNAT_RESPONSE_EVT:                  break;/*!< Unknown AT cmd Response*/
        case ESP_HF_IND_UPDATE_EVT:                     break;/*!< Indicator Update Event*/
        case ESP_HF_CIND_RESPONSE_EVT:   
            snprintf((char *)buffer, 64, "CIND message received\r\n");
            esp_at_port_write_data(buffer, strlen((char *)buffer));

            esp_bd_addr_t remote_addr;
            esp_read_mac(&remote_addr[0], ESP_MAC_BT);
            memcpy(remote_addr, param->cind_rep.remote_addr, sizeof(remote_addr));
            esp_hf_call_status_t call_state = ESP_HF_CALL_STATUS_NO_CALLS;
            esp_hf_call_setup_status_t call_setup_state = ESP_HF_CALL_SETUP_STATUS_IDLE;
            esp_hf_network_state_t ntk_state = ESP_HF_NETWORK_STATE_AVAILABLE;
            int signal = 5;
            esp_hf_roaming_status_t roam = ESP_HF_ROAMING_STATUS_INACTIVE;
            int batt_lev = 5;
            esp_hf_call_held_status_t call_held_status = ESP_HF_CALL_HELD_STATUS_NONE;

            esp_err_t result = esp_hf_ag_cind_response(remote_addr, call_state, call_setup_state, ntk_state, signal, roam, batt_lev, call_held_status);
            
            snprintf((char *)buffer, 64, "CIND response result: %i\r\n", result);
            esp_at_port_write_data(buffer, strlen((char *)buffer));
            break;/*!< Call And Device Indicator Response*/
        case ESP_HF_COPS_RESPONSE_EVT:                  break;/*!< Current operator information */
        case ESP_HF_CLCC_RESPONSE_EVT:                  break;/*!< List of current calls notification */
        case ESP_HF_CNUM_RESPONSE_EVT:                  break;/*!< Subscriber information response from HF Client */
        case ESP_HF_VTS_RESPONSE_EVT:                   break;/*!< Enable or not DTMF */
        case ESP_HF_NREC_RESPONSE_EVT:                  break;/*!< Enable or not NREC */
        case ESP_HF_ATA_RESPONSE_EVT:                   break;/*!< Answer an Incoming Call */
        case ESP_HF_CHUP_RESPONSE_EVT:                  break;/*!< Reject an Incoming Call */
        case ESP_HF_DIAL_EVT:                           break;/*!< Origin an outgoing call with specific number or the dial the last number */
        case ESP_HF_WBS_RESPONSE_EVT:                   break;/*!< Codec Status */
        case ESP_HF_BCS_RESPONSE_EVT:                   break;/*!< Final Codec Choice */
        case ESP_HF_PKT_STAT_NUMS_GET_EVT:              break;/*!< Request number of packet different status */
    }
}

static uint8_t at_test_cmd_hfp_ag(uint8_t *cmd_name)
{
    uint8_t buffer[64] = {0};
    snprintf((char *)buffer, 64, "hfp_ag command: <AT%s=?> is executed\r\n", cmd_name);
    esp_at_port_write_data(buffer, strlen((char *)buffer));

    return ESP_AT_RESULT_CODE_OK;
}

static uint8_t at_query_cmd_hfp_ag(uint8_t *cmd_name)
{
    uint8_t buffer[64] = {0};
    snprintf((char *)buffer, 64, "query command: <AT%s?> is executed\r\n", cmd_name);
    esp_at_port_write_data(buffer, strlen((char *)buffer));

    return ESP_AT_RESULT_CODE_OK;
}

static uint8_t at_setup_cmd_hfp_ag(uint8_t para_num)
{
    uint8_t index = 0;

    // get first parameter, and parse it into a digit
    int32_t digit = 0;
    if (esp_at_get_para_as_digit(index++, &digit) != ESP_AT_PARA_PARSE_RESULT_OK) {
        return ESP_AT_RESULT_CODE_ERROR;
    }

    // get second parameter, and parse it into a string
    uint8_t *str = NULL;
    if (esp_at_get_para_as_str(index++, &str) != ESP_AT_PARA_PARSE_RESULT_OK) {
        return ESP_AT_RESULT_CODE_ERROR;
    }

    // allocate a buffer and construct the data, then send the data to mcu via interface (uart/spi/sdio/socket)
    uint8_t *buffer = (uint8_t *)malloc(512);
    if (!buffer) {
        return ESP_AT_RESULT_CODE_ERROR;
    }
    int len = snprintf((char *)buffer, 512, "setup command: <AT%s=%d,\"%s\"> is executed\r\n",
                       esp_at_get_current_cmd_name(), digit, str);
    esp_at_port_write_data(buffer, len);

    // remember to free the buffer
    free(buffer);

    return ESP_AT_RESULT_CODE_OK;
}

static uint8_t at_exe_cmd_hfp_ag(uint8_t *cmd_name)
{
    uint8_t buffer[64] = {0};
    snprintf((char *)buffer, 64, "execute command: <AT%s> is executed\r\n", cmd_name);
    esp_at_port_write_data(buffer, strlen((char *)buffer));

    return ESP_AT_RESULT_CODE_OK;
}

static uint8_t at_hfp_ag_unrecognized_cmd(uint8_t *cmd_name)
{
    return ESP_AT_RESULT_CODE_ERROR;
}

static uint8_t at_hfp_ag_unrecognized_para(uint8_t para_num)
{
    return ESP_AT_RESULT_CODE_ERROR;
}

static uint8_t at_exe_cmd_hfp_ag_init(uint8_t para_num)
{
    int32_t digit = 0;
    if (esp_at_get_para_as_digit(0, &digit) != ESP_AT_PARA_PARSE_RESULT_OK) {
        return ESP_AT_RESULT_CODE_ERROR;
    }
    if(digit > 1)
    {
        return ESP_AT_RESULT_CODE_ERROR;
    }

    esp_hf_ag_register_callback(at_hfp_ag_callback);

    esp_err_t result = digit == 1 ? esp_hf_ag_init() : esp_hf_ag_deinit();
    return result == ESP_OK ? ESP_AT_RESULT_CODE_OK : ESP_AT_RESULT_CODE_ERROR;
}

static uint8_t at_exe_cmd_hfp_ag_connect_service(uint8_t para_num)
{
    esp_bd_addr_t bytes;
    int values[6];
    char* mac;
    int i;

    if(esp_at_get_para_as_str(0, (unsigned char **)&mac) != ESP_AT_PARA_PARSE_RESULT_OK)
    {
        uint8_t buffer[64] = {0};
        snprintf((char *)buffer, 64, "Could not recognize String\r\n");
        esp_at_port_write_data(buffer, strlen((char *)buffer));
        return ESP_AT_RESULT_CODE_ERROR;
    }

    if (6 == sscanf(mac, "%x:%x:%x:%x:%x:%x%*c",
                    &values[0], &values[1], &values[2],
                    &values[3], &values[4], &values[5]))
    {
      /* convert to uint8_t */
      for (i = 0; i < 6; ++i)
        bytes[i] = (uint8_t)values[i];
    }
    else
    {
        uint8_t buffer[64] = {0};
        snprintf((char *)buffer, 64, "Could not recognize Parse %s\r\n", mac);
        esp_at_port_write_data(buffer, strlen((char *)buffer));
        return ESP_AT_RESULT_CODE_ERROR;
    }

    esp_err_t result = esp_hf_ag_slc_connect(bytes);
    return result == ESP_OK ? ESP_AT_RESULT_CODE_OK : ESP_AT_RESULT_CODE_ERROR;
}

static uint8_t at_exe_cmd_hfp_ag_connect_Audio(uint8_t para_num)
{
    esp_bd_addr_t bytes;
    int values[6];
    char* mac;
    int i;

    if(esp_at_get_para_as_str(0, (unsigned char **)&mac) != ESP_AT_PARA_PARSE_RESULT_OK)
    {
        uint8_t buffer[64] = {0};
        snprintf((char *)buffer, 64, "Could not recognize String\r\n");
        esp_at_port_write_data(buffer, strlen((char *)buffer));
        return ESP_AT_RESULT_CODE_ERROR;
    }

    if (6 == sscanf(mac, "%x:%x:%x:%x:%x:%x%*c",
                    &values[0], &values[1], &values[2],
                    &values[3], &values[4], &values[5]))
    {
      /* convert to uint8_t */
      for (i = 0; i < 6; ++i)
        bytes[i] = (uint8_t)values[i];
    }
    else
    {
        uint8_t buffer[64] = {0};
        snprintf((char *)buffer, 64, "Could not recognize Parse %s\r\n", mac);
        esp_at_port_write_data(buffer, strlen((char *)buffer));
        return ESP_AT_RESULT_CODE_ERROR;
    }

    esp_err_t result = esp_hf_ag_audio_connect(bytes);
    return result == ESP_OK ? ESP_AT_RESULT_CODE_OK : ESP_AT_RESULT_CODE_ERROR;
}

static const esp_at_cmd_struct at_hfp_ag_cmd[] = {
    {"+HFPAG", at_test_cmd_hfp_ag, at_query_cmd_hfp_ag, at_setup_cmd_hfp_ag, at_exe_cmd_hfp_ag},
    {"+HFPAGINIT", at_hfp_ag_unrecognized_cmd, at_hfp_ag_unrecognized_cmd, at_exe_cmd_hfp_ag_init, at_hfp_ag_unrecognized_cmd},
    {"+HFPAGCONNECTSVC", at_hfp_ag_unrecognized_cmd, at_hfp_ag_unrecognized_cmd, at_exe_cmd_hfp_ag_connect_service, at_hfp_ag_unrecognized_cmd},
    {"+HFPAGCONNECTAUD", at_hfp_ag_unrecognized_cmd, at_hfp_ag_unrecognized_cmd, at_exe_cmd_hfp_ag_connect_service, at_hfp_ag_unrecognized_cmd},
};

bool esp_at_hfp_ag_cmd_regist(void)
{
    return esp_at_custom_cmd_array_regist(at_hfp_ag_cmd, sizeof(at_hfp_ag_cmd) / sizeof(esp_at_cmd_struct));
}

ESP_AT_CMD_SET_INIT_FN(esp_at_hfp_ag_cmd_regist, 1);
