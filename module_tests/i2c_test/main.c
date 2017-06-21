/* Copyright(C) 2017 Verizon. All rights reserved. */

#include "sys.h"
#include "dbg.h"
#include "i2c_hal.h"
#include "module_config.h"

/* Exit On Error (EOE) macro */
#define EOE(func) \
	do { \
		if (!(func)) \
			return false; \
	} while (0)

struct array_t {
	uint8_t sz;      /* Number of bytes contained in the data buffer */
	uint8_t *bytes;  /* Pointer to the data buffer */
};

int main()
{
	sys_init();
	dbg_module_init();
	struct array_t data;
	uint8_t value = 0x27;
	i2c_addr_t i2c_dest_addr;
	uint8_t a[CALIB_SZ] = {0};
	data.bytes = a;
	dbg_printf("Begin:\n");

	periph_t i2c_handle =  i2c_init(I2C_SCL, I2C_SDA);
	ASSERT(!(i2c_handle == NO_PERIPH));
	i2c_dest_addr.slave = SLAVE_ADDR;
	i2c_dest_addr.reg = SLAVE_ID;
	EOE(i2c_read(i2c_handle, i2c_dest_addr, 1, data.bytes));
	if (*(data.bytes) == SLAVE_IDENTIFIER)
		dbg_printf("Sensor identified\n");
	else
		dbg_printf("Wrong Sensor !!!\n");

	i2c_dest_addr.reg = CTRL_REG ;
	printf("writing value  of 0x%2x in to the register\n", value);
	EOE(i2c_write(i2c_handle, i2c_dest_addr, 1 , &value));

	printf("Reading value from the register\n");
	EOE(i2c_read(i2c_handle, i2c_dest_addr, 1, data.bytes));
	dbg_printf("value is 0x%2x\n", *(data.bytes));
	return 0;
}
