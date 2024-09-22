#include <stm32g4xx_hal.h>
#define hal_perror(m, x, r) printf(m ": failed to " x " error: %u", (unsigned int)r)
