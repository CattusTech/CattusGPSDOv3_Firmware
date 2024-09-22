#include "gps.h"
#include "hardware_error.h"
#include <FreeRTOS.h>
#include <stdio.h>
#include <string.h>
#include <task.h>

GPIO_InitTypeDef         gpio_gps_rx;
GPIO_InitTypeDef         gpio_gps_tx;
USART_HandleTypeDef      gps_handle;
RCC_PeriphCLKInitTypeDef gps_clk;

float        gps_latitude;
char         gps_latitude_chr;
float        gps_longitude;
char         gps_longitude_chr;
unsigned int gps_sv_number;
float        gps_hdop;
int          gps_valid;
char         gps_mode;

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

    auto result = HAL_RCCEx_PeriphCLKConfig(&(gps_clk));
    if (result != HAL_OK)
        hal_perror("gps", "HAL_RCCEx_PeriphCLKConfig", result);

    __HAL_RCC_USART1_CLK_ENABLE();

    gps_handle.Instance            = USART1;
    gps_handle.Init.BaudRate       = 9600;
    gps_handle.Init.WordLength     = USART_WORDLENGTH_8B;
    gps_handle.Init.StopBits       = USART_STOPBITS_1;
    gps_handle.Init.Parity         = USART_PARITY_NONE;
    gps_handle.Init.Mode           = USART_MODE_TX_RX;
    gps_handle.Init.CLKPolarity    = USART_POLARITY_LOW;
    gps_handle.Init.CLKPhase       = USART_PHASE_1EDGE;
    gps_handle.Init.CLKLastBit     = USART_LASTBIT_ENABLE;
    gps_handle.Init.ClockPrescaler = USART_PRESCALER_DIV1;
    gps_handle.SlaveMode           = USART_SLAVEMODE_DISABLE;

    result = HAL_USART_Init(&gps_handle);
    if (result != HAL_OK)
        hal_perror("gps", "HAL_USART_Init", result);
    result = HAL_USARTEx_SetTxFifoThreshold(&gps_handle, USART_TXFIFO_THRESHOLD_1_8);
    if (result != HAL_OK)
        hal_perror("gps", "HAL_USARTEx_SetTxFifoThreshold", result);
    result = HAL_USARTEx_SetRxFifoThreshold(&gps_handle, USART_RXFIFO_THRESHOLD_1_8);
    if (result != HAL_OK)
        hal_perror("gps", "HAL_USARTEx_SetRxFifoThreshold", result);
    result = HAL_USARTEx_DisableFifoMode(&gps_handle);
    if (result != HAL_OK)
        hal_perror("gps", "HAL_USARTEx_DisableFifoMode", result);

    HAL_NVIC_SetPriority(USART1_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);

    printf("gps: up, listening to xxGGA on USART1 at 9600 8n1\n");
}

void gps_deinit()
{
    HAL_NVIC_DisableIRQ(USART1_IRQn);

    __HAL_RCC_USART1_FORCE_RESET();
    __HAL_RCC_USART1_RELEASE_RESET();
    HAL_USART_DeInit(&(gps_handle));

    __HAL_RCC_USART1_CLK_DISABLE();

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_7);
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_6);
}

void gps_update()
{
    const size_t message_buffer_size = 512;
    char         message_buffer[message_buffer_size];
    HAL_USART_Receive_IT(&(gps_handle), (uint8_t*)(message_buffer), message_buffer_size - 1);

    auto status = HAL_USART_GetState(&(gps_handle));
    if (status == HAL_USART_STATE_BUSY_RX)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }
    else if (status == HAL_USART_STATE_ERROR)
    {
        hal_perror("gps", "HAL_USART_GetState", status);
    }

    if (strlen(message_buffer) == message_buffer_size - 1)
    {
        printf("gps: panic! message buffer overflow\n");
        while (1)
        {
        }
    }
    char* message_buffer_delim_saveptr;
    char* message_line = strtok_r(message_buffer, "\r\n", &message_buffer_delim_saveptr);
    while (message_line != NULL)
    {
        unsigned int message_checksum = 0;
        for (size_t i = 0; i < strlen(message_line); i++)
        {
            if (message_line[i] == '$')
            {
                continue;
            }
            else if (message_line[i] == '*')
            {
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
        char*  message_line_delim_saveptr;
        size_t message_id = strtok_r(message_line, ",", &message_line_delim_saveptr);
        if (strcmp(message_id + 3, "GGA") != 0)
        {
            goto message_line_next;
        }
        else
        {
            const size_t entry_count = 14;
            char*        entry_list[entry_count];
            for (size_t i = 0; i < entry_count; i++)
            {
                entry_list[i] = strtok_r(NULL, ",", &message_line_delim_saveptr);
            }
            // checksum
            unsigned int checksum_l4b[2] = { (message_checksum & 0xF0) >> 4, message_checksum & 0x0F };
            char         checksum_chr[2] = {
                (char)((checksum_l4b[0] <= 0x9) ? (checksum_l4b[0] + '0') : (checksum_l4b[0] - 10 + 'A')),
                (char)((checksum_l4b[1] <= 0x9) ? (checksum_l4b[1] + '0') : (checksum_l4b[1] - 10 + 'A'))
            };
            if (entry_list[14][1] != checksum_chr[0] || entry_list[14][2] != checksum_chr[1])
            {
                printf("gps: crtical warning! a checksum error has been detected, skipped.\n");
                goto message_line_next;
            }
            else
            {
                // time unused
                (void)entry_list[0];
                // latitude parser
                unsigned int latitude_degree;
                float        latitude_minute;
                sscanf(entry_list[1], "%2u%f", &latitude_degree, &latitude_minute);
                gps_latitude     = latitude_degree + latitude_minute / 60.0f;
                gps_latitude_chr = entry_list[2][0];
                // longitiude parser
                unsigned int longitude_degree;
                float        longitude_minute;
                sscanf(entry_list[3], "%2u%f", &longitude_degree, &longitude_minute);
                gps_longitude     = longitude_degree + longitude_minute / 60.0f;
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
                        "gps: pos: %2.9f%c,%2.9f%c\n", gps_latitude, gps_latitude_chr, gps_longitude, gps_longitude_chr);
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
            }
        }
message_line_next:
        message_line = strtok_r(NULL, "\r\n", &message_buffer_delim_saveptr);
        continue;
    }
}
