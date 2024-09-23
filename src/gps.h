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

void gps_init();
void gps_deinit();
void gps_update();
