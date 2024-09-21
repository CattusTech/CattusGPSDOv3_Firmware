#include <cstdio>
#include "swj.hpp"
using namespace periph;
swj::swj()
    : console(this->swo_baud)
{
    this->gpio_swdck.Pin       = GPIO_PIN_14;
    this->gpio_swdck.Alternate = GPIO_AF0_SWJ;
    this->gpio_swdck.Mode      = GPIO_MODE_AF_PP;
    this->gpio_swdck.Pull      = GPIO_PULLUP;
    this->gpio_swdck.Speed     = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &(this->gpio_swdck));

    this->gpio_swdio.Pin       = GPIO_PIN_13;
    this->gpio_swdio.Alternate = GPIO_AF0_SWJ;
    this->gpio_swdio.Mode      = GPIO_MODE_AF_PP;
    this->gpio_swdio.Pull      = GPIO_PULLUP;
    this->gpio_swdio.Speed     = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &(this->gpio_swdio));

    this->gpio_swo.Pin       = GPIO_PIN_3;
    this->gpio_swo.Alternate = GPIO_AF0_SWJ;
    this->gpio_swo.Mode      = GPIO_MODE_AF_PP;
    this->gpio_swo.Pull      = GPIO_PULLUP;
    this->gpio_swo.Speed     = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &(this->gpio_swo));
    
    std::printf("swj: initialized with Trace Async Sw mode, SWO baudrate: %lu\n", (unsigned long)this->swo_baud);
}
swj::~swj()
{
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_14);
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_13);
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_3);
}
swj::trace_console::trace_console(unsigned long baudrate)
{
    uint32_t SWOPrescaler = (HAL_RCC_GetSysClockFreq() / baudrate) - 1u;

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    DBGMCU->CR |= DBGMCU_CR_TRACE_IOEN;

    TPI->ACPR = SWOPrescaler;

    ITM->LAR = 0xC5ACCE55;
    ITM->TCR =
        (ITM_TCR_ITMENA_Msk | ITM_TCR_SYNCENA_Msk | ITM_TCR_DWTENA_Msk | ITM_TCR_SWOENA_Msk | ITM_TCR_TraceBusID_Msk);
    ITM->TPR = ITM_TPR_PRIVMASK_Msk;
    ITM->TER |= 0x00000001;

    DWT->CTRL  = 0x400003FE;
    TPI->FFCR = 0x00000100;
}

swj::trace_console::~trace_console()
{
    ITM->TER &= ~0x00000001;
}
