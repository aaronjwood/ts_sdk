/* Copyright(C) 2016 Verizon. All rights reserved. */

#include <stm32f4xx_hal.h>
#include <string.h>
#include "dbg.h"
#include "uart.h"
#include "at.h"

const char *host = "httpbin.org";
const char *port = "80";
/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow :
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 180000000
  *            HCLK(Hz)                       = 180000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 4
  *            APB2 Prescaler                 = 2
  *            HSE Frequency(Hz)              = 8000000
  *            PLL_M                          = 8
  *            PLL_N                          = 360
  *            PLL_P                          = 2
  *            PLL_Q                          = 8
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 5
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
	RCC_ClkInitTypeDef RCC_ClkInitStruct;
	RCC_OscInitTypeDef RCC_OscInitStruct;

	/* Enable Power Control clock */
	__HAL_RCC_PWR_CLK_ENABLE();

	/*
	 * The voltage scaling allows optimizing the power consumption when the
	 * device is clocked below the maximum system frequency, to update the
	 * voltage scaling value regarding system frequency refer to product
	 * datasheet.
	 */
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	/*
	 * Enable HSE Oscillator and activate PLL with HSE as source.
	 * Note : The following code uses a non-crystal / non-ceramic high speed
	 * external clock source (signified by RCC_HSE_BYPASS). It is derived
	 * from the MCO of the chip housing the STLink firmware. This MCO is in
	 * turn sourced from an 8MHz on-board crystal.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 8;
	RCC_OscInitStruct.PLL.PLLN = 360;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 8;		/* PLL48CLK = 45 MHz. */
	if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
		/* Initialization Error */
		raise_err();

	/* Extend maximum clock frequency to 180 MHz from 168 MHz. */
	if(HAL_PWREx_EnableOverDrive() != HAL_OK)
		/* Initialization Error */
		raise_err();

	/*
	 * Select PLL as system clock source and configure the HCLK, PCLK1 and
	 * PCLK2 clocks dividers.
	 */
	RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK |
			RCC_CLOCKTYPE_HCLK |
			RCC_CLOCKTYPE_PCLK1 |
			RCC_CLOCKTYPE_PCLK2);
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK; /* 180 MHz */
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;        /* 180 MHz */
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;         /* 45 MHz */
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;         /* 90 MHz */
	if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
		/* Initialization Error */
		raise_err();
}

/* Increments the SysTick value. */
void SysTick_Handler(void)
{
	HAL_IncTick();
}

/*
 * The following are interrupt handlers that trap illegal memory access and
 * illegal program behavior. These should be defined to prevent runaway code
 * while debugging.
 */
void HardFault_Handler(void)
{
	while (1)
		;
}

void MemManage_Handler(void)
{
	while (1)
		;
}

void BusFault_Handler(void)
{
	while (1)
		;
}

void UsageFault_Handler(void)
{
	while (1)
		;
}

int tcp_connect()
{
	int count = 0;
	int s_id = -1;

	while (s_id < 0 && count < 5) {
		s_id = at_tcp_connect(host, port);
		HAL_Delay(1000);
		count++;
	}
	if (s_id < 0) {
		dbg_printf("Invalid socket id: %d, looping forever\n", s_id);
		ASSERT(0);
	}
	dbg_printf("Socket id is :%d\n", s_id);
	return s_id;
}

void tcp_send(int s_id, uint8_t *send_buf, size_t len)
{
	int count = 0;
	int sent_data = 0;
	int res = 0;
	while (1) {
		res = at_tcp_send(s_id, (send_buf + sent_data), len - sent_data);
		if (res < 0) {
			if (count > 5)
				break;
			else {
				count++;
				res = 0;
			}
			HAL_Delay(2000);
		} else {
			sent_data += res;
			if (sent_data == len)
				break;
		}

	}
	if (sent_data <= 0) {
		dbg_printf("TCP send failed, looping forever\n");
		ASSERT(0);
	}
	dbg_printf("Sent data: %d\n", sent_data);

}

int main(int argc, char *argv[])
{
	HAL_Init();
	SystemClock_Config();

	dbg_module_init();
	dbg_printf("Begin:\n");
	/* Step 1: modem init */
	dbg_printf("Initializing modem...\n");
	if (!at_init()) {
		dbg_printf("Modem init failed, looping forever\n");
		ASSERT(0);
	}
	dbg_printf("Initializing modem done...\n");
	/* step 2: tcp connect */
	int s_id = tcp_connect();
	dbg_printf("TCP connect done...\n");

	/* step 3: send data */
	char *send_buf = "GET \\ HTTP\\1.1\r\n";
	dbg_printf("Sending data:\n");
	size_t len = strlen(send_buf);
	tcp_send(s_id, (uint8_t *)send_buf, len);

	/* step 4: wait for the data */
	int r_b;
	uint8_t count = 0;
	while (count < 5) {
		r_b = at_read_available(s_id);
		printf("Read available:%d\n", r_b);
		if (r_b > 0)
			break;
		HAL_Delay(1000);
		count++;
	}
	uint8_t read_buf[r_b + 1];
	read_buf[r_b] = 0x0;
	int bytes = at_tcp_recv(s_id, read_buf, r_b);
	printf("Read Bytes: %d\n", bytes);
	printf("%s\n", (char *)read_buf);

	/* step 5: tcp close */
	at_tcp_close(s_id);

	while (1)
		;
	return 0;
}
