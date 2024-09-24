#include "clock.h"
#include "counter.h"
#include "gps.h"
#include "hardware_error.h"
#include "ocxo.h"
#include "screen.h"
#include "swj.h"
#include <FreeRTOS.h>
#include <inttypes.h>
#include <message_buffer.h>
#include <stdio.h>
#include <stm32g4xx_hal.h>
#include <task.h>

static void watchdog_task(void* v)
{
    (void)v;
    static IWDG_HandleTypeDef iwdg_handle;
    iwdg_handle.Instance       = IWDG;
    iwdg_handle.Init.Prescaler = IWDG_PRESCALER_32;
    iwdg_handle.Init.Window    = 4095;
    iwdg_handle.Init.Reload    = 1000;

    HAL_IWDG_Init(&iwdg_handle);

    printf("watchdog: started with %" PRIu32 " ms\n", iwdg_handle.Init.Reload);

    while (1)
    {
        HAL_IWDG_Refresh(&iwdg_handle);
        const TickType_t xDelay = 750 / portTICK_PERIOD_MS;

        vTaskDelay(xDelay);
    }
}
void gps_task(void* v)
{
    (void)v;
    gps_init();
    while (1)
    {
        gps_update(); // will block when no data available
    }
}
void screen_task(void* v)
{
    (void)v;
    screen_init();
    while (1)
    {
        screen_update();
        vTaskDelay(100); // nobody cares about fixed freshrate ~10Hz
    }
}
void ocxo_task(void* v)
{
    (void)v;
    ocxo_init();
    while (1)
    {
        ocxo_update(); // will block when no data available
        vTaskDelay(100);
    }
}
void counter_task(void* v)
{
    (void)v;
    counter_init();
    while (1)
    {
        counter_update(); // will block when no data available
    }
}
void print_reset_cause()
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

void vApplicationStackOverflowHook(TaskHandle_t xTask, char* pcTaskName)
{
    (void)xTask;
    while(1)
    {

    }
}

uint8_t      ucHeap[configTOTAL_HEAP_SIZE] __attribute__((section("._user_heap_stack")));
TaskHandle_t gps_task_handle;
TaskHandle_t screen_task_handle;
TaskHandle_t ocxo_task_handle;
TaskHandle_t counter_task_handle;

int main()
{
    HAL_Init();
    clock_init();
    swj_init(2000000);
    printf("Cattus GPSDOv3, Build time:%s %s\n", __DATE__, __TIME__);
    printf("System clock: %" PRIu32 "MHz, ", HAL_RCC_GetSysClockFreq() / 1000000);
    print_reset_cause();

    HAL_NVIC_SetPriority(PendSV_IRQn, 15, 0);
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

    BaseType_t result;
    result = xTaskCreate(watchdog_task, "watchdog_task", 32, NULL, configMAX_PRIORITIES - 1, NULL);
    if (result != pdPASS)
        hal_perror("freertos", "xTaskCreate", result);
    //result = xTaskCreate(gps_task, "gps_task", 256, NULL, 1, &gps_task_handle);
    if (result != pdPASS)
        hal_perror("freertos", "xTaskCreate", result);
    result = xTaskCreate(screen_task, "screen_task", 256, NULL, 2, &screen_task_handle);
    if (result != pdPASS)
        hal_perror("freertos", "xTaskCreate", result);
    result = xTaskCreate(ocxo_task, "ocxo_task", 256, NULL, 3, &ocxo_task_handle);
    if (result != pdPASS)
        hal_perror("freertos", "xTaskCreate", result);
    result = xTaskCreate(counter_task, "counter_task", 256, NULL, 4, &counter_task_handle);
    if (result != pdPASS)
        hal_perror("freertos", "xTaskCreate", result);

    printf("freertos: started\n");
    vTaskStartScheduler();

    while (1)
    {
    }
};
