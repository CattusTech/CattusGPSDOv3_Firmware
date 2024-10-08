/**
  ******************************************************************************
  * @file    stm32g4xx_hal_timebase_tim_template.c
  * @author  MCD Application Team
  * @brief   HAL time base based on the hardware TIM Template.
  *
  *          This file override the native HAL time base functions (defined as weak)
  *          the TIM time base:
  *           + Initializes the TIM peripheral to generate a Period elapsed Event each 1ms
  *           + HAL_IncTick is called inside HAL_TIM_PeriodElapsedCallback ie each 1ms
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  @verbatim
  ==============================================================================
                        ##### How to use this driver #####
  ==============================================================================
    [..]
    This file must be copied to the application folder and modified as follows:
    (#) Rename it to 'stm32g4xx_hal_timebase_tim.c'
    (#) Add this file and the TIM HAL driver files to your project and make sure
       HAL_TIM_MODULE_ENABLED is defined in stm32g4xx_hal_conf.h

    [..]
    (@) The application needs to ensure that the time base is always set to 1 millisecond
       to have correct HAL operation.

  @endverbatim
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include <stm32g4xx_hal.h>

TIM_HandleTypeDef TimHandle;
HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
    RCC_ClkInitTypeDef clkconfig;
    uint32_t           uwTimclock;
    uint32_t           uwAPB1Prescaler;
    uint32_t           uwPrescalerValue;
    uint32_t           pFLatency;
    HAL_StatusTypeDef  status;

    /* Configure the TIM7 IRQ priority */
    HAL_NVIC_SetPriority(TIM7_IRQn, TickPriority, 0U);

    /* Enable the TIM7 global Interrupt */
    HAL_NVIC_EnableIRQ(TIM7_IRQn);

    /* Enable TIM7 clock */
    __HAL_RCC_TIM7_CLK_ENABLE();

    /* Get clock configuration */
    HAL_RCC_GetClockConfig(&clkconfig, &pFLatency);

    /* Get APB1 prescaler */
    uwAPB1Prescaler = clkconfig.APB1CLKDivider;

    /* Compute TIM7 clock */
    if (uwAPB1Prescaler == RCC_HCLK_DIV1)
    {
        uwTimclock = HAL_RCC_GetPCLK1Freq();
    }
    else
    {
        uwTimclock = 2U * HAL_RCC_GetPCLK1Freq();
    }

    /* Compute the prescaler value to have TIM7 counter clock equal to 1MHz */
    uwPrescalerValue = (uint32_t)((uwTimclock / 1000000U) - 1U);

    /* Initialize TIM7 */
    TimHandle.Instance = TIM7;

    /* Initialize TIMx peripheral as follow:
    + Period = [(TIM7CLK/1000) - 1]. to have a (1/1000) s time base.
    + Prescaler = (uwTimclock/1000000 - 1) to have a 1MHz counter clock.
    + ClockDivision = 0
    + Counter direction = Up
    */
    TimHandle.Init.Period        = (1000000U / 1000U) - 1U;
    TimHandle.Init.Prescaler     = uwPrescalerValue;
    TimHandle.Init.ClockDivision = 0;
    TimHandle.Init.CounterMode   = TIM_COUNTERMODE_UP;
    status                       = HAL_TIM_Base_Init(&TimHandle);
    if (status == HAL_OK)
    {
        /* Start the TIM time Base generation in interrupt mode */
        status = HAL_TIM_Base_Start_IT(&TimHandle);
        if (status == HAL_OK)
        {
            /* Configure the SysTick IRQ priority */
            if (TickPriority < (1UL << __NVIC_PRIO_BITS))
            {
                /* Configure the TIM IRQ priority */
                HAL_NVIC_SetPriority(TIM7_IRQn, TickPriority, 0U);
                uwTickPrio = TickPriority;
            }
            else
            {
                status = HAL_ERROR;
            }
        }
    }

    /* Return function status */
    return status;
}

/**
 * @brief  Suspend Tick increment.
 * @note   Disable the tick increment by disabling TIM7 update interrupt.
 * @param  None
 * @retval None
 */
void HAL_SuspendTick(void)
{
    /* Disable TIM7 update interrupt */
    __HAL_TIM_DISABLE_IT(&TimHandle, TIM_IT_UPDATE);
}

/**
 * @brief  Resume Tick increment.
 * @note   Enable the tick increment by enabling TIM7 update interrupt.
 * @param  None
 * @retval None
 */
void HAL_ResumeTick(void)
{
    /* Enable TIM7 update interrupt */
    __HAL_TIM_ENABLE_IT(&TimHandle, TIM_IT_UPDATE);
}

/**
 * @brief  Period elapsed callback in non blocking mode
 * @note   This function is called  when TIM7 interrupt took place, inside
 * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
 * a global variable "uwTick" used as application time base.
 * @param  htim : TIM handle
 * @retval None
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    (void)htim;
    HAL_IncTick();
}

/**
 * @brief  This function handles TIM interrupt request.
 * @param  None
 * @retval None
 */
void TIM7_DAC_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&TimHandle);
}

/**
 * @}
 */

/**
 * @}
 */
