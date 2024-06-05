/*! @mainpage Proyecto Final
 *
 * \section genDesc General Description
 *
 *  Podometro con acelerometro ADXL335
 * 
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 22/05/2024 | Document creation		                         |
 *
 * @author Nacho Perez
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "timer_mcu.H"
#include "ADXL335.h"
/*==================[macros and definitions]=================================*/
/** @def BAUD_RATE
 * @brief baud rate
*/
#define BAUD_RATE 115200

/** @def DECIMAL
 * @brief Constante 10
*/
#define DECIMAL 10

/** @def CANTIDAD_MAXIMOS
 * @brief Cuantos maximos(picos) se quieren tomar en cuenta para el calculo de UMBRAL
*/
#define CANTIDAD_MAXIMOS 10

/** @def LECTURAS_CALIBRACION
 * @brief Cuantos valores se quieren tomar en cuenta
*/
#define LECTURAS_CALIBRACION 100

/** @def FREC_LECTURA_US
 * @brief frecuencia de lectura de datos en us
*/
#define FREC_LECTURA_US 25000

/*==================[internal data definition]===============================*/
TaskHandle_t readacc_task_handle = NULL;
TaskHandle_t aaa_task_handle = NULL;


/** @def cantidad_pasos
 * @brief Define la cantidad de pasos que se dieron
*/
int cantidad_pasos = 0;

/** @def umbral
 * @brief Define el valor del umbral para medir los pasos
*/
float umbral = 6;

/** @def x_promedio
 * @brief Define el promedio de los valores leidos en X en el acelerometro en etapa de calibracion
*/
float x_promedio;

/** @def y_promedio
 * @brief Define el promedio de los valores leidos en Y en el acelerometro en etapa de calibracion
*/
float y_promedio;

/** @def z_promedio
 * @brief Define el promedio de los valores leidos en Z en el acelerometro en etapa de calibracion
*/
float z_promedio;

/** @def CONTAR_PASOS
 * @brief Bandera para determinar si contar o no un paso
*/
bool CONTAR_PASOS = true;

/*==================[internal functions declaration]=========================*/

/** @fn readacc_timer
 *  @brief Función invocada en la interrupción del timer A
 * 	@param[in] No hay parametros
 * 	@return
 */
void readacc_timer(void *param)
{
	xTaskNotifyGive(readacc_task_handle); /* Envía una notificación a la tarea asociada */
}

/** @fn control_prom_timer
 *  @brief Función invocada en la interrupción del timer B
 * 	@param[in] No hay parametros
 * 	@return
 */
void control_prom_timer(void *param)
{
	xTaskNotifyGive(control_prom_task_handle); /* Envía una notificación a la tarea asociada */
}

/** @fn readacc_task
 * 	@brief Tarea que controla el evento de leer el acelerometro
 * 	@param[in] No hay parametros
 * 	@return
*/
void readacc_task(void *pvParameter)
{
    float umbral;
    float maximos[CANTIDAD_MAXIMOS] = {0};
    float vector_acc;
	while (1)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); /* Espera aca hasta nueva notificacion */
        /* Codigo para encontrar maximos y hacer umbral adaptativo */

        /* Codigo para contar pasos */
        vector_acc = sqrt((ReadXValue-x_promedio)^2 + (ReadYValue-y_promedio)^2 + (ReadZValue-z_promedio)^2);

        if(vector_acc > umbral && CONTAR_PASOS){
            CONTAR_PASOS = !CONTAR_PASOS; //Levanta bandera para no seguir contando pasos
            CANTIDAD_PASOS++; //Suma un paso al detectar que el vector_acc supero el umbral y la bandera esta baja
        } else if (vector_acc < umbral && !CONTAR_PASOS)
        {
            CONTAR_PASOS = !CONTAR_PASOS; //Baja bandera
        }        
	}
}

/** @fn aaa
 * 	@brief Tarea que actualiza promedios
 * 	@param[in] No hay parametros
 * 	@return
*/
void aaa(void *pvParameter)
{
	while (1)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); /* Espera aca hasta nueva notificacion */
	}
}

/** @fn Calibrate_Acc
 * 	@brief Calibra el acelerometro calculando los valores promedios del estado inicial
 *  @param[in] No hay parámetros
 * 	@return 1 (TRUE) si no hay error
*/
Calibrate_Acc(){
    /* Loop para leer los valores iniciales */
    x_promedio = 0;
    y_promedio = 0;
    z_promedio = 0;
    for (uint16_t i = 0; i < LECTURAS_CALIBRACION; i++)
    {
        /* Sumatoria de los primeros *LECTURAS_CALIBRACION* valores */
        x_promedio = x_promedio + ReadXValue();
        y_promedio = y_promedio + ReadYValue();
        z_promedio = z_promedio + ReadZValue();
    }
    
    /* Calculo de los promedios */
    x_promedio = x_promedio/LECTURAS_CALIBRACION;
    y_promedio = y_promedio/LECTURAS_CALIBRACION;
    z_promedio = z_promedio/LECTURAS_CALIBRACION;

    return 1;
}

/** @fn InitPdmtr
 * 	@brief Inicializa accesorios y variables iniciales para poder utilizar el podometro
 *  @param[in] No hay parámetros
 * 	@return 1 (TRUE) si no hay error
*/
bool InitPdmtr(){
    /* Inicializacion de acelerometro */
    ADXL335Init();

    if(Calibrate_Acc()){
        //Calibracion finalizada
    }

    /* Inicializacion bt */
    /*le_config_t ble_configuration = {
        "ESP_EDU_1",
        read_data
    };
    
    BleInit(&ble_configuration);*/

    return 1;    
}

/** @fn InitTasks
 * 	@brief Inicializa tareas
 *  @param[in] No hay parámetros
 * 	@return 1 (TRUE) si no hay error
*/
bool InitTasks(){
    /* Task creation */
	xTaskCreate(&readacc_task, "Reading", 1024, NULL, 5, &readacc_task_handle);
	//xTaskCreate(&control_prom_task, "Calculo_Prom", 1024, NULL, 5, &control_prom_task_handle);
    return 1;
}

/** @fn InitTimers
 * 	@brief Inicializa timers
 *  @param[in] No hay parámetros
 * 	@return 1 (TRUE) si no hay error
*/
bool InitTimers(){
    /* Inicialización de timers */
	timer_config_t timer_readacc= {
		.timer = TIMER_A,
		.period = FREC_LECTURA_US,
		.func_p = &readacc_timer,
		.param_p = NULL,
	};
	TimerInit(&timer_readacc);
	/*timer_config_t timer_promedio = {
		.timer = TIMER_B,
		.period = FREC_CONTROL_PROM_US,
		.func_p = &control_prom_timer,
		.param_p = NULL,
	};
	TimerInit(&timer_promedio);*/
    
	/* Inicialización del conteo de timers */
	TimerStart(timer_readacc.timer);
	//TimerStart(timer_promedio.timer);

    return 1;
}

/*==================[external functions definition]==========================*/
void app_main(void){
    if(InitPdmtr()){
        //Podometro inicializado
    }

    if(InitTasks()){
        //Tasks inicializadas
    }

    if(InitTimers()){
        //Timers inicialziados
    }
}