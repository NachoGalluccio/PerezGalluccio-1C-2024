/*! @mainpage Examen
 *
 * @section genDesc General Description
 *
 * Dispositivo basado en la ESP-EDU que permite controlar el riego y el pH de una plantera.
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	sensor Agua	| 	GPIO_0		|
 * | 	sensor pH	| 	GPIO_1		|
 * | 	Bomba Agua	| 	GPIO_20		|
 * | 	Bomba Base	| 	GPIO_21		|
 * | 	Bomba Acido	| 	GPIO_22		|
 *
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 09/05/2024 | Document creation		                         |
 *
 * @author Nacho Perez :)
 *
 */


/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
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

/*==================[internal data definition]===============================*/
TaskHandle_t medicion_task_handle = NULL;
TaskHandle_t serialtransmission_task_handle = NULL;
TaskHandle_t bombas_task_handle = NULL;

/** @def gpioConf_t
 *  @brief struct que define un pin GPIO y su direccion, entrada o salida.
*/
typedef struct
{
	gpio_t pin;		/*!< GPIO  pin number*/
	io_t dir;		/*!< GPIO direction '0' IN; '1' OUT*/	
} gpioConf_t;

/** @def Bomba_Agua
 *  @brief Struct con los datos de la bomba de agua
*/
gpioConf_t Bomba_Agua = {GPIO_20,GPIO_OUTPUT};

/** @def Bomba_Base
 *  @brief Struct con los datos de la bomba de base pHB
*/
gpioConf_t Bomba_Base = {GPIO_21,GPIO_OUTPUT};

/** @def Bomba_Acido
 *  @brief Struct con los datos de la bomba de acido pHA
*/
gpioConf_t Bomba_Acido = {GPIO_22,GPIO_OUTPUT};

/** @def Bomba_pH
 *  @brief Struct con los datos del sensor de pH
*/
gpioConf_t Sensor_pH = {GPIO_1,GPIO_INPUT};

/** @def Sensor_Humedad
 *  @brief Struct con los datos del sensor de humedad
*/
gpioConf_t Sensor_Humedad = {GPIO_0,GPIO_INPUT};

/** @def Encendido
 * @brief booleano para controlar encedido y apagado
*/
bool Encendido = false;

/** @def Humedad
 * @brief booleano para controlar estado de la humedad
*/
bool Humedad = false;

/** @def pH
 * @brief float para guardar valor de pH
*/
float pH = 0;

/** @def FREC_MUESTREO_US
 * @brief frecuencia de lectura de datos en us
*/
uint16_t FREC_MUESTREO_US = 3000000; //3 segundos

/** @def FREC_TRANSMISSION_US
 * @brief frecuencia de transmision de datos en us
*/
uint16_t FREC_TRANSMISSION_US = 5000000; //5 segundos

// Punteros a Timers
timer_config_t* timerA;
timer_config_t* timerB;

/*==================[internal functions declaration]=========================*/

/**
 * @brief Función invocada en la interrupción del timer A
 */
void medicion_timer(void *param)
{
	xTaskNotifyGive(medicion_task_handle); /* Envía una notificación a la tarea asociada */
}

/**
 * @brief Función invocada en la interrupción del timer B
 */
void serialtransmission_timer(void *param)
{
	xTaskNotifyGive(serialtransmission_task_handle); /* Envía una notificación a la tarea asociada a los switches */
}

/** @fn medicion_task
 * 	@brief Tarea que controla el evento de medir agua y ph
*/
void medicion_task(void *pvParameter)
{
	uint16_t read_value;
	while (1)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); /* Espera aca hasta nueva notificacion */
		//Medimos humedad
		Humedad = GPIORead(Sensor_Humedad.dir);

		// Medimos pH
		// El sensor entrega de 0 a 3000 mV para un rango de 0 a 14 pH
		// Entonces la conversion sera tal (lectura[mV])*14/3000 = pH
		AnalogInputReadSingle(CH1,&read_value);

		pH = read_value*14/3000;
		xTaskNotifyGive(bombas_task_handle); /* Envia una notificacion a la tarea asociada*/
	}
}

/** @fn bombas_task
 * 	@brief Tarea que controla las bombas
*/
void bombas_task(void *pvParameter)
{
	while(1){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // Espera aca hasta nueva notificacion
		// Control de pH para encender/apagar bombas base/acido
		if(pH > 6.7){
			GPIOOn(Bomba_Acido.dir);
		} else if (pH < 6.0){
			GPIOOn(Bomba_Base.dir);
		} else {
			GPIOOff(Bomba_Acido.dir);
			GPIOOff(Bomba_Base.dir);
		}

		// Control de humedad para encender/apagar bomba de agua
		if(Humedad){
			GPIOOff(Bomba_Agua.dir);
		} else {
			GPIOOn(Bomba_Agua.dir);
		}
	}
}

/** @fn serialtransmission_task
 * 	@brief Tarea que controla el evento de transmitir datos por puerto serie
*/
void serialtransmission_task(void *pvParameter)
{
	//Cadena a enviar
	char msg[32];
	while (1)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		if(Humedad){//Pregunto por humedad y doy formato al texto dependiendo el caso
			sprintf(msg, "pH: %.1d, humedad correcta\n", pH);
		} else {
			sprintf(msg, "pH: %.1d, humedad incorrecta\n", pH);
		}

		//Envio el mensaje
		UartSendString(UART_PC, msg);
		
		//Pregunto por el estado de la Bomba de agua
		if(GPIORead(Bomba_Agua.dir)){
			sprintf(msg, "Bomba de agua encendida");
		}

		//Envio el mensaje
		UartSendString(UART_PC, msg);
		
		//Pregunto por el estado de la bomba base
		if(GPIORead(Bomba_Acido.dir)){
			sprintf(msg, "Bomba pHA encendida");
		}

		//Envio el mensaje
		UartSendString(UART_PC, msg);
		
		//Pregunto por el estado de la bomba de acido
		if(GPIORead(Bomba_Base.dir)){
			sprintf(msg, "Bomba pHB encendida");
		}

		//Envio el mensaje
		UartSendString(UART_PC, msg);
	}
}

/** @fn Encender
 * 	@brief Activa la funcion de la tecla 1
*/
void Encender()
{
	if(!Encendido){
		TimerStart(timerA->timer);
		TimerStart(timerB->timer);
	}
}

/** @fn Apagar
 * 	@brief Activa la funcion de la tecla 2
*/
void Apagar()
{
	if(Encendido){
		TimerStop(timerA->timer);
		TimerStop(timerB->timer);
		GPIOOff(Bomba_Agua.dir);
		GPIOOff(Bomba_Base.dir);
		GPIOOff(Bomba_Acido.dir);
	}
}

/** @fn dataReceived
 * 	@brief UART
*/
void dataReceived()
{
	uint8_t read_byte;
}
/*==================[external functions definition]==========================*/
void app_main(void)
{
	//Init
	SwitchesInit();

	SwitchActivInt(SWITCH_1, &Encender, NULL);
	SwitchActivInt(SWITCH_2, &Apagar, NULL);

	/* Inicialización de timers */
	timer_config_t timer_medicion = {
		.timer = TIMER_A,
		.period = FREC_MUESTREO_US,
		.func_p = &medicion_timer,
		.param_p = NULL,
	};
	TimerInit(&timer_medicion);
	timer_config_t timer_transmission = {
		.timer = TIMER_B,
		.period = FREC_TRANSMISSION_US,
		.func_p = &serialtransmission_timer,
		.param_p = NULL,
	};
	TimerInit(&timer_transmission);

	//Guardo puntero al timer
	timerA = &timer_medicion;
	timerB = &timer_transmission;

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
	xTaskCreate(&medicion_task, "medicion", 1024, NULL, 5, &medicion_task_handle);
	xTaskCreate(&serialtransmission_task, "transmission", 1024, NULL, 5, &serialtransmission_task_handle);
	xTaskCreate(&bombas_task, "bombas", 1024, NULL, 5, &bombas_task_handle);

	/* Inicialización del conteo de timers */
	TimerStart(timer_medicion.timer);
	TimerStart(timer_transmission.timer);
}
/*==================[end of file]============================================*/