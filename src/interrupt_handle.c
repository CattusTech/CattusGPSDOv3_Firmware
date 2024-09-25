#include "gps.h"
#include "ocxo.h"
#include "screen.h"
#include <FreeRTOS.h>
#include <stm32g4xx_hal.h>
#include <task.h>
extern USART_HandleTypeDef gps_handle;
extern SPI_HandleTypeDef   screen_spi_handle;
extern ADC_HandleTypeDef   ocxo_adc_handle;
extern TIM_HandleTypeDef   counter_timer_handle;
extern TaskHandle_t        gps_task_handle;
extern TaskHandle_t        screen_task_handle;
extern TaskHandle_t        ocxo_task_handle;
extern TaskHandle_t        counter_task_handle;

void USART1_IRQHandler(void)
{
    HAL_USART_IRQHandler(&gps_handle);
    vTaskNotifyGiveFromISR(gps_task_handle, NULL);
}
void SPI2_IRQHandler(void)
{
    HAL_SPI_IRQHandler(&screen_spi_handle);
}
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    vTaskNotifyGiveFromISR(screen_task_handle, NULL);
}
void ADC1_2_IRQHandler(void)
{
    HAL_ADC_IRQHandler(&ocxo_adc_handle);
    vTaskNotifyGiveFromISR(ocxo_task_handle, NULL);
}
void TIM2_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&counter_timer_handle);
    vTaskNotifyGiveFromISR(counter_task_handle, NULL);
}
