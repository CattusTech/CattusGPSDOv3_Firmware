#pragma once
#include <stm32g4xx_hal.h>
void ocxo_init();
void ocxo_update();

extern int ocxo_valid;
extern int ocxo_overheat;
