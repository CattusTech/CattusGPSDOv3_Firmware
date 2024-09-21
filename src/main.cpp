#include "clock.hpp"
#include "swj.hpp"
#include "clock.hpp"
#include <FreeRTOS.h>
#include <cinttypes>
#include <cstdio>
#include <stm32g4xx_hal.h>
#include <task.h>

static void watchdog_task([[maybe_unused]] void* v)
{
    static IWDG_HandleTypeDef iwdg_handle;
    iwdg_handle.Instance       = IWDG;
    iwdg_handle.Init.Prescaler = IWDG_PRESCALER_32;
    iwdg_handle.Init.Window    = 4095;
    iwdg_handle.Init.Reload    = 1000;

    HAL_IWDG_Init(&iwdg_handle);

    std::printf("watchdog: started with %" PRIu32 " ms\n", iwdg_handle.Init.Reload);

    while (true)
    {
        HAL_IWDG_Refresh(&iwdg_handle);
        const TickType_t xDelay = 750 / portTICK_PERIOD_MS;

        vTaskDelay(xDelay);
    }
}

static void print_reset_cause()
{
    if (__HAL_RCC_GET_FLAG(RCC_FLAG_BORRST))
        printf("Last reset: Brown Out\n");
    if (__HAL_RCC_GET_FLAG(RCC_FLAG_OBLRST))
        printf("Last reset: Option Byte Loader\n");
    if (__HAL_RCC_GET_FLAG(RCC_FLAG_PINRST))
        printf("Last reset: External Pin\n");
    if (__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST))
        printf("Last reset: Software\n");
    if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST))
        printf("Last reset: IWDG\n");
    if (__HAL_RCC_GET_FLAG(RCC_FLAG_WWDGRST))
        printf("Last reset: WWDG\n");
    if (__HAL_RCC_GET_FLAG(RCC_FLAG_LPWRRST))
        printf("Last reset: Low Power\n");
    __HAL_RCC_CLEAR_RESET_FLAGS();
}

int main()
{
    HAL_Init();
    core::clock_init();
    auto swj_handle = periph::swj();

    printf("Cattus GPSDOv3, Build time:%s %s\n", __DATE__, __TIME__);
    printf("System clock: %" PRIu32 "MHz, ", HAL_RCC_GetSysClockFreq() / 1000000);
    print_reset_cause();

    HAL_NVIC_SetPriority(PendSV_IRQn, 15, 0);
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

    xTaskCreate(watchdog_task, "watchdog_task", 128, nullptr, configMAX_PRIORITIES - 1, nullptr);

    printf("freertos: started\n");
    vTaskStartScheduler();

    while (true)
    {

    }
};
