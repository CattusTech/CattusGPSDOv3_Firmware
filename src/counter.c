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
#include "ocxo.h"
#include <FreeRTOS.h>
#include <arm_math.h>
#include <stm32g4xx_hal.h>
#include <task.h>

GPIO_InitTypeDef        gpio_counter_ref_pps;
GPIO_InitTypeDef        gpio_counter_ref_10m;
TIM_ClockConfigTypeDef  counter_timer_clock;
TIM_HandleTypeDef       counter_timer_handle;
TIM_MasterConfigTypeDef counter_timer_masterconf;
TIM_SlaveConfigTypeDef  counter_timer_slaveconf;
TIM_IC_InitTypeDef      counter_timer_ic;
arm_pid_instance_f32    counter_pid;
unsigned int            counter_gate  = 1;
float                   counter_cycle = 0.0f;

void counter_init()
{
    gpio_counter_ref_pps.Pin       = GPIO_PIN_5;
    gpio_counter_ref_pps.Alternate = GPIO_AF1_TIM2;
    gpio_counter_ref_pps.Mode      = GPIO_MODE_AF_PP;
    gpio_counter_ref_pps.Pull      = GPIO_NOPULL;
    gpio_counter_ref_pps.Speed     = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &gpio_counter_ref_pps);

    gpio_counter_ref_10m.Pin       = GPIO_PIN_0;
    gpio_counter_ref_10m.Alternate = GPIO_AF14_TIM2;
    gpio_counter_ref_10m.Mode      = GPIO_MODE_AF_PP;
    gpio_counter_ref_10m.Pull      = GPIO_NOPULL;
    gpio_counter_ref_10m.Speed     = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(GPIOA, &gpio_counter_ref_10m);

    __HAL_RCC_TIM2_CLK_ENABLE();

    counter_timer_handle.Instance               = TIM2;
    counter_timer_handle.Init.Prescaler         = 0;
    counter_timer_handle.Init.CounterMode       = TIM_COUNTERMODE_UP;
    counter_timer_handle.Init.Period            = 20000000;
    counter_timer_handle.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    counter_timer_handle.Init.RepetitionCounter = 0;
    counter_timer_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    HAL_StatusTypeDef result = HAL_TIM_Base_Init(&counter_timer_handle);
    if (result != HAL_OK)
        hal_perror("counter", "HAL_TIM_Base_Init", result);

    counter_timer_clock.ClockSource    = TIM_CLOCKSOURCE_ETRMODE2;
    counter_timer_clock.ClockPolarity  = TIM_ETRPOLARITY_NONINVERTED;
    counter_timer_clock.ClockPrescaler = TIM_CLOCKPRESCALER_DIV1;
    counter_timer_clock.ClockFilter    = 0;

    result = HAL_TIM_ConfigClockSource(&counter_timer_handle, &counter_timer_clock);
    if (result != HAL_OK)
        hal_perror("counter", "HAL_TIM_ConfigClockSource", result);

    result = HAL_TIM_IC_Init(&counter_timer_handle);
    if (result != HAL_OK)
        hal_perror("counter", "HAL_TIM_IC_Init", result);

    counter_timer_slaveconf.SlaveMode       = TIM_SLAVEMODE_RESET;
    counter_timer_slaveconf.InputTrigger    = TIM_TS_TI1FP1;
    counter_timer_slaveconf.TriggerPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
    counter_timer_slaveconf.TriggerFilter   = 0;

    result = HAL_TIM_SlaveConfigSynchro(&counter_timer_handle, &counter_timer_slaveconf);
    if (result != HAL_OK)
        hal_perror("counter", "HAL_TIM_SlaveConfigSynchro", result);

    counter_timer_masterconf.MasterOutputTrigger  = TIM_TRGO_RESET;
    counter_timer_masterconf.MasterOutputTrigger2 = TIM_TRGO2_RESET;
    counter_timer_masterconf.MasterSlaveMode      = TIM_MASTERSLAVEMODE_DISABLE;

    result = HAL_TIMEx_MasterConfigSynchronization(&counter_timer_handle, &counter_timer_masterconf);
    if (result != HAL_OK)
        hal_perror("counter", "HAL_TIMEx_MasterConfigSynchronization", result);

    counter_timer_ic.ICPolarity  = TIM_INPUTCHANNELPOLARITY_RISING;
    counter_timer_ic.ICSelection = TIM_ICSELECTION_DIRECTTI;
    counter_timer_ic.ICPrescaler = TIM_ICPSC_DIV1;
    counter_timer_ic.ICFilter    = 0;

    result = HAL_TIM_IC_ConfigChannel(&counter_timer_handle, &counter_timer_ic, TIM_CHANNEL_1);
    if (result != HAL_OK)
        hal_perror("counter", "HAL_TIM_IC_ConfigChannel", result);

    HAL_NVIC_SetPriority(TIM2_IRQn, 4, 0);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);

    result = HAL_TIM_IC_Start_IT(&counter_timer_handle, TIM_CHANNEL_1);
    if (result != HAL_OK)
        hal_perror("counter", "HAL_TIM_IC_Start_IT", result);

    printf("counter: TIM2 as counter timer, 10M period\n");

    counter_pid.Kp = 3.3232e-3f;
    counter_pid.Ki = 1.7655e-3f;
    counter_pid.Kd = 0.0f;
    arm_pid_init_f32(&counter_pid, 0);
    printf("counter: PID controller, Kp:%f, Ki:%f, Kd:%f\n", counter_pid.Kp, counter_pid.Ki, counter_pid.Kd);
}
static inline uint16_t caculate_dac_voltage(float voltage)
{
    return voltage / 3.0f * 0xfff;
}
void counter_update()
{
    for (size_t i = 0; i < counter_gate; i++)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }
    unsigned int captured_cycle = HAL_TIM_ReadCapturedValue(&counter_timer_handle, TIM_CHANNEL_1);

    int cycle_difference = captured_cycle - 10000000;
    counter_cycle        = (float)cycle_difference / counter_gate + 10000000.0f;

    printf(
        "counter: gate: %u, captured: %u, diff: %d, calc: %fMhz\n",
        counter_gate,
        captured_cycle,
        cycle_difference,
        counter_cycle / 1000000.0f);

    if (cycle_difference == 0 && counter_gate != 1000)
    {
        // counter_gate *= 10;
    }
    else
    {
        float next_voltage = arm_pid_f32(&counter_pid, cycle_difference);
        ocxo_vtune_bin     = caculate_dac_voltage((next_voltage + 2) / 2);
        printf("counter: cycle delta: %d, next voltage: %fV\n", cycle_difference, next_voltage);
    }
}
