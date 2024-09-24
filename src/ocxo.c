#include "hardware_error.h"
#include <stm32g4xx_hal.h>

GPIO_InitTypeDef         gpio_ocxo_vcurr;
GPIO_InitTypeDef         gpio_ocxo_vtemp;
RCC_PeriphCLKInitTypeDef ocxo_adc_clk;
ADC_HandleTypeDef        ocxo_adc_handle;
ADC_ChannelConfTypeDef   ocxo_adc_channel_vcurr;
ADC_ChannelConfTypeDef   ocxo_adc_channel_vtemp;

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

    result = HAL_ADC_Start(&ocxo_adc_handle);
    if (result != HAL_OK)
        hal_perror("ocxo", "HAL_ADC_Start", result);

    printf("ocxo: adc running on vcurr and vtemp, 12bit resolution, 640.5 cycles\n");

}
