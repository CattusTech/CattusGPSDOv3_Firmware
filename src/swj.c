#include "swj.h"
#include <stdio.h>
GPIO_InitTypeDef gpio_swdio;
GPIO_InitTypeDef gpio_swdck;
GPIO_InitTypeDef gpio_swo;

void swj_init(size_t baudrate)
{
    gpio_swdck.Pin       = GPIO_PIN_14;
    gpio_swdck.Alternate = GPIO_AF0_SWJ;
    gpio_swdck.Mode      = GPIO_MODE_AF_PP;
    gpio_swdck.Pull      = GPIO_PULLUP;
    gpio_swdck.Speed     = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio_swdck);

    gpio_swdio.Pin       = GPIO_PIN_13;
    gpio_swdio.Alternate = GPIO_AF0_SWJ;
    gpio_swdio.Mode      = GPIO_MODE_AF_PP;
    gpio_swdio.Pull      = GPIO_PULLUP;
    gpio_swdio.Speed     = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio_swdio);

    gpio_swo.Pin       = GPIO_PIN_3;
    gpio_swo.Alternate = GPIO_AF0_SWJ;
    gpio_swo.Mode      = GPIO_MODE_AF_PP;
    gpio_swo.Pull      = GPIO_PULLUP;
    gpio_swo.Speed     = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &gpio_swo);

    uint32_t SWOPrescaler = (HAL_RCC_GetSysClockFreq() / baudrate) - 1u;

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    DBGMCU->CR |= DBGMCU_CR_TRACE_IOEN;

    TPI->ACPR = SWOPrescaler;

    ITM->LAR = 0xC5ACCE55;
    ITM->TCR =
        (ITM_TCR_ITMENA_Msk | ITM_TCR_SYNCENA_Msk | ITM_TCR_DWTENA_Msk | ITM_TCR_SWOENA_Msk | ITM_TCR_TraceBusID_Msk);
    ITM->TPR = ITM_TPR_PRIVMASK_Msk;
    ITM->TER |= 0x00000001;

    DWT->CTRL = 0x400003FE;
    TPI->FFCR = 0x00000100;

    printf("swj: initialized with Trace Async Sw mode, SWO baudrate: %lu\n", (unsigned long)baudrate);
}
void swj_deinit()
{
    ITM->TER &= ~0x00000001;

    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_14);
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_13);
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_3);
}
