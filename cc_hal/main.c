#include <stdint.h>

#include "gpio_hal.h"
#include <stm32f4xx_hal.h>

int main(int argc, char *argv[])
{
	HAL_Init();	/* Done through platform_init() */
	return 0;
}
