#pragma once
#include <stm32g4xx_hal.h>
namespace periph
{
    class swj
    {
        class trace_console
        {
        public:
            trace_console(unsigned long baudrate);
            ~trace_console();
        };

    public:
        static constexpr auto swo_baud = 2000000;

    private:
        GPIO_InitTypeDef gpio_swdio;
        GPIO_InitTypeDef gpio_swdck;
        GPIO_InitTypeDef gpio_swo;
        trace_console    console;

    public:
        swj();
        ~swj();
    };
} // namespace periph
