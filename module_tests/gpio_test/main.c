/* Copyright(C) 2017 Verizon. All rights reserved. */

/*
 * This test program is an illustration to use the gpio hal apis
 * for enabling the pins to write mode. This test uses
 * hal functions for initialisation and write apis
 * to enable write over pins.
 */
#include "sys.h"
#include "gpio_hal.h"
#include "dbg.h"
#include "uart_hal.h"

int main()
{
	pin_name_t pin_name = PC13;
	gpio_config_t my_led;
	sys_init();
	dbg_module_init();
	dbg_printf("\r Welcome to Beduin_v2:\r\n");

        dbg_printf("Init LEDS\n\r");
	my_led.dir = OUTPUT;
	my_led.pull_mode = PP_PULL_UP;
	my_led.speed = SPEED_LOW;
	gpio_init(pin_name, &my_led);

        dbg_printf("set leds in default state\n\r");
        /* set onboard LEDS */
	while (1) {
        	dbg_printf("set leds in OFF state\n\r");
		gpio_write(pin_name, PIN_LOW);
		sys_delay(1500);
        	dbg_printf("set leds in ON state\n\r");
		gpio_write(pin_name, PIN_HIGH);
		sys_delay(1500);
	}
	return 0;
}
