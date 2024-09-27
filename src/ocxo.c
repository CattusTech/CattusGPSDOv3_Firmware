// Cattus GNSS Disciplined Oscillator
// Copyright (C) 2024 AlanCui4080

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.

// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
#include "hardware_error.h"
#include <FreeRTOS.h>
#include <task.h>

GPIO_InitTypeDef         gpio_ocxo_vcurr;
GPIO_InitTypeDef         gpio_ocxo_vtemp;
GPIO_InitTypeDef         gpio_ocxo_vtune;
RCC_PeriphCLKInitTypeDef ocxo_adc_clk;
ADC_HandleTypeDef        ocxo_adc_handle;
ADC_ChannelConfTypeDef   ocxo_adc_channel_vcurr;
ADC_ChannelConfTypeDef   ocxo_adc_channel_vtemp;
DAC_HandleTypeDef        ocxo_dac_handle;
DAC_ChannelConfTypeDef   ocxo_dac_channel_vtune;

int      ocxo_valid    = 0;
int      ocxo_overheat = 0;
uint16_t ocxo_vtune_bin;

static inline float caculate_adc_voltage(uint32_t data)
{
    return data * 3.0f / 0xfff;
}
void ocxo_init()
{
    gpio_ocxo_vcurr.Pin  = GPIO_PIN_6;
    gpio_ocxo_vcurr.Mode = GPIO_MODE_ANALOG;
    gpio_ocxo_vcurr.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio_ocxo_vcurr);

    gpio_ocxo_vtemp.Pin  = GPIO_PIN_7;
    gpio_ocxo_vtemp.Mode = GPIO_MODE_ANALOG;
    gpio_ocxo_vtemp.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio_ocxo_vtemp);

    ocxo_adc_clk.Adc12ClockSelection = RCC_PERIPHCLK_ADC12;
    ocxo_adc_clk.Adc12ClockSelection = RCC_ADC12CLKSOURCE_SYSCLK;

    HAL_StatusTypeDef result = HAL_RCCEx_PeriphCLKConfig(&ocxo_adc_clk);
    if (result != HAL_OK)
        hal_perror("ocxo", "HAL_RCCEx_PeriphCLKConfig", result);

    __HAL_RCC_ADC12_CLK_ENABLE();
    ocxo_adc_handle.Instance                   = ADC2;
    ocxo_adc_handle.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4;
    ocxo_adc_handle.Init.Resolution            = ADC_RESOLUTION_12B;
    ocxo_adc_handle.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    ocxo_adc_handle.Init.GainCompensation      = 0;
    ocxo_adc_handle.Init.ScanConvMode          = ADC_SCAN_DISABLE;
    ocxo_adc_handle.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
    ocxo_adc_handle.Init.LowPowerAutoWait      = DISABLE;
    ocxo_adc_handle.Init.ContinuousConvMode    = DISABLE;
    ocxo_adc_handle.Init.NbrOfConversion       = 2;
    ocxo_adc_handle.Init.DiscontinuousConvMode = DISABLE;
    ocxo_adc_handle.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    ocxo_adc_handle.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    ocxo_adc_handle.Init.DMAContinuousRequests = DISABLE;
    ocxo_adc_handle.Init.Overrun               = ADC_OVR_DATA_OVERWRITTEN;
    ocxo_adc_handle.Init.OversamplingMode      = DISABLE;

    result = HAL_ADC_Init(&ocxo_adc_handle);
    if (result != HAL_OK)
        hal_perror("ocxo", "HAL_ADC_Init", result);

    ocxo_adc_channel_vcurr.Channel      = ADC_CHANNEL_3;
    ocxo_adc_channel_vcurr.Rank         = ADC_REGULAR_RANK_1;
    ocxo_adc_channel_vcurr.SamplingTime = ADC_SAMPLETIME_640CYCLES_5;
    ocxo_adc_channel_vcurr.SingleDiff   = ADC_SINGLE_ENDED;
    ocxo_adc_channel_vcurr.OffsetNumber = ADC_OFFSET_NONE;
    ocxo_adc_channel_vcurr.Offset       = 0;

    result = HAL_ADC_ConfigChannel(&ocxo_adc_handle, &ocxo_adc_channel_vcurr);
    if (result != HAL_OK)
        hal_perror("ocxo", "HAL_ADC_ConfigChannel", result);

    ocxo_adc_channel_vtemp.Channel      = ADC_CHANNEL_4;
    ocxo_adc_channel_vtemp.Rank         = ADC_REGULAR_RANK_2;
    ocxo_adc_channel_vtemp.SamplingTime = ADC_SAMPLETIME_640CYCLES_5;
    ocxo_adc_channel_vtemp.SingleDiff   = ADC_SINGLE_ENDED;
    ocxo_adc_channel_vtemp.OffsetNumber = ADC_OFFSET_NONE;
    ocxo_adc_channel_vtemp.Offset       = 0;

    result = HAL_ADC_ConfigChannel(&ocxo_adc_handle, &ocxo_adc_channel_vtemp);
    if (result != HAL_OK)
        hal_perror("ocxo", "HAL_ADC_ConfigChannel", result);

    HAL_SYSCFG_DisableVREFBUF();
    HAL_SYSCFG_VREFBUF_HighImpedanceConfig(SYSCFG_VREFBUF_HIGH_IMPEDANCE_ENABLE);

    result = HAL_ADCEx_Calibration_Start(&ocxo_adc_handle, ADC_SINGLE_ENDED);
    if (result != HAL_OK)
        hal_perror("ocxo", "HAL_ADCEx_Calibration_Start", result);

    HAL_NVIC_SetPriority(ADC1_2_IRQn, 3, 0);
    HAL_NVIC_EnableIRQ(ADC1_2_IRQn);

    result = HAL_ADC_Start_IT(&ocxo_adc_handle);
    if (result != HAL_OK)
        hal_perror("ocxo", "HAL_ADC_Start_IT", result);

    printf("ocxo: adc running on vcurr and vtemp, 12bit resolution, 640.5 cycles\n");

    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    float vcurr = caculate_adc_voltage(HAL_ADC_GetValue(&ocxo_adc_handle));
    float vtemp = caculate_adc_voltage(HAL_ADC_GetValue(&ocxo_adc_handle));

    printf("ocxo: current: %fmA, temp: %f°C\n", vcurr * 2, vtemp / 10);

    gpio_ocxo_vtemp.Pin  = GPIO_PIN_4;
    gpio_ocxo_vtemp.Mode = GPIO_MODE_ANALOG;
    gpio_ocxo_vtemp.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio_ocxo_vtune);

    ocxo_dac_handle.Instance = DAC1;

    result = HAL_DAC_Init(&ocxo_dac_handle);
    if (result != HAL_OK)
        hal_perror("ocxo", "HAL_DAC_Init", result);

    ocxo_dac_channel_vtune.DAC_HighFrequency           = DAC_HIGH_FREQUENCY_INTERFACE_MODE_AUTOMATIC;
    ocxo_dac_channel_vtune.DAC_DMADoubleDataMode       = DISABLE;
    ocxo_dac_channel_vtune.DAC_SignedFormat            = DISABLE;
    ocxo_dac_channel_vtune.DAC_SampleAndHold           = DAC_SAMPLEANDHOLD_DISABLE;
    ocxo_dac_channel_vtune.DAC_Trigger                 = DAC_TRIGGER_SOFTWARE;
    ocxo_dac_channel_vtune.DAC_Trigger2                = DAC_TRIGGER_NONE;
    ocxo_dac_channel_vtune.DAC_OutputBuffer            = DAC_OUTPUTBUFFER_DISABLE;
    ocxo_dac_channel_vtune.DAC_ConnectOnChipPeripheral = DAC_CHIPCONNECT_EXTERNAL;
    ocxo_dac_channel_vtune.DAC_UserTrimming            = DAC_TRIMMING_FACTORY;

    result = HAL_DAC_ConfigChannel(&ocxo_dac_handle, &ocxo_dac_channel_vtune, DAC_CHANNEL_1);
    if (result != HAL_OK)
        hal_perror("ocxo", "HAL_DAC_ConfigChannel", result);

    result = HAL_DACEx_SelfCalibrate(&ocxo_dac_handle, &ocxo_dac_channel_vtune, DAC_CHANNEL_1);
    if (result != HAL_OK)
        hal_perror("ocxo", "HAL_DACEx_SelfCalibrate", result);

    result = HAL_DACEx_DualStart(&ocxo_dac_handle);
    if (result != HAL_OK)
        hal_perror("ocxo", "HAL_DACEx_DualStart", result);

    ocxo_vtune_bin = 0xfff / 2;
    result         = HAL_DAC_SetValue(&ocxo_dac_handle, DAC_CHANNEL_1, DAC_ALIGN_12B_R, ocxo_vtune_bin);
    if (result != HAL_OK)
        hal_perror("ocxo", "HAL_DAC_SetValue", result);

    printf("ocxo: dac running on vtune, 12bit resolution, no buffer\n");
}

void ocxo_update()
{
    HAL_StatusTypeDef result = HAL_ADC_Start_IT(&ocxo_adc_handle);
    if (result != HAL_OK)
        hal_perror("ocxo", "HAL_ADC_Start_IT", result);
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    float vcurr = caculate_adc_voltage(HAL_ADC_GetValue(&ocxo_adc_handle));
    float vtemp = caculate_adc_voltage(HAL_ADC_GetValue(&ocxo_adc_handle));

    const float vcurr_threshold = 250.0f;
    const float vtemp_threshold = 85.0f;

    if (vcurr * 2 <= vcurr_threshold)
    {
        ocxo_valid = 1;
    }
    else
    {
        ocxo_valid = 0;
    }

    if (vtemp / 10 >= vtemp_threshold)
    {
        ocxo_overheat = 1;
    }
    else
    {
        ocxo_overheat = 0;
    }
}
