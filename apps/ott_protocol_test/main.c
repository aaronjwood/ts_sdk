/* Copyright(C) 2016 Verizon. All rights reserved. */

#include <stm32f4xx_hal.h>
#include "dbg.h"
#include "ott_protocol.h"

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

#if 0
#define SERVER_PORT "4433"
#define SERVER_NAME "localhost"
#else
/* Temporary - a site that happens to speak our single crypto suite. */
#define SERVER_PORT "443"
#define SERVER_NAME "www.atper.org"
#endif

static msg_t msg;
#define RECV_TIMEOUT_MS		2000
void test_scenario_1(void)
{
	/* Scenario 1: Device has status to report, Cloud has nothing pending */

	/* 1 - Initiate the TCP/TLS session */
	dbg_printf("Initiating a secure connection to the cloud:\n");
	if (ott_initiate_connection(SERVER_NAME, SERVER_PORT) != OTT_OK) {
		dbg_printf("\tCould not connect to the cloud.\n");
		return;
	}

	/* 2 & 3 - Send the version byte and authenticate the device with the cloud. */
	uint8_t d_ID[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
	uint8_t d_sec[] = {0, 1, 2, 3, 4, 5, 6};
	dbg_printf("Sending version byte and authentication message:\n");
	ASSERT(ott_send_auth_to_cloud(CF_PENDING, d_ID, sizeof(d_sec), d_sec)
			== OTT_OK);

	/* 4 - Wait for a timeout before checking for a message from the cloud. */
	HAL_Delay(RECV_TIMEOUT_MS);
	if (ott_retrieve_msg(&msg) != OTT_OK) {
		dbg_printf("\tERR: No response from cloud, closing connection.\n");
		ASSERT(ott_send_ctrl_msg(CF_NACK | CF_QUIT) == OTT_OK);
		ASSERT(ott_close_connection() == OTT_OK);
		return;
	}
	/* Make sure the previous message was ACKed. */
	if (!OTT_FLAG_IS_SET(msg.c_flags, CF_ACK)) {
		dbg_printf("\tCloud did not ACK previous message.\n");
		ASSERT(ott_close_connection() == OTT_OK);
		return;
	}
	dbg_printf("\tAuthenticated with the cloud.\n");

	/* 5 - Send the status of the sensors attached to the device. */
	uint8_t status[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
	dbg_printf("Sending device status to the cloud:\n");
	ASSERT(ott_send_status_to_cloud(CF_NONE, sizeof(status), status) == OTT_OK);

	/* 6 - Wait for a timeout before checking for a message from the cloud. */
	HAL_Delay(RECV_TIMEOUT_MS);
	if (ott_retrieve_msg(&msg) != OTT_OK) {
		dbg_printf("\tERR: No response from cloud, closing connection.\n");
		ASSERT(ott_send_ctrl_msg(CF_NACK | CF_QUIT) == OTT_OK);
		ASSERT(ott_close_connection() == OTT_OK);
		return;
	}
	/* Make sure the previous message was ACKed. */
	if (!OTT_FLAG_IS_SET(msg.c_flags, CF_ACK)) {
		dbg_printf("\tCloud did not ACK previous message.\n");
		ASSERT(ott_close_connection() == OTT_OK);
		return;
	}
	dbg_printf("\tDevice status delivered to the cloud.\n");

	/* 7 - In this scenario, the cloud sets the QUIT flag. */
	ASSERT(OTT_FLAG_IS_SET(msg.c_flags, CF_QUIT));
	dbg_printf("Closing secure connection.\n");
	ASSERT(ott_close_connection() == OTT_OK);
}

void test_scenario_2(void)
{
	/* Code */
}

void test_scenario_3(void)
{
	/* Code */
}

#define DELAY_MS	20000
int main(int argc, char *argv[])
{
	HAL_Init();
	SystemClock_Config();

	dbg_module_init();
	ASSERT(ott_protocol_init() == OTT_OK);

	dbg_printf("Begin:\n");
	while (1) {
		dbg_printf("Test scenario 1\n");
		test_scenario_1();
		HAL_Delay(DELAY_MS);
		dbg_printf("Test scenario 2\n");
		test_scenario_2();
		HAL_Delay(DELAY_MS);
		dbg_printf("Test scenario 3\n");
		test_scenario_3();
		HAL_Delay(DELAY_MS);
	}
	return 0;
}
