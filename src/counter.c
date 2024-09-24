#include "hardware_error.h"
#include "ocxo.h"
#include <FreeRTOS.h>
#include <stm32g4xx_hal.h>
#include <task.h>
GPIO_InitTypeDef       gpio_counter_ref_pps;
GPIO_InitTypeDef       gpio_counter_ref_10m;
TIM_ClockConfigTypeDef counter_timer_clock;
TIM_HandleTypeDef      counter_timer_handle;
TIM_IC_InitTypeDef     counter_timer_ic;

unsigned int counter_gate;

void counter_init()
{
    gpio_counter_ref_pps.Pin       = GPIO_PIN_5;
    gpio_counter_ref_pps.Alternate = GPIO_AF10_TIM2;
    gpio_counter_ref_pps.Mode      = GPIO_MODE_AF_PP;
    gpio_counter_ref_pps.Pull      = GPIO_NOPULL;
    gpio_counter_ref_pps.Speed     = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &gpio_counter_ref_pps);

    gpio_counter_ref_pps.Pin       = GPIO_PIN_0;
    gpio_counter_ref_pps.Alternate = GPIO_AF10_TIM2;
    gpio_counter_ref_pps.Mode      = GPIO_MODE_AF_PP;
    gpio_counter_ref_pps.Pull      = GPIO_NOPULL;
    gpio_counter_ref_pps.Speed     = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(GPIOA, &gpio_counter_ref_10m);

    __HAL_RCC_TIM2_CLK_ENABLE();

    counter_timer_handle.Instance               = TIM2;
    counter_timer_handle.Init.Prescaler         = 0;
    counter_timer_handle.Init.CounterMode       = TIM_COUNTERMODE_UP;
    counter_timer_handle.Init.Period            = 10000000;
    counter_timer_handle.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    counter_timer_handle.Init.RepetitionCounter = 0;
    counter_timer_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

    HAL_StatusTypeDef result = HAL_TIM_IC_Init(&counter_timer_handle);
    if (result != HAL_OK)
        hal_perror("counter", "HAL_TIM_IC_Init", result);

    counter_timer_ic.ICPolarity  = TIM_INPUTCHANNELPOLARITY_RISING;
    counter_timer_ic.ICSelection = TIM_ICSELECTION_DIRECTTI;
    counter_timer_ic.ICPrescaler = TIM_ICPSC_DIV1;
    counter_timer_ic.ICFilter    = 0;

    result = HAL_TIM_IC_ConfigChannel(&counter_timer_handle, &counter_timer_ic, TIM_CHANNEL_1);
    if (result != HAL_OK)
        hal_perror("counter", "HAL_TIM_IC_ConfigChannel", result);

    counter_timer_clock.ClockSource    = TIM_CLOCKSOURCE_ETRMODE2;
    counter_timer_clock.ClockPolarity  = TIM_ETRPOLARITY_NONINVERTED;
    counter_timer_clock.ClockPrescaler = TIM_CLOCKPRESCALER_DIV1;
    counter_timer_clock.ClockFilter    = 0;

    result = HAL_TIM_ConfigClockSource(&counter_timer_handle, &counter_timer_clock);
    if (result != HAL_OK)
        hal_perror("counter", "HAL_TIM_ConfigClockSource", result);

    HAL_NVIC_SetPriority(TIM2_IRQn, 4, 0);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);

    result = HAL_TIM_IC_Start(&counter_timer_handle, TIM_CHANNEL_1);
    if (result != HAL_OK)
        hal_perror("counter", "HAL_TIM_IC_Start", result);

    printf("counter: TIM2 as counter timer, 20M period\n");
}
