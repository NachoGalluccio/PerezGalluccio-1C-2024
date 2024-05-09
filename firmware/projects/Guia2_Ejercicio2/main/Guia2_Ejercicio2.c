/*! @mainpage Guia2_Ejercicio2
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
#include "timer_mcu.h"
/*==================[macros and definitions]=================================*/
/** @def refresh
 *  @brief tiempo de actualizacion de tareas
*/
#define REFRESH_PERIOD 1000
/*==================[internal data definition]===============================*/
TaskHandle_t medicion_task_handle = NULL;
TaskHandle_t switches_task_handle = NULL;
/** @def isON
 * @brief bool para controlar encendido/apagado
*/
bool isON = false;
/** @def hold
 * @brief bool para controlar hold
*/
bool hold = 0;
/*==================[internal functions declaration]=========================*/

/**
 * @brief Función invocada en la interrupción del timer A
 */
void FuncTimerMedicion(void* param){
    xTaskNotifyGive(medicion_task_handle);    /* Envía una notificación a la tarea asociada a la medicion */
}

/**
 * @brief Función invocada en la interrupción del timer B
 */
void FuncTimerSwitches(void* param){
    xTaskNotifyGive(switches_task_handle);    /* Envía una notificación a la tarea asociada a los switches */
}
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
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);    /* La tarea espera en este punto hasta recibir una notificación */
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
	}
}

/** @fn FuncTecla_1
 * 	@brief Cambia bool isON al presionar tecla 1
 * 	@param [void]
 * 	@return
*/
static void FuncTecla_1(){
	isON = !isON;
}

/** @fn FuncTecla_2
 * 	@brief Cambia bool hold al presionar tecla 2
 * 	@param [void]
 * 	@return
*/
static void FuncTecla_2(){
	hold = !hold;
}

/*==================[external functions definition]==========================*/
void app_main(void)
{
	LedsInit();
	SwitchesInit();
	LcdItsE0803Init();
	HcSr04Init(GPIO_3, GPIO_2);

	SwitchActivInt(SWITCH_1,&FuncTecla_1,NULL);
	SwitchActivInt(SWITCH_2,&FuncTecla_2,NULL);

	/* Inicialización de timers */
    timer_config_t timer_medicion = {
        .timer = TIMER_A,
        .period = REFRESH_PERIOD*1000, /* De um a ms */
        .func_p = FuncTimerMedicion,
        .param_p = NULL
    };
    TimerInit(&timer_medicion);

	/* Task creation */
	xTaskCreate(&medicionTask, "medicion", 2048, NULL, 5, &medicion_task_handle);
	/*xTaskCreate(&ledControl, "ledControl", 512, NULL, 5, &led3_task_handle);*/

    /* Inicialización del conteo de timers */
    TimerStart(timer_medicion.timer);

}
/*==================[end of file]============================================*/