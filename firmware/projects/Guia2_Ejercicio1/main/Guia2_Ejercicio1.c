/*! @mainpage Guia2_Ejercicio1
 *
 * @section genDesc General Description
 *
 * Se mide distancia con sensor ultrasonico y se reporta en un display, utilizando los LEDs para marcar medidas.
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	HC_SR04	 	| 	GPIO_X		|
 * | 	LCD0803	 	| 	GPIO_X		|
 *
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 09/05/2024 | Document creation		                         |
 *
 * @author Nacho Perez
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
/** @def refresh
 *  @brief tiempo de actualizacion de tareas
*/
#define refresh 1000
/** @def switch_refresh
 * @brief tiempo de actualizacion de tarea de control de switch
*/
#define switch_refresh 100
/*==================[internal data definition]===============================*/
/** @def isON
 * @brief bool para controlar encendido/apagado
*/
bool isON = false;
/** @def hold
 * @brief bool para controlar hold
*/
bool hold = 0;
/*==================[internal functions declaration]=========================*/
/** @fn encenderLEDS
 * 	@brief Enciende los LEDs correspondiente con la medicion
 * 	@param centimetros Medicion en cm
 * 	@return
*/
void encenderLEDS(uint16_t centimetros)
{
	if (centimetros >= 10)
	{
		LedOn(LED_1);
		LedOff(LED_2);
		LedOff(LED_3);
		if (centimetros >= 20)
		{
			LedOn(LED_2);
			LedOff(LED_3);
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

/** @fn medicionTask
 * 	@brief Tarea que controla el evento de medir con el sensor ultrasonico
 * 	@param [void]
 * 	@return
*/
static void medicionTask(void *pvParameter)
{
	uint16_t medicion;
	while (1)
	{
		medicion = HcSr04ReadDistanceInCentimeters();
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

/** @fn switchControl
 * 	@brief Tarea que controla el evento de los switches
 * 	@param [void]
 * 	@return
*/
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
	HcSr04Init(GPIO_3, GPIO_2);

	xTaskCreate(&medicionTask, "medicion", 2048, NULL, 5, NULL);
	xTaskCreate(&switchControl, "switchControl", 512, NULL, 5, NULL);
	/*xTaskCreate(&ledControl, "ledControl", 512, NULL, 5, &led3_task_handle);*/

	/* Tiempo de pulso a 5cm: 514 us*/
}
/*==================[end of file]============================================*/