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
#include "uart_mcu.h"
/*==================[macros and definitions]=================================*/
/** @def REFRESH_PERIOD_US
 * @brief tiempo de actualizacion en us
*/
#define REFRESH_PERIOD_US 1000000

/** @def BAUD_RATE
 * @brief baud rate
*/
#define BAUD_RATE 115200

/** @def DECIMAL
 * @brief Constante 10
*/
#define DECIMAL 10

/** @def CONSTANTE_INCHES_CM
 * @brief Constante de conversion de pulgadas-cm
*/
#define CONSTANTE_INCHES_CM 2.54
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
/** @def medicion_cm
 * @brief True -> medicion en cm // False -> medicion en inches
*/
bool medicion_cm = true;
/** @def max_distance
 * @brief Distancia maxima medida
*/
uint16_t max_distance;	

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
 * 	@param medicion Medicion en cm
 * 	@return
*/
void encenderLEDS(uint16_t medicion)
{
	if(!medicion_cm){
		medicion = medicion * CONSTANTE_INCHES_CM;
	}
	if (medicion >= 10)
	{
		LedOn(LED_1);
		LedOff(LED_2);
		LedOff(LED_3);
		if (medicion >= 20)
		{
			LedOn(LED_2);
			LedOff(LED_3);
			if (medicion >= 30)
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
		if(medicion_cm){
			medicion = HcSr04ReadDistanceInCentimeters();
		} else {
			medicion = HcSr04ReadDistanceInInches();
		}
		if (isON)
		{
			encenderLEDS(medicion);
			if (!hold)
			{
				LcdItsE0803Write(medicion);
				UartSendString(UART_PC, (char*)UartItoa(medicion,DECIMAL));
				if(medicion_cm){
					UartSendString(UART_PC, " cm\n");
				} else {
					UartSendString(UART_PC, " inches\n");
				}
				
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

/** @fn dataReceived
 * 	@brief Tarea que se ejecuta al recibir un dato por puerto serie
 * 	@param [void]
 * 	@return
*/
static void dataReceived(){
	uint8_t read_byte;
	UartReadByte(UART_PC, &read_byte);
	switch(read_byte){
    		case 'O':	/* Toggle On/Off*/
    			FuncTecla_1();
    		break;
    		case 'H':	/* Toggle Hold */
    			FuncTecla_2();
    		break;
			case 'I':	/* Cambia cm <-> inch*/
				medicion_cm = !medicion_cm;
			break;
			case 'M':	/* Show MAX */
				LcdItsE0803Write(max_distance);
				FuncTecla_2();
			break;
		}
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
        .period = REFRESH_PERIOD_US,
        .func_p = FuncTimerMedicion,
        .param_p = NULL,
    };
    TimerInit(&timer_medicion);

	/* Inicializacion de UART */
	serial_config_t serial_config = {
		.port = UART_PC,
		.baud_rate = BAUD_RATE,
		.func_p = &dataReceived,
		.param_p = NULL,
	};
	UartInit(&serial_config);

	/* Task creation */
	xTaskCreate(&medicionTask, "medicion", 2048, NULL, 5, &medicion_task_handle);
	/*xTaskCreate(&ledControl, "ledControl", 512, NULL, 5, &led3_task_handle);*/

    /* Inicialización del conteo de timers */
    TimerStart(timer_medicion.timer);

}
/*==================[end of file]============================================*/