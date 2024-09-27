#include "gps.h"
#include "hardware_error.h"
#include "ocxo.h"
#include "screen.h"
#include <FreeRTOS.h>
#include <stm32g4xx_hal.h>
#include <task.h>
extern UART_HandleTypeDef gps_handle;
extern SPI_HandleTypeDef  screen_spi_handle;
extern DMA_HandleTypeDef  screen_dma_handle;
extern ADC_HandleTypeDef  ocxo_adc_handle;
extern TIM_HandleTypeDef  counter_timer_handle;
extern TaskHandle_t       gps_task_handle;
extern TaskHandle_t       screen_task_handle;
extern TaskHandle_t       ocxo_task_handle;
extern TaskHandle_t       counter_task_handle;

void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&gps_handle);
}
#define UART_LINE_BUFFER_SIZE 256
// we have many messages to be parsed, let it go.
void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart)
{
    (void)huart;
    __HAL_UART_CLEAR_FLAG(&gps_handle, UART_CLEAR_OREF);
    HAL_UART_Receive_IT(&gps_handle, (uint8_t*)uart_line_buffer + uart_line_ptr, 1);
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart)
{
    (void)huart;
    if (uart_line_ptr >= UART_LINE_BUFFER_SIZE - 1)
        printf("gps: crtical warning! line length overflow\n");

    if (uart_line_buffer[uart_line_ptr] != '\n')
    {
        uart_line_ptr++;
        HAL_StatusTypeDef result = HAL_UART_Receive_IT(&gps_handle, (uint8_t*)uart_line_buffer + uart_line_ptr, 1);
        if (result != HAL_OK)
            hal_perror("gps", "HAL_UART_Receive_IT", result);
    }
    else
    {
        vTaskNotifyGiveFromISR(gps_task_handle, NULL);
    }
}
void SPI2_IRQHandler(void)
{
    HAL_SPI_IRQHandler(&screen_spi_handle);
}
void DMA1_Channel1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&screen_dma_handle);
}
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef* hspi)
{
    (void)hspi;
    vTaskNotifyGiveFromISR(screen_task_handle, NULL);
}
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef* hspi)
{
    (void)hspi;
    while (1)
    {
    }
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
