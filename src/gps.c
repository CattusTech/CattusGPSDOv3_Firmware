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
#include "gps.h"
#include "hardware_error.h"
#include <FreeRTOS.h>
#include <stdio.h>
#include <string.h>
#include <task.h>

GPIO_InitTypeDef         gpio_gps_rx;
GPIO_InitTypeDef         gpio_gps_tx;
UART_HandleTypeDef       gps_handle;
DMA_HandleTypeDef        gps_dma_handle;
RCC_PeriphCLKInitTypeDef gps_clk;

double       gps_latitude      = 0.0;
char         gps_latitude_chr  = 'N';
double       gps_longitude     = 0.0f;
char         gps_longitude_chr = 'E';
unsigned int gps_sv_number     = 0;
float        gps_hdop          = 0.0f;
int          gps_valid         = 0;
unsigned int gps_time_h        = 0;
unsigned int gps_time_m        = 0;
float        gps_time_s        = 0.0f;

void gps_init()
{
    gpio_gps_rx.Pin       = GPIO_PIN_7;
    gpio_gps_rx.Alternate = GPIO_AF7_USART1;
    gpio_gps_rx.Mode      = GPIO_MODE_AF_PP;
    gpio_gps_rx.Pull      = GPIO_PULLUP;
    gpio_gps_rx.Speed     = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &(gpio_gps_rx));

    gpio_gps_tx.Pin       = GPIO_PIN_6;
    gpio_gps_tx.Alternate = GPIO_AF7_USART1;
    gpio_gps_tx.Mode      = GPIO_MODE_AF_PP;
    gpio_gps_tx.Pull      = GPIO_PULLUP;
    gpio_gps_tx.Speed     = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &(gpio_gps_tx));

    gps_clk.PeriphClockSelection = RCC_PERIPHCLK_USART1;
    gps_clk.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;

    HAL_StatusTypeDef result = HAL_RCCEx_PeriphCLKConfig(&(gps_clk));
    if (result != HAL_OK)
        hal_perror("gps", "HAL_RCCEx_PeriphCLKConfig", result);

    __HAL_RCC_DMA2_CLK_ENABLE();

    gps_dma_handle.Instance                 = DMA2_Channel1;
    gps_dma_handle.Init.Request             = DMA_REQUEST_USART1_RX;
    gps_dma_handle.Init.PeriphInc           = DMA_PINC_DISABLE;
    gps_dma_handle.Init.MemInc              = DMA_MINC_ENABLE;
    gps_dma_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    gps_dma_handle.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    gps_dma_handle.Init.Mode                = DMA_NORMAL;
    gps_dma_handle.Init.Priority            = DMA_PRIORITY_LOW;

    result = HAL_DMA_Init(&gps_dma_handle);
    if (result != HAL_OK)
        hal_perror("gps", "HAL_DMA_Init", result);
    __HAL_LINKDMA(&gps_handle, hdmarx, gps_dma_handle);

    HAL_NVIC_SetPriority(DMA2_Channel1_IRQn, 1, 1);
    HAL_NVIC_EnableIRQ(DMA2_Channel1_IRQn);

    __HAL_RCC_USART1_CLK_ENABLE();

    gps_handle.Instance                    = USART1;
    gps_handle.Init.BaudRate               = 38400;
    gps_handle.Init.WordLength             = UART_WORDLENGTH_8B;
    gps_handle.Init.StopBits               = UART_STOPBITS_1;
    gps_handle.Init.Parity                 = UART_PARITY_NONE;
    gps_handle.Init.Mode                   = UART_MODE_TX_RX;
    gps_handle.Init.HwFlowCtl              = UART_HWCONTROL_NONE;
    gps_handle.Init.OverSampling           = UART_OVERSAMPLING_16;
    gps_handle.Init.OneBitSampling         = UART_ONE_BIT_SAMPLE_DISABLE;
    gps_handle.Init.ClockPrescaler         = UART_PRESCALER_DIV1;
    gps_handle.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_RXOVERRUNDISABLE_INIT;
    gps_handle.AdvancedInit.OverrunDisable = UART_ADVFEATURE_OVERRUN_DISABLE;

    result = HAL_UART_Init(&gps_handle);
    if (result != HAL_OK)
        hal_perror("gps", "HAL_USART_Init", result);
    result = HAL_UARTEx_SetTxFifoThreshold(&gps_handle, USART_TXFIFO_THRESHOLD_1_8);
    if (result != HAL_OK)
        hal_perror("gps", "HAL_USARTEx_SetTxFifoThreshold", result);
    result = HAL_UARTEx_SetRxFifoThreshold(&gps_handle, USART_TXFIFO_THRESHOLD_1_8);
    if (result != HAL_OK)
        hal_perror("gps", "HAL_USARTEx_SetRxFifoThreshold", result);
    result = HAL_UARTEx_DisableFifoMode(&gps_handle);
    if (result != HAL_OK)
        hal_perror("gps", "HAL_UARTEx_DisabeFifoMode", result);

    HAL_NVIC_SetPriority(USART1_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);

    printf("gps: up, listening to xxGGA on USART1 at 9600 8n1\n");
}

void gps_deinit()
{
    HAL_NVIC_DisableIRQ(USART1_IRQn);

    __HAL_RCC_USART1_FORCE_RESET();
    __HAL_RCC_USART1_RELEASE_RESET();
    HAL_UART_DeInit(&(gps_handle));

    __HAL_RCC_USART1_CLK_DISABLE();

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_7);
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_6);
}

char   uart_line_buffer[UART_LINE_BUFFER_SIZE];
size_t uart_line_ptr;
char*  gps_getline()
{
    uart_line_ptr = 0;
    memset(uart_line_buffer, 0, UART_LINE_BUFFER_SIZE);

    HAL_UART_Receive_IT(&gps_handle, (uint8_t*)uart_line_buffer + uart_line_ptr, 1); // emit!

    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    return uart_line_buffer;
}

int gps_update()
{
    char* message_line = gps_getline();

    unsigned int message_checksum = 0;
    for (size_t i = 0; i < strlen(message_line); i++)
    {
        if (message_line[i] == '$')
        {
            continue;
        }
        else if (message_line[i] == '*')
        {
            message_line[i] = ',';
            break;
        }
        message_checksum ^= message_line[i];
    }
    // -1   $xxGGA
    // 0    Time (Not Used)
    // 1    Latitude
    // 2    N/S
    // 3    Longitude
    // 4    E/W
    // 5    Quality
    // 6    SvNumber
    // 7    HDOP
    // 8    Altitude (Not Used)
    // 9    "M"
    // 10   Separation (Not Used)
    // 11   "M"
    // 12   DifferentialAge (Not Used)
    // 13   DifferentialStation (Not Used)
    // 14   Checksum
    char* message_line_delim_ptr = message_line;
    char* message_id             = strsep(&message_line_delim_ptr, ",");
    if (strcmp(message_id + 3, "GGA") != 0)
    {
        return 0;
    }
    else
    {
        const size_t entry_count = 15;
        char*        entry_list[entry_count];
        for (size_t i = 0; i < entry_count; i++)
        {
            entry_list[i] = strsep(&message_line_delim_ptr, ",");
        }
        // checksum
        unsigned int checksum_l4b[2] = { (message_checksum & 0xF0) >> 4, message_checksum & 0x0F };
        char         checksum_chr[2] = {
            (char)((checksum_l4b[0] <= 0x9) ? (checksum_l4b[0] + '0') : (checksum_l4b[0] - 10 + 'A')),
            (char)((checksum_l4b[1] <= 0x9) ? (checksum_l4b[1] + '0') : (checksum_l4b[1] - 10 + 'A'))
        };
        if (entry_list[14][0] != checksum_chr[0] || entry_list[14][1] != checksum_chr[1])
        {
            printf("gps: crtical warning! a checksum error has been detected, skipped.\n");
            return 0;
        }
        else
        {
            // time unused
            sscanf(entry_list[0], "%2u%2u%f", &gps_time_h, &gps_time_m, &gps_time_s);
            // latitude parser
            unsigned int latitude_degree;
            float        latitude_minute;
            sscanf(entry_list[1], "%2u%f", &latitude_degree, &latitude_minute);
            gps_latitude     = (float)latitude_degree + latitude_minute / 60.0f;
            gps_latitude_chr = entry_list[2][0];
            // longitiude parser
            unsigned int longitude_degree;
            float        longitude_minute;
            sscanf(entry_list[3], "%3u%f", &longitude_degree, &longitude_minute);
            gps_longitude     = (float)longitude_degree + longitude_minute / 60.0f;
            gps_longitude_chr = entry_list[4][0];
            // quality parser
            unsigned int quality;
            sscanf(entry_list[5], "%u", &quality);
            if (quality == 0 || quality == 6)
            {
                gps_valid = 0;
            }
            else if (gps_valid == 0)
            {
                printf("gps: first fix captured.\n");
                printf(
                    "gps: pos: %12.9lf%c,%12.9lf%c\n", gps_latitude, gps_latitude_chr, gps_longitude, gps_longitude_chr);
                gps_valid = 1;
            }
            else
            {
                gps_valid = 1;
            }
            // svnumber parser
            sscanf(entry_list[6], "%u", &(gps_sv_number));
            // HDOP
            sscanf(entry_list[7], "%f", &(gps_hdop));

            (void)entry_list[8];
            (void)entry_list[9];
            (void)entry_list[10];
            (void)entry_list[11];
            (void)entry_list[12];
            (void)entry_list[13];

            return 1;
        }
    }
    return 0;
}
