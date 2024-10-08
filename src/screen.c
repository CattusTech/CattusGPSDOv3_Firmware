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
#include "screen.h"
#include "counter.h"
#include "gps.h"
#include "hardware_error.h"
#include "ocxo.h"
#include <FreeRTOS.h>
#include <stdio.h>
#include <task.h>

GPIO_InitTypeDef  gpio_screen_mosi;
GPIO_InitTypeDef  gpio_screen_sck;
GPIO_InitTypeDef  gpio_screen_cs;
GPIO_InitTypeDef  gpio_screen_dc;
GPIO_InitTypeDef  gpio_screen_reset;
TIM_HandleTypeDef screen_timer_handle;
SPI_HandleTypeDef screen_spi_handle;
DMA_HandleTypeDef screen_dma_handle;

u8g2_t  u8g2_handle;
uint8_t u8x8_byte_method(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr)
{
    HAL_StatusTypeDef result;
    switch (msg)
    {
        case U8X8_MSG_BYTE_SEND:
            HAL_SPI_StateTypeDef state = HAL_SPI_GetState(&screen_spi_handle);
            if (state != HAL_SPI_STATE_READY)
                hal_perror("screen", "HAL_SPI_GetState", state);
            result = HAL_SPI_Transmit_IT(&screen_spi_handle, (uint8_t*)arg_ptr, arg_int);
            if (result != HAL_OK)
                hal_perror("screen", "HAL_SPI_Transmit_DMA", result);
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            break;
        case U8X8_MSG_BYTE_INIT:
            u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);
            u8x8_gpio_SetReset(u8x8, 0);
            u8x8->gpio_and_delay_cb(u8x8, U8X8_MSG_DELAY_MILLI, u8x8->display_info->reset_pulse_width_ms, NULL);
            u8x8_gpio_SetReset(u8x8, 1);
            u8x8->gpio_and_delay_cb(u8x8, U8X8_MSG_DELAY_MILLI, u8x8->display_info->post_reset_wait_ms, NULL);
            break;
        case U8X8_MSG_BYTE_SET_DC:
            u8x8_gpio_SetDC(u8x8, arg_int);
            break;
        case U8X8_MSG_BYTE_START_TRANSFER:
            u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_enable_level);
            u8x8->gpio_and_delay_cb(u8x8, U8X8_MSG_DELAY_NANO, u8x8->display_info->post_chip_enable_wait_ns, NULL);
            break;
        case U8X8_MSG_BYTE_END_TRANSFER:
            u8x8->gpio_and_delay_cb(u8x8, U8X8_MSG_DELAY_NANO, u8x8->display_info->post_chip_enable_wait_ns, NULL);
            u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);
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
            for (size_t i = 0; i < arg_int; i++)
            {
                __NOP();
            }
            break;
        case U8X8_MSG_DELAY_10MICRO:
            break;
        case U8X8_MSG_DELAY_100NANO:
            break;
        case U8X8_MSG_DELAY_MILLI:
            vTaskDelay(arg_int);
            break;
        case U8X8_MSG_DELAY_I2C:
        case U8X8_MSG_GPIO_CS:
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, arg_int);
            break;
        case U8X8_MSG_GPIO_DC:
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, arg_int);
            break;
        case U8X8_MSG_GPIO_RESET:
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, arg_int);
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

    gpio_screen_cs.Pin   = GPIO_PIN_12;
    gpio_screen_cs.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio_screen_cs.Pull  = GPIO_PULLUP;
    gpio_screen_cs.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(GPIOB, &gpio_screen_cs);

    gpio_screen_dc.Pin   = GPIO_PIN_11;
    gpio_screen_dc.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio_screen_dc.Pull  = GPIO_NOPULL;
    gpio_screen_dc.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(GPIOB, &gpio_screen_dc);

    gpio_screen_reset.Pin   = GPIO_PIN_10;
    gpio_screen_reset.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio_screen_reset.Pull  = GPIO_PULLDOWN;
    gpio_screen_reset.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(GPIOB, &gpio_screen_reset);

    __HAL_RCC_DMAMUX1_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();

    screen_dma_handle.Instance                 = DMA1_Channel1;
    screen_dma_handle.Init.Request             = DMA_REQUEST_SPI2_TX;
    screen_dma_handle.Init.PeriphInc           = DMA_PINC_DISABLE;
    screen_dma_handle.Init.MemInc              = DMA_MINC_ENABLE;
    screen_dma_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    screen_dma_handle.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    screen_dma_handle.Init.Mode                = DMA_NORMAL;
    screen_dma_handle.Init.Priority            = DMA_PRIORITY_LOW;

    HAL_StatusTypeDef result = HAL_DMA_Init(&screen_dma_handle);
    if (result != HAL_OK)
        hal_perror("screen", "HAL_DMA_Init", result);
    __HAL_LINKDMA(&screen_spi_handle, hdmatx, screen_dma_handle);

    HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 2, 1);
    HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

    __HAL_RCC_SPI2_CLK_ENABLE();

    screen_spi_handle.Instance               = SPI2;
    screen_spi_handle.Init.Mode              = SPI_MODE_MASTER;
    screen_spi_handle.Init.Direction         = SPI_DIRECTION_2LINES;
    screen_spi_handle.Init.DataSize          = SPI_DATASIZE_8BIT;
    screen_spi_handle.Init.CLKPolarity       = SPI_POLARITY_HIGH;
    screen_spi_handle.Init.CLKPhase          = SPI_PHASE_2EDGE;
    screen_spi_handle.Init.NSS               = SPI_NSS_SOFT;
    screen_spi_handle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64; // 2.25Mhz
    screen_spi_handle.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    screen_spi_handle.Init.TIMode            = SPI_TIMODE_DISABLE;
    screen_spi_handle.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    screen_spi_handle.Init.CRCPolynomial     = 7;
    screen_spi_handle.Init.CRCLength         = SPI_CRC_LENGTH_DATASIZE;
    screen_spi_handle.Init.NSSPMode          = SPI_NSS_PULSE_DISABLE;

    result = HAL_SPI_Init(&screen_spi_handle);
    if (result != HAL_OK)
        hal_perror("screen", "HAL_SPI_Init", result);

    HAL_NVIC_SetPriority(SPI2_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(SPI2_IRQn);

    printf("screen: ready\n");
    u8g2_Setup_sh1106_128x64_noname_f(&u8g2_handle, U8G2_R0, u8x8_byte_method, u8x8_gpio_and_delay_method);
    uint8_t* buf = (uint8_t*)pvPortMalloc(u8g2_GetBufferSize(&u8g2_handle)); // dynamically allocate a buffer of the
                                                                             // required size
    if (buf == NULL)
        hal_perror("screen", "pvPortMalloc", buf);
    u8g2_SetBufferPtr(&u8g2_handle, buf);

    u8g2_InitDisplay(&u8g2_handle);
    printf("screen: setup for sh1106_128x64_noname_f\n");
    u8g2_SetPowerSave(&u8g2_handle, 0);
    u8g2_ClearBuffer(&u8g2_handle);
    u8g2_SetFont(&u8g2_handle, u8g2_font_profont12_mf);
    u8g2_DrawStr(&u8g2_handle, 0, 12, "Initializing...");
    u8g2_SendBuffer(&u8g2_handle);
    printf("screen: \"Initializing...\" has been printed on the screen\n");
}
void screen_update()
{
    // generated by https://lopaka.app/sandbox
    u8g2_ClearBuffer(&u8g2_handle);
    u8g2_SetBitmapMode(&u8g2_handle, 1);
    u8g2_SetFontMode(&u8g2_handle, 1);
    u8g2_SetFont(&u8g2_handle, u8g2_font_profont12_mf);

    char screen_string_buffer[32];
    u8g2_DrawStr(&u8g2_handle, 1, 9, "Cattus GNSSDOv3 FW:01");
    u8g2_DrawLine(&u8g2_handle, 0, 11, 127, 11);

    u8g2_DrawStr(&u8g2_handle, 4, 22, "GNSS");
    sprintf(screen_string_buffer, "%02u", gps_sv_number);
    u8g2_DrawStr(&u8g2_handle, 40, 22, screen_string_buffer);

    u8g2_DrawStr(&u8g2_handle, 64, 22, "OCXO");
    if (ocxo_overheat)
    {
        u8g2_DrawStr(&u8g2_handle, 100, 22, "OVHT!");
    }
    else
    {
        sprintf(screen_string_buffer, "%04X", ocxo_vtune_bin);
        u8g2_DrawStr(&u8g2_handle, 100, 22, screen_string_buffer);
    }

    sprintf(screen_string_buffer, "Lat:  %12.9lf\xb0%c", gps_latitude, gps_latitude_chr);
    u8g2_DrawStr(&u8g2_handle, 4, 33, screen_string_buffer);

    sprintf(screen_string_buffer, "Lon: %13.9lf\xb0%c", gps_longitude, gps_longitude_chr);
    u8g2_DrawStr(&u8g2_handle, 4, 43, screen_string_buffer);

    sprintf(screen_string_buffer, "Freq: %12.9lfMhz", counter_cycle / 1000000.0f);
    u8g2_DrawStr(&u8g2_handle, 4, 52, screen_string_buffer);

    sprintf(screen_string_buffer, "%-4u", counter_gate);
    u8g2_DrawStr(&u8g2_handle, 4, 62, screen_string_buffer);

    sprintf(screen_string_buffer, "P:%04.2lf", gps_hdop);
    u8g2_DrawStr(&u8g2_handle, 34, 62, screen_string_buffer);

    sprintf(screen_string_buffer, "%02u:%02u:%02u", gps_time_h, gps_time_m, (unsigned int)gps_time_s);
    u8g2_DrawStr(&u8g2_handle, 76, 62, screen_string_buffer);

    if (gps_valid)
    {
        if (gps_hdop > 2)
        {
            u8g2_SetFont(&u8g2_handle, u8g2_font_open_iconic_embedded_1x_t);
            u8g2_DrawGlyph(&u8g2_handle, 30, 22, 71); // warning mark
        }
        else
        {
            u8g2_SetFont(&u8g2_handle, u8g2_font_open_iconic_check_1x_t);
            u8g2_DrawGlyph(&u8g2_handle, 30, 22, 64); // check mark
        }
    }
    else
    {
        u8g2_SetFont(&u8g2_handle, u8g2_font_open_iconic_check_1x_t);
        u8g2_DrawGlyph(&u8g2_handle, 30, 22, 68); // cross mark
    }

    if (ocxo_overheat)
    {
        if (ocxo_valid)
        {
            u8g2_SetFont(&u8g2_handle, u8g2_font_open_iconic_check_1x_t);
            u8g2_DrawGlyph(&u8g2_handle, 90, 22, 64); // check mark
        }
        else
        {
            u8g2_SetFont(&u8g2_handle, u8g2_font_open_iconic_check_1x_t);
            u8g2_DrawGlyph(&u8g2_handle, 90, 22, 68); // cross mark
        }
    }
    else
    {
        u8g2_SetFont(&u8g2_handle, u8g2_font_open_iconic_embedded_1x_t);
        u8g2_DrawGlyph(&u8g2_handle, 90, 22, 71); // warning mark
    }

    u8g2_SendBuffer(&u8g2_handle);
};
void screen_deinit()
{
    HAL_NVIC_DisableIRQ(SPI2_IRQn);

    __HAL_RCC_SPI1_FORCE_RESET();
    __HAL_RCC_SPI1_RELEASE_RESET();
    HAL_SPI_DeInit(&screen_spi_handle);

    __HAL_RCC_SPI1_CLK_DISABLE();

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_15);
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_13);
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_12);
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_11);
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_10);
}
