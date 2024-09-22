#include "screen.h"
#include "hardware_error.h"
#include <FreeRTOS.h>
#include <stdio.h>
#include <task.h>
GPIO_InitTypeDef gpio_screen_mosi;
GPIO_InitTypeDef gpio_screen_sck;
GPIO_InitTypeDef gpio_screen_nss;
GPIO_InitTypeDef gpio_screen_dc;
GPIO_InitTypeDef gpio_screen_reset;

SPI_HandleTypeDef screen_handle;

u8g2_t  u8g2_handle;
uint8_t u8x8_byte_method(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr)
{
    (void)u8x8;
    switch (msg)
    {
        case U8X8_MSG_BYTE_SEND:
            HAL_SPI_Transmit_IT(&screen_handle, (uint8_t*)arg_ptr, arg_int);
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            break;
        case U8X8_MSG_BYTE_INIT:
            __HAL_SPI_DISABLE(&screen_handle);
            break;
        case U8X8_MSG_BYTE_SET_DC:
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, arg_int == 0 ? GPIO_PIN_RESET : GPIO_PIN_SET);
            break;
        case U8X8_MSG_BYTE_START_TRANSFER:
            __HAL_SPI_ENABLE(&screen_handle);
            break;
        case U8X8_MSG_BYTE_END_TRANSFER:
            __HAL_SPI_DISABLE(&screen_handle);
            break;
        default:
            return 0;
    }
    return 1;
};
uint8_t u8x8_gpio_and_delay_method(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr)
{
    (void)arg_ptr;
    switch (msg)
    {
        case U8X8_MSG_GPIO_AND_DELAY_INIT:
            break;
        case U8X8_MSG_DELAY_NANO:
            break;
        case U8X8_MSG_DELAY_10MICRO:
            break;
        case U8X8_MSG_DELAY_100NANO:
            break;
        case U8X8_MSG_DELAY_MILLI:
            vTaskDelay(arg_int);
            break;
        case U8X8_MSG_DELAY_I2C:
        case U8X8_MSG_GPIO_I2C_CLOCK:
            break;
        case U8X8_MSG_GPIO_I2C_DATA:
            break;
        default:
            u8x8_SetGPIOResult(u8x8, 1);
            break;
    }
    return 1;
};

void screen_init()
{
    gpio_screen_mosi.Pin       = GPIO_PIN_15;
    gpio_screen_mosi.Mode      = GPIO_MODE_AF_PP;
    gpio_screen_mosi.Pull      = GPIO_NOPULL;
    gpio_screen_mosi.Speed     = GPIO_SPEED_FREQ_MEDIUM;
    gpio_screen_mosi.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(GPIOB, &gpio_screen_mosi);

    gpio_screen_sck.Pin       = GPIO_PIN_13;
    gpio_screen_sck.Mode      = GPIO_MODE_AF_PP;
    gpio_screen_sck.Pull      = GPIO_NOPULL;
    gpio_screen_sck.Speed     = GPIO_SPEED_FREQ_MEDIUM;
    gpio_screen_sck.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(GPIOB, &gpio_screen_sck);

    gpio_screen_nss.Pin       = GPIO_PIN_12;
    gpio_screen_nss.Mode      = GPIO_MODE_AF_PP;
    gpio_screen_nss.Pull      = GPIO_PULLDOWN;
    gpio_screen_nss.Speed     = GPIO_SPEED_FREQ_MEDIUM;
    gpio_screen_nss.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(GPIOB, &gpio_screen_nss);

    gpio_screen_dc.Pin   = GPIO_PIN_11;
    gpio_screen_dc.Mode  = GPIO_MODE_AF_PP;
    gpio_screen_dc.Pull  = GPIO_NOPULL;
    gpio_screen_dc.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(GPIOB, &gpio_screen_dc);

    gpio_screen_reset.Pin   = GPIO_PIN_10;
    gpio_screen_reset.Mode  = GPIO_MODE_AF_PP;
    gpio_screen_reset.Pull  = GPIO_PULLDOWN;
    gpio_screen_reset.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(GPIOB, &gpio_screen_reset);

    __HAL_RCC_SPI1_CLK_ENABLE();

    screen_handle.Instance               = SPI2;
    screen_handle.Init.Mode              = SPI_MODE_MASTER;
    screen_handle.Init.Direction         = SPI_DIRECTION_2LINES;
    screen_handle.Init.DataSize          = SPI_DATASIZE_8BIT;
    screen_handle.Init.CLKPolarity       = SPI_POLARITY_HIGH;
    screen_handle.Init.CLKPhase          = SPI_PHASE_2EDGE;
    screen_handle.Init.NSS               = SPI_NSS_HARD_OUTPUT;
    screen_handle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16; // 9Mhz
    screen_handle.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    screen_handle.Init.TIMode            = SPI_TIMODE_DISABLE;
    screen_handle.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    screen_handle.Init.CRCPolynomial     = 7;
    screen_handle.Init.CRCLength         = SPI_CRC_LENGTH_DATASIZE;
    screen_handle.Init.NSSPMode          = SPI_NSS_PULSE_DISABLE;

    HAL_StatusTypeDef result = HAL_SPI_Init(&screen_handle);
    if (result != HAL_OK)
        hal_perror("screen", "HAL_SPI_Init", result);

    HAL_NVIC_SetPriority(SPI2_IRQn, 1, 1);
    HAL_NVIC_EnableIRQ(SPI2_IRQn);

    u8g2_Setup_sh1106_128x64_noname_f(&u8g2_handle, U8G2_R0, u8x8_byte_method, u8x8_gpio_and_delay_method);
    u8g2_InitDisplay(&u8g2_handle);
    printf("screen: setup for sh1106_128x64_noname_f\n");
    u8g2_SetPowerSave(&u8g2_handle, 0);
    u8g2_ClearBuffer(&u8g2_handle);
    u8g2_SetFont(&u8g2_handle, u8g2_font_spleen6x12_mf);
    u8g2_DrawStr(&u8g2_handle, 0, 12, "Initializing...");
    u8g2_SendBuffer(&u8g2_handle);
    printf("screen: \"Initializing...\" has been printed on the screen\n");
}
void screen_deinit()
{
    HAL_NVIC_DisableIRQ(SPI2_IRQn);

    __HAL_RCC_SPI1_FORCE_RESET();
    __HAL_RCC_SPI1_RELEASE_RESET();
    HAL_SPI_DeInit(&screen_handle);

    __HAL_RCC_SPI1_CLK_DISABLE();

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_15);
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_13);
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_12);
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_11);
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_10);
}
