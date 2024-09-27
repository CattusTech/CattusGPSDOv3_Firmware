#pragma once
#include <stm32g4xx_hal.h>

extern double       gps_latitude;
extern char         gps_latitude_chr;
extern double       gps_longitude;
extern char         gps_longitude_chr;
extern unsigned int gps_sv_number;
extern float        gps_hdop;
extern int          gps_valid;
extern char         gps_mode;
extern unsigned int gps_time_h;
extern unsigned int gps_time_m;
extern float        gps_time_s;

#define UART_LINE_BUFFER_SIZE 256

extern char   uart_line_buffer[UART_LINE_BUFFER_SIZE];
extern size_t uart_line_ptr;

void gps_init();
void gps_deinit();
int gps_update();
