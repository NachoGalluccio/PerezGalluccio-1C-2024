/*! @mainpage Template
 *
 * @section genDesc General Description
 *
 * This section describes how the program works.
 *
 * <a href="https://drive.google.com/...">Operation Example</a>
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	PIN_X	 	| 	GPIO_X		|
 *
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 12/09/2023 | Document creation		                         |
 *
 * @author Albano Peñalva (albano.penalva@uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "switch.h"
#include "driver/gpio.h"
#include "driver/gpio_filter.h"
#include "gpio_mcu.h"
#include "lcditse0803.h"
#include "hc_sr04.h"
/*==================[macros and definitions]=================================*/
#define refresh 1000
#define switch_refresh 100
/*==================[internal data definition]===============================*/
bool isON = false;
uint8_t hold = 0;
/*==================[internal functions declaration]=========================*/
void encenderLEDS(uint16_t centimetros)
{
	if (centimetros >= 10)
	{
		LedOn(LED_1);
		if (centimetros >= 20)
		{
			LedOn(LED_2);
			if (centimetros >= 30)
			{
				LedOn(LED_3);
			}
		}
	}
	else
	{
		LedsOffAll();
	}
}

static void medicionTask(void *pvParameter)
{
	uint16_t medicion;
	while (1)
	{
		printf("hola");
		medicion = 15;
		printf("%d",medicion);
		if (isON)
		{
			encenderLEDS(medicion);
			if (!hold)
			{
				LcdItsE0803Write(medicion);
			}
		} else {
			LedsOffAll();
			LcdItsE0803Off();
		}
		vTaskDelay(refresh / portTICK_PERIOD_MS);
	}
}

static void switchControl(void *pvParameter)
{
	uint8_t teclas;
	while (1)
	{
		teclas = SwitchesRead();
		switch (teclas)
		{
		case SWITCH_1:
			isON = !isON;
			break;
		case SWITCH_2:
			hold = !hold;
			break;
			/*case SWITCH_1+SWITCH_2:
				LedToggle(LED_3);
			break;*/
		}
		vTaskDelay(switch_refresh / portTICK_PERIOD_MS);
	}
}

/*==================[external functions definition]==========================*/
void app_main(void)
{
	LedsInit();
	SwitchesInit();
	LcdItsE0803Init();
	HcSr04Init(GPIO_2, GPIO_3);
	LcdItsE0803Write(15);

	xTaskCreate(&medicionTask, "medicion", 2048, NULL, 5, NULL);
	xTaskCreate(&switchControl, "switchControl", 512, NULL, 5, NULL);
	/*xTaskCreate(&ledControl, "ledControl", 512, NULL, 5, &led3_task_handle);*/
}
/*==================[end of file]============================================*/