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
#include "timer_mcu.h"
#include "uart_mcu.h"
#include "analog_io_mcu.h"
/*==================[macros and definitions]=================================*/
/** @def BAUD_RATE
 * @brief baud rate
*/
#define BAUD_RATE 115200

/** @def DECIMAL
 * @brief Constante 10
*/
#define DECIMAL 10

/** @def BUFFER_SIZE
 * @brief Tamaño del vector de ecg
*/
#define BUFFER_SIZE 231
/*==================[internal data definition]===============================*/
TaskHandle_t ADconversion_task_handle = NULL;
TaskHandle_t serialtransmission_task_handle = NULL;

/** @def read_value
 * @brief valor leido x puerto serie
*/
uint16_t read_value;

/** @def FREC_MUESTREO_US
 * @brief frecuencia de lectura de datos en us
*/
uint16_t FREC_MUESTREO_US = 2000;

/** @def FREC_TRANSMISSION-US
 * @brief frecuencia de transmision de datos en us
*/
uint16_t FREC_TRANSMISSION_US = 4000;

const char ecg[BUFFER_SIZE] = {
    76, 77, 78, 77, 79, 86, 81, 76, 84, 93, 85, 80,
    89, 95, 89, 85, 93, 98, 94, 88, 98, 105, 96, 91,
    99, 105, 101, 96, 102, 106, 101, 96, 100, 107, 101,
    94, 100, 104, 100, 91, 99, 103, 98, 91, 96, 105, 95,
    88, 95, 100, 94, 85, 93, 99, 92, 84, 91, 96, 87, 80,
    83, 92, 86, 78, 84, 89, 79, 73, 81, 83, 78, 70, 80, 82,
    79, 69, 80, 82, 81, 70, 75, 81, 77, 74, 79, 83, 82, 72,
    80, 87, 79, 76, 85, 95, 87, 81, 88, 93, 88, 84, 87, 94,
    86, 82, 85, 94, 85, 82, 85, 95, 86, 83, 92, 99, 91, 88,
    94, 98, 95, 90, 97, 105, 104, 94, 98, 114, 117, 124, 144,
    180, 210, 236, 253, 227, 171, 99, 49, 34, 29, 43, 69, 89,
    89, 90, 98, 107, 104, 98, 104, 110, 102, 98, 103, 111, 101,
    94, 103, 108, 102, 95, 97, 106, 100, 92, 101, 103, 100, 94, 98,
    103, 96, 90, 98, 103, 97, 90, 99, 104, 95, 90, 99, 104, 100, 93,
    100, 106, 101, 93, 101, 105, 103, 96, 105, 112, 105, 99, 103, 108,
    99, 96, 102, 106, 99, 90, 92, 100, 87, 80, 82, 88, 77, 69, 75, 79,
    74, 67, 71, 78, 72, 67, 73, 81, 77, 71, 75, 84, 79, 77, 77, 76, 76,
};
/*==================[internal functions declaration]=========================*/

/**
 * @brief Función invocada en la interrupción del timer A
 */
void ADconversion_timer(void *param)
{
	xTaskNotifyGive(ADconversion_task_handle); /* Envía una notificación a la tarea asociada */
}

/**
 * @brief Función invocada en la interrupción del timer B
 */
void serialtransmission_timer(void *param)
{
	xTaskNotifyGive(serialtransmission_task_handle); /* Envía una notificación a la tarea asociada a los switches */
}

/** @fn ADconversion_task
 * 	@brief Tarea que controla el evento convertir de analogico a digital
 * 	@param [void]
 * 	@return
*/
void ADconversion_task(void *pvParameter)
{
	while (1)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); /* Espera aca hasta nueva notificacion */
		AnalogInputReadSingle(CH1, &read_value);
		UartSendString(UART_PC, (char*)UartItoa(read_value,DECIMAL));
		UartSendByte(UART_PC, "\r");
	}
}

/** @fn serialtransmission_task
 * 	@brief Tarea que controla el evento de transmitir datos por puerto serie
 * 	@param [void]
 * 	@return
*/
void serialtransmission_task(void *pvParameter)
{
	uint8_t i = 0;
	while (1)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		AnalogOutputWrite(ecg[i]);
		i++;
		if (i => BUFFER_SIZE)
			i = 0;
	}
}
/** @fn FuncTecla_1
 * 	@brief Activa la funcion de la tecla 1
 * 	@param [void]
 * 	@return
*/
void FuncTecla_1()
{
	FREC_TRANSMISSION_US = FREC_TRANSMISSION_US + 100;
}

/** @fn FuncTecla_2
 * 	@brief Activa la funcion de la tecla 2
 * 	@param [void]
 * 	@return
*/
void FuncTecla_2()
{
	FREC_TRANSMISSION_US = FREC_TRANSMISSION_US - 100;
}

/** @fn dataReceived
 * 	@brief Tarea que se ejecuta al recibir un dato por puerto serie
 * 	@param [void]
 * 	@return
*/
void dataReceived()
{
	uint8_t read_byte;
	UartReadByte(UART_PC, &read_byte);
	switch (read_byte)
	{
	case 'T':
		FuncTecla_1();
		break;
	case 'B':
		FuncTecla_2();
		break;
	case 'R':
		FREC_TRANSMISSION_US = 4000;
		break;
	}
}

/*==================[external functions definition]==========================*/
void app_main(void)
{
	SwitchesInit();

	SwitchActivInt(SWITCH_1, &FuncTecla_1, NULL);
	SwitchActivInt(SWITCH_2, &FuncTecla_2, NULL);

	/* Inicialización de timers */
	timer_config_t timer_conversion = {
		.timer = TIMER_A,
		.period = FREC_MUESTREO_US,
		.func_p = &ADconversion_timer,
		.param_p = NULL,
	};
	TimerInit(&timer_conversion);
	timer_config_t timer_transmission = {
		.timer = TIMER_B,
		.period = FREC_TRANSMISSION_US,
		.func_p = &serialtransmission_timer,
		.param_p = NULL,
	};
	TimerInit(&timer_transmission);

	/* Inicializacion de UART */
	serial_config_t serial_config = {
		.port = UART_PC,
		.baud_rate = BAUD_RATE,
		.func_p = &dataReceived,
		.param_p = NULL,
	};
	UartInit(&serial_config);

	/* Inicializacion analog_io */
	analog_input_config_t analog_congif = {
		.input = CH1,
		.mode = ADC_SINGLE,
		.param_p = NULL,
		.func_p = NULL,
		.sample_frec = 0,
	};
	AnalogInputInit(&analog_congif);
	AnalogOutputInit();

	/* Task creation */
	xTaskCreate(&ADconversion_task, "conversion", 1024, NULL, 5, &ADconversion_task_handle);
	xTaskCreate(&serialtransmission_task, "transmission", 1024, NULL, 5, &serialtransmission_task_handle);

	/* Inicialización del conteo de timers */
	TimerStart(timer_conversion.timer);
	TimerStart(timer_transmission.timer);
}
/*==================[end of file]============================================*/