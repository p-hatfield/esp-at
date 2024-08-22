/*
 * SPDX-FileCopyrightText: 2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include "esp_at.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "soc/gpio_sig_map.h"

static uint8_t at_i2s_unrecognized_cmd(uint8_t *cmd_name);
static uint8_t at_i2s_unrecognized_para(uint8_t para_num);
static uint8_t at_exe_cmd_i2s_init(uint8_t para_num);
static esp_err_t at_i2s_init(i2s_role_t mode, int rate, int clock, int sync, int dataOut, int dataIn);

typedef enum
{
    AT_I2S_INIT,
}at_i2s_t;

static const char * const at_i2s_str[] = 
{
    "+I2SINIT",            //AT_I2S_INIT
};

static uint8_t at_i2s_unrecognized_cmd(uint8_t *cmd_name)
{
    return ESP_AT_RESULT_CODE_ERROR;
}

static uint8_t at_i2s_unrecognized_para(uint8_t para_num)
{
    return ESP_AT_RESULT_CODE_ERROR;
}

static uint8_t at_exe_cmd_i2s_init(uint8_t para_num)
{
    int32_t mode;
    int32_t rate;
    int32_t clock;
    int32_t sync;
    int32_t dataOut;
    int32_t dataIn;
    esp_at_para_parse_result_type parseResult = ESP_AT_PARA_PARSE_RESULT_OK;
    esp_err_t result = ESP_FAIL;

    parseResult |= esp_at_get_para_as_digit(0, &mode);
    parseResult |= esp_at_get_para_as_digit(1, &rate);
    parseResult |= esp_at_get_para_as_digit(2, &clock);
    parseResult |= esp_at_get_para_as_digit(3, &sync);
    parseResult |= esp_at_get_para_as_digit(4, &dataOut);
    parseResult |= esp_at_get_para_as_digit(5, &dataIn);

    if(parseResult == ESP_AT_PARA_PARSE_RESULT_OK)
    {
        result = at_i2s_init((i2s_role_t)mode, rate, clock, sync, dataOut, dataIn);  
    }
    return result == ESP_OK ? ESP_AT_RESULT_CODE_OK : ESP_AT_RESULT_CODE_ERROR;
}

static esp_err_t at_i2s_init(i2s_role_t mode, int rate, int clock, int sync, int dataOut, int dataIn)
{    
    i2s_chan_config_t chanConfig = { 
        .id = 0, 
        .role = mode, 
        .dma_desc_num = 6, 
        .dma_frame_num = 240, 
        .auto_clear = true, 
    };
    i2s_chan_handle_t rx;
    i2s_chan_handle_t tx;

    i2s_std_config_t stdConfig = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(rate),
        .slot_cfg = I2S_STD_PCM_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = clock,
            .ws = sync,
            .dout = dataOut,
            .din = dataIn,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    esp_err_t err = ESP_OK;
    err |= i2s_new_channel(&chanConfig, &tx, &rx);
    err |= i2s_channel_init_std_mode(tx, &stdConfig);
    err |= i2s_channel_init_std_mode(rx, &stdConfig);
    err |= i2s_channel_enable(tx);
    err |= i2s_channel_enable(rx);

    if(err == ESP_OK)
    {
        if(mode == I2S_ROLE_MASTER)
        {
            esp_rom_gpio_connect_out_signal(sync, PCMFSYNC_OUT_IDX, false, false);
            esp_rom_gpio_connect_out_signal(clock, PCMCLK_OUT_IDX, false, false);
        }
        else
        {
            esp_rom_gpio_connect_in_signal(sync, PCMFSYNC_IN_IDX, false);
            esp_rom_gpio_connect_in_signal(clock, PCMCLK_IN_IDX, false);
        }
        esp_rom_gpio_connect_out_signal(dataOut, PCMDOUT_IDX, false, false);
        esp_rom_gpio_connect_in_signal(dataIn, PCMDIN_IDX, false);
    }

    return err;
}

/*Ignoring warning to avoid changing API source*/
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
static const esp_at_cmd_struct at_i2s_cmd[] = {
    {"+I2SINIT", at_i2s_unrecognized_cmd, at_i2s_unrecognized_cmd, at_exe_cmd_i2s_init, at_i2s_unrecognized_cmd},
};
#pragma GCC diagnostic pop

bool esp_at_i2s_cmd_regist(void)
{
    return esp_at_custom_cmd_array_regist(at_i2s_cmd, sizeof(at_i2s_cmd) / sizeof(esp_at_cmd_struct));
}

ESP_AT_CMD_SET_INIT_FN(esp_at_i2s_cmd_regist, 1);
