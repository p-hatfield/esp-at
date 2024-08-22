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
#include "esp_gap_bt_api.h"

static char* HFP_AG_VERSION = "v0.0.1\r\n";

typedef enum
{
    AT_HFP_AG_INIT,
    AT_HFP_AG_CONNECT_SVC,
    AT_HFP_AG_DISCONNECT_SVC,
    AT_HFP_AG_CONNECT_AUD,
    AT_HFP_AG_DISCONNECT_AUD,
    AT_HFP_AG_SET_MIC_VOL,
    AT_HFP_AG_SET_SPK_VOL,

    AT_HFP_AG_CONNECT_STATUS,
    AT_HFP_AG_AUDIO_STATUS,
    AT_HFP_AG_MIC_VOL_STATUS,
    AT_HFP_AG_SPK_VOL_STATUS,
}at_hfp_ag_t;

static const char * const at_hfp_ag_str[] = 
{
    /*Input*/
    "+HFPAGINIT",            //AT_HFP_AG_INIT
    "+HFPAGCONNECTSVC",      //AT_HFP_AG_CONNECT_SVC
    "+HFPAGDISCONNECTSVC",   //AT_HFP_AG_DISCONNECT_SVC
    "+HFPAGCONNECTAUD",      //AT_HFP_AG_CONNECT_AUD
    "+HFPAGDISCONNECTAUD",   //AT_HFP_AG_DISCONNECT_AUD
    "+HFPAGSETMICVOL",       //AT_HFP_AG_SET_MIC_VOL
    "+HFPAGSETSPKVOL",       //AT_HFP_AG_SET_SPK_VOL
    /*Output*/
    "+HFPAGCONNECTSTS",      //AT_HFP_AG_CONNECT_STATUS
    "+HFPAGAUDIOSTS",        //AT_HFP_AG_AUDIO_STATUS
    "+HFPAGMICVOLSTS",       //AT_HFP_AG_MIC_VOL_STATUS
    "+HFPAGSPKVOLSTS"        //AT_HFP_AG_SPK_VOL_STATUS
};

static void at_hfp_ag_get_address(esp_bd_addr_t *bytes);

static void at_hfp_ag_handle_connection_state(esp_hf_cb_param_t *param);
static void at_hfp_ag_handle_audio_state(esp_hf_cb_param_t *param);
static void at_hfp_ag_handle_bvra_resp(esp_hf_cb_param_t *param);
static void at_hfp_ag_handle_volume_control(esp_hf_cb_param_t *param);
static void at_hfp_ag_handle_unat_resp(esp_hf_cb_param_t *param);
static void at_hfp_ag_handle_ind_update(esp_hf_cb_param_t *param);
static void at_hfp_ag_handle_cind_resp(esp_hf_cb_param_t *param);
static void at_hfp_ag_handle_cops_resp(esp_hf_cb_param_t *param);
static void at_hfp_ag_handle_clcc_resp(esp_hf_cb_param_t *param);
static void at_hfp_ag_handle_cnum_resp(esp_hf_cb_param_t *param);
static void at_hfp_ag_handle_vts_resp(esp_hf_cb_param_t *param);
static void at_hfp_ag_handle_nrec_resp(esp_hf_cb_param_t *param);
static void at_hfp_ag_handle_ata_resp(esp_hf_cb_param_t *param);
static void at_hfp_ag_handle_chup_resp(esp_hf_cb_param_t *param);
static void at_hfp_ag_handle_dial(esp_hf_cb_param_t *param);
static void at_hfp_ag_handle_wbs_resp(esp_hf_cb_param_t *param);
static void at_hfp_ag_handle_bsc_resp(esp_hf_cb_param_t *param);
static void at_hfp_ag_handle_packet_status(esp_hf_cb_param_t *param);

static void at_hfp_ag_callback(esp_hf_cb_event_t event, esp_hf_cb_param_t *param)
{
    switch(event)
    {
        case ESP_HF_CONNECTION_STATE_EVT:   at_hfp_ag_handle_connection_state(param);   break;/*!< Connection state changed event */
        case ESP_HF_AUDIO_STATE_EVT:        at_hfp_ag_handle_audio_state(param);        break;/*!< Audio connection state change event */
        case ESP_HF_BVRA_RESPONSE_EVT:      at_hfp_ag_handle_bvra_resp(param);          break;/*!< Voice recognition state change event */
        case ESP_HF_VOLUME_CONTROL_EVT:     at_hfp_ag_handle_volume_control(param);     break;/*!< Audio volume control command from HF Client: provided by +VGM or +VGS message */
        case ESP_HF_UNAT_RESPONSE_EVT:      at_hfp_ag_handle_unat_resp(param);          break;/*!< Unknown AT cmd Response*/
        case ESP_HF_IND_UPDATE_EVT:         at_hfp_ag_handle_ind_update(param);         break;/*!< Indicator Update Event*/
        case ESP_HF_CIND_RESPONSE_EVT:      at_hfp_ag_handle_cind_resp(param);          break;/*!< Call And Device Indicator Response*/
        case ESP_HF_COPS_RESPONSE_EVT:      at_hfp_ag_handle_cops_resp(param);          break;/*!< Current operator information */
        case ESP_HF_CLCC_RESPONSE_EVT:      at_hfp_ag_handle_clcc_resp(param);          break;/*!< List of current calls notification */
        case ESP_HF_CNUM_RESPONSE_EVT:      at_hfp_ag_handle_cnum_resp(param);          break;/*!< Subscriber information response from HF Client */
        case ESP_HF_VTS_RESPONSE_EVT:       at_hfp_ag_handle_vts_resp(param);           break;/*!< Enable or not DTMF */
        case ESP_HF_NREC_RESPONSE_EVT:      at_hfp_ag_handle_nrec_resp(param);          break;/*!< Enable or not NREC */
        case ESP_HF_ATA_RESPONSE_EVT:       at_hfp_ag_handle_ata_resp(param);           break;/*!< Answer an Incoming Call */
        case ESP_HF_CHUP_RESPONSE_EVT:      at_hfp_ag_handle_chup_resp(param);          break;/*!< Reject an Incoming Call */
        case ESP_HF_DIAL_EVT:               at_hfp_ag_handle_dial(param);               break;/*!< Origin an outgoing call with specific number or the dial the last number */
        case ESP_HF_WBS_RESPONSE_EVT:       at_hfp_ag_handle_wbs_resp(param);           break;/*!< Codec Status */
        case ESP_HF_BCS_RESPONSE_EVT:       at_hfp_ag_handle_bsc_resp(param);           break;/*!< Final Codec Choice */
        case ESP_HF_PKT_STAT_NUMS_GET_EVT:  at_hfp_ag_handle_packet_status(param);      break;/*!< Request number of packet different status */
    }
}

static void at_hfp_ag_handle_connection_state(esp_hf_cb_param_t *param)
{
    uint8_t buffer[64] = {0};
    snprintf((char *)buffer, 64, "AT%s=%i\r\n", at_hfp_ag_str[AT_HFP_AG_CONNECT_STATUS], param->conn_stat.state);
    esp_at_port_write_data(buffer, strlen((char *)buffer));
}
static void at_hfp_ag_handle_audio_state(esp_hf_cb_param_t *param)
{
    uint8_t buffer[64] = {0};
    snprintf((char *)buffer, 64, "AT%s=%i\r\n", at_hfp_ag_str[AT_HFP_AG_AUDIO_STATUS], param->conn_stat.state);
    esp_at_port_write_data(buffer, strlen((char *)buffer));
}
static void at_hfp_ag_handle_bvra_resp(esp_hf_cb_param_t *param)
{
}
static void at_hfp_ag_handle_volume_control(esp_hf_cb_param_t *param)
{
    const char * atCmd = param->volume_control.type == ESP_HF_VOLUME_TYPE_SPK ? at_hfp_ag_str[AT_HFP_AG_SPK_VOL_STATUS] : at_hfp_ag_str[AT_HFP_AG_MIC_VOL_STATUS];
    uint8_t buffer[64] = {0};
    snprintf((char *)buffer, 64, "AT%s=%i\r\n", atCmd, param->volume_control.volume);
    esp_at_port_write_data(buffer, strlen((char *)buffer));
}
static void at_hfp_ag_handle_unat_resp(esp_hf_cb_param_t *param)
{
    uint8_t buffer[64] = {0};
    snprintf((char *)buffer, 64, "AT%s\r\n", param->unat_rep.unat);
    esp_at_port_write_data(buffer, strlen((char *)buffer));
    
    esp_hf_ag_unknown_at_send(param->unat_rep.remote_addr, NULL);
}
static void at_hfp_ag_handle_ind_update(esp_hf_cb_param_t *param)
{
}
static void at_hfp_ag_handle_cind_resp(esp_hf_cb_param_t *param)
{
    uint8_t buffer[64] = {0};
    esp_bd_addr_t remote_addr;
    memcpy(remote_addr, param->cind_rep.remote_addr, sizeof(remote_addr));
    esp_hf_call_status_t call_state = ESP_HF_CALL_STATUS_NO_CALLS;
    esp_hf_call_setup_status_t call_setup_state = ESP_HF_CALL_SETUP_STATUS_IDLE;
    esp_hf_network_state_t ntk_state = ESP_HF_NETWORK_STATE_AVAILABLE;
    int signal = 5;
    esp_hf_roaming_status_t roam = ESP_HF_ROAMING_STATUS_INACTIVE;
    int batt_lev = 5;
    esp_hf_call_held_status_t call_held_status = ESP_HF_CALL_HELD_STATUS_NONE;

    esp_err_t result = esp_hf_ag_cind_response(remote_addr, call_state, call_setup_state, ntk_state, signal, roam, batt_lev, call_held_status);
    esp_at_port_write_data(buffer, strlen((char *)buffer));
}
static void at_hfp_ag_handle_cops_resp(esp_hf_cb_param_t *param)
{
}
static void at_hfp_ag_handle_clcc_resp(esp_hf_cb_param_t *param)
{
}
static void at_hfp_ag_handle_cnum_resp(esp_hf_cb_param_t *param)
{
}
static void at_hfp_ag_handle_vts_resp(esp_hf_cb_param_t *param)
{
}
static void at_hfp_ag_handle_nrec_resp(esp_hf_cb_param_t *param)
{
}
static void at_hfp_ag_handle_ata_resp(esp_hf_cb_param_t *param)
{
}
static void at_hfp_ag_handle_chup_resp(esp_hf_cb_param_t *param)
{
}
static void at_hfp_ag_handle_dial(esp_hf_cb_param_t *param)
{
}
static void at_hfp_ag_handle_wbs_resp(esp_hf_cb_param_t *param)
{
}
static void at_hfp_ag_handle_bsc_resp(esp_hf_cb_param_t *param)
{
}
static void at_hfp_ag_handle_packet_status(esp_hf_cb_param_t *param)
{
}

static uint8_t at_exe_cmd_hfp_ag(uint8_t *cmd_name)
{
    esp_at_port_write_data((uint8_t *)HFP_AG_VERSION, strlen(HFP_AG_VERSION));
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
    esp_bd_addr_t addr;
    at_hfp_ag_get_address(&addr);

    esp_err_t result = esp_hf_ag_slc_connect(addr);
    return result == ESP_OK ? ESP_AT_RESULT_CODE_OK : ESP_AT_RESULT_CODE_ERROR;
}

static uint8_t at_exe_cmd_hfp_ag_connect_audio(uint8_t para_num)
{
    esp_bd_addr_t addr;
    at_hfp_ag_get_address(&addr);

    esp_err_t result = esp_hf_ag_audio_connect(addr);
    return result == ESP_OK ? ESP_AT_RESULT_CODE_OK : ESP_AT_RESULT_CODE_ERROR;
}

static uint8_t at_exe_cmd_hfp_ag_disconnect_service(uint8_t para_num)
{
    esp_bd_addr_t addr;
    at_hfp_ag_get_address(&addr);

    esp_err_t result = esp_hf_ag_slc_disconnect(addr);
    return result == ESP_OK ? ESP_AT_RESULT_CODE_OK : ESP_AT_RESULT_CODE_ERROR;
}

static uint8_t at_exe_cmd_hfp_ag_disconnect_audio(uint8_t para_num)
{
    esp_bd_addr_t addr;
    at_hfp_ag_get_address(&addr);

    esp_err_t result = esp_hf_ag_audio_disconnect(addr);
    return result == ESP_OK ? ESP_AT_RESULT_CODE_OK : ESP_AT_RESULT_CODE_ERROR;
}

static uint8_t at_exe_cmd_hfp_ag_set_mic_vol(uint8_t para_num)
{
    esp_bd_addr_t addr;
    int32_t value;
    esp_err_t result = ESP_FAIL;
    at_hfp_ag_get_address(&addr);

    if(esp_at_get_para_as_digit(1, &value) == ESP_AT_PARA_PARSE_RESULT_OK)
    {
        result = esp_hf_ag_volume_control(addr, ESP_HF_VOLUME_CONTROL_TARGET_MIC, value);        
    }
    return result == ESP_OK ? ESP_AT_RESULT_CODE_OK : ESP_AT_RESULT_CODE_ERROR;
}

static uint8_t at_exe_cmd_hfp_ag_set_spk_vol(uint8_t para_num)
{
    esp_bd_addr_t addr;
    int32_t value;
    esp_err_t result = ESP_FAIL;
    at_hfp_ag_get_address(&addr);

    if(esp_at_get_para_as_digit(1, &value) == ESP_AT_PARA_PARSE_RESULT_OK)
    {
        result = esp_hf_ag_volume_control(addr, ESP_HF_VOLUME_CONTROL_TARGET_SPK, value);        
    }
    return result == ESP_OK ? ESP_AT_RESULT_CODE_OK : ESP_AT_RESULT_CODE_ERROR;
}

/*Ignoring warning to avoid changing API source*/
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
static const esp_at_cmd_struct at_hfp_ag_cmd[] = {
    {"+HFPAG", at_hfp_ag_unrecognized_cmd, at_hfp_ag_unrecognized_cmd, at_hfp_ag_unrecognized_para, at_exe_cmd_hfp_ag},
    {at_hfp_ag_str[AT_HFP_AG_INIT], at_hfp_ag_unrecognized_cmd, at_hfp_ag_unrecognized_cmd, at_exe_cmd_hfp_ag_init, at_hfp_ag_unrecognized_cmd},
    {at_hfp_ag_str[AT_HFP_AG_CONNECT_SVC], at_hfp_ag_unrecognized_cmd, at_hfp_ag_unrecognized_cmd, at_exe_cmd_hfp_ag_connect_service, at_hfp_ag_unrecognized_cmd},
    {at_hfp_ag_str[AT_HFP_AG_DISCONNECT_SVC], at_hfp_ag_unrecognized_cmd, at_hfp_ag_unrecognized_cmd, at_exe_cmd_hfp_ag_disconnect_service, at_hfp_ag_unrecognized_cmd},
    {at_hfp_ag_str[AT_HFP_AG_CONNECT_AUD], at_hfp_ag_unrecognized_cmd, at_hfp_ag_unrecognized_cmd, at_exe_cmd_hfp_ag_connect_audio, at_hfp_ag_unrecognized_cmd},
    {at_hfp_ag_str[AT_HFP_AG_DISCONNECT_AUD], at_hfp_ag_unrecognized_cmd, at_hfp_ag_unrecognized_cmd, at_exe_cmd_hfp_ag_disconnect_audio, at_hfp_ag_unrecognized_cmd},
    {at_hfp_ag_str[AT_HFP_AG_SET_MIC_VOL], at_hfp_ag_unrecognized_cmd, at_hfp_ag_unrecognized_cmd, at_exe_cmd_hfp_ag_set_mic_vol, at_hfp_ag_unrecognized_cmd},
    {at_hfp_ag_str[AT_HFP_AG_SET_SPK_VOL], at_hfp_ag_unrecognized_cmd, at_hfp_ag_unrecognized_cmd, at_exe_cmd_hfp_ag_set_spk_vol, at_hfp_ag_unrecognized_cmd},
};
#pragma GCC diagnostic pop

bool esp_at_hfp_ag_cmd_regist(void)
{
    return esp_at_custom_cmd_array_regist(at_hfp_ag_cmd, sizeof(at_hfp_ag_cmd) / sizeof(esp_at_cmd_struct));
}

static void at_hfp_ag_get_address(esp_bd_addr_t *bytes)
{
    int values[6];
    char* mac;
    int i;

    if(esp_at_get_para_as_str(0, (unsigned char **)&mac) != ESP_AT_PARA_PARSE_RESULT_OK)
    {
        uint8_t buffer[64] = {0};
        snprintf((char *)buffer, 64, "Could not recognize String\r\n");
        esp_at_port_write_data(buffer, strlen((char *)buffer));
    }

    if (6 == sscanf(mac, "%x:%x:%x:%x:%x:%x%*c",
                    &values[0], &values[1], &values[2],
                    &values[3], &values[4], &values[5]))
    {
        for (i = 0; i < 6; ++i)
        {
            (*bytes)[i] = (uint8_t)values[i];
        }
    }
    else
    {
        uint8_t buffer[64] = {0};
        snprintf((char *)buffer, 64, "Could not recognize Parse %s\r\n", mac);
        esp_at_port_write_data(buffer, strlen((char *)buffer));
    }
}

ESP_AT_CMD_SET_INIT_FN(esp_at_hfp_ag_cmd_regist, 1);
