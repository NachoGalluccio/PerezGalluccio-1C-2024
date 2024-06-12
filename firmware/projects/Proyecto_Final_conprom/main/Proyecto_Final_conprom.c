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

#include "ADXL335.h"
#include "led.h"
#include "timer_mcu.H"

#include "ble_mcu.h"

#include "math.h"
/*==================[macros and definitions]=================================*/
/** @def BAUD_RATE
 * @brief baud rate
*/
#define BAUD_RATE 115200

/** @def DECIMAL
 * @brief Constante 10
*/
#define DECIMAL 10

/** @def ULTIMOS_VALORES
 * @brief Cuantos valores se toman en cuenta para suavizar la señal
*/
#define ULTIMOS_VALORES 10

/** @def LECTURAS_CALIBRACION
 * @brief Cuantos valores se quieren tomar en cuenta
*/
#define LECTURAS_CALIBRACION 100

/** @def FREC_LECTURA_US
 * @brief frecuencia de lectura de datos en us
*/
#define FREC_LECTURA_US 20000

/** @def PERIODO_REFRACTARIO_US
 * @brief Tiempo por el cual no se va a tomar una nueva lectura
*/
#define PERIODO_REFRACTARIO_US 400000

#define CONFIG_BLINK_PERIOD 500

#define LED_BT	LED_1

/*==================[internal data definition]===============================*/
TaskHandle_t readacc_task_handle = NULL;

/** @def cantidad_pasos
 * @brief Define la cantidad de pasos que se dieron
*/
int cantidad_pasos = 0;

/** @def umbral
 * @brief Define el valor del umbral para medir los pasos
*/
float umbral = 0.3;

/** @def x_promedio
 * @brief Define el promedio de los valores leidos en X en el acelerometro en etapa de calibracion
*/
float x_promedio = 0;

/** @def y_promedio
 * @brief Define el promedio de los valores leidos en Y en el acelerometro en etapa de calibracion
*/
float y_promedio = 0;

/** @def z_promedio
 * @brief Define el promedio de los valores leidos en Z en el acelerometro en etapa de calibracion
*/
float z_promedio = 0;

/*==================[internal functions declaration]=========================*/

/** @fn readacc_timer
 *  @brief Función invocada en la interrupción del timer A
 */
void readacc_timer(void *param)
{
	xTaskNotifyGive(readacc_task_handle); /* Envía una notificación a la tarea asociada */
}

/** @fn control_prom_timer
 *  @brief Función invocada en la interrupción del timer B
 */
void control_prom_timer(void *param)
{
	//xTaskNotifyGive(control_prom_task_handle); /* Envía una notificación a la tarea asociada */
}

/** @fn readacc_task
 * 	@brief Tarea que controla el evento de leer el acelerometro
*/
void readacc_task(void *pvParameter)
{
    float vector_acc = 0;

    uint16_t t_refractario = 0;

    bool contar_pasos = true;
    bool tiempo_muerto = false;

    float ultimos_vectores_acc[ULTIMOS_VALORES] = {0};
    float promedio_ultimos_vectores_acc = 0;
    
    uint8_t contador = 0;

    float lectura_x, lectura_y, lectura_z;
    
	char msg[48];
    
	while (1)
	{        
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); /* Espera aca hasta nueva notificacion */
        /* Codigo para encontrar maximos y hacer umbral adaptativo */

        /* Codigo para contar pasos */
        lectura_x = ReadXValue();
        lectura_y = ReadYValue();
        lectura_z = ReadZValue();

        // Calcula el vector_acc
        // Saco del promedio el valor que voy a sobre escribir
        promedio_ultimos_vectores_acc = promedio_ultimos_vectores_acc*ULTIMOS_VALORES - ultimos_vectores_acc[contador];

        // Sobreescribo el ultimo valor leido
        ultimos_vectores_acc[contador] = sqrt((lectura_x-x_promedio)*(lectura_x-x_promedio) + (lectura_y-y_promedio)*(lectura_y-y_promedio) + (lectura_z-z_promedio)*(lectura_z-z_promedio));
        
        //Calculo el promedio
        promedio_ultimos_vectores_acc = (promedio_ultimos_vectores_acc + ultimos_vectores_acc[contador])/ULTIMOS_VALORES;

        vector_acc = promedio_ultimos_vectores_acc;

        if(tiempo_muerto){//Tiempo en que no se toma lectura
            t_refractario = t_refractario + FREC_LECTURA_US/1000;
            if (t_refractario >= PERIODO_REFRACTARIO_US/1000){
                t_refractario = 0;
                contar_pasos = !contar_pasos;
                tiempo_muerto = !tiempo_muerto;
            }
        } else {
            if(vector_acc < umbral*0.9 && !contar_pasos){
                tiempo_muerto = !tiempo_muerto;
            }
        
            if (vector_acc > umbral*1.1 && contar_pasos){// Controla si se dio un paso o no
                //Levanta bandera para no seguir contando pasos
                contar_pasos = !contar_pasos; 

                cantidad_pasos++; //Suma un paso al detectar que el vector_acc supero el umbral y la bandera esta baja

                //Se da formato y se envia msg por BT
                sprintf(msg, "Cantidad de pasos: %d\n", cantidad_pasos );

                BleSendString(msg);
            }   
        }
        
        contador++;
        if(contador >= ULTIMOS_VALORES) contador = 0; //Reinicia contador
	}
}

/** @fn Calibrate_Acc
 * 	@brief Calibra el acelerometro calculando los valores promedios del estado inicial
 * 	@return 1 (TRUE) si no hay error
*/
bool Calibrate_Acc(){
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

void read_data(uint8_t * data, uint8_t length){
    /* Data */
    if(data[0] == 'R'){
        cantidad_pasos = 0;
        Calibrate_Acc();
    } 
}
/*==================[external functions definition]==========================*/
void app_main(void){
    /* Inicializacion de acelerometro */
    ADXL335Init();

    /* Calibracion */
    Calibrate_Acc();

    /* Inicializacion LEDS */
    LedsInit();

    /* Inicializacion BT */
    ble_config_t ble_configuration = {
        "NachoP",
        read_data
    };

    BleInit(&ble_configuration);

    /* Inicialización de timers */
    timer_config_t timer_readacc= {
		.timer = TIMER_A,
		.period = FREC_LECTURA_US,
		.func_p = &readacc_timer,
		.param_p = NULL,
	};
	TimerInit(&timer_readacc);

    /* Task creation */
	xTaskCreate(&readacc_task, "Reading", 2048, NULL, 5, &readacc_task_handle);
	//xTaskCreate(&control_prom_task, "Calculo_Prom", 1024, NULL, 5, &control_prom_task_handle);

	/* Inicialización del conteo de timers */
	TimerStart(timer_readacc.timer);

    // While para usar LEDs de guia para la conexion BT
    while(1){
        vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
        switch(BleStatus()){
            case BLE_OFF:
                LedOff(LED_BT);
            break;
            case BLE_DISCONNECTED:
                LedToggle(LED_BT);
            break;
            case BLE_CONNECTED:
                LedOn(LED_BT);
            break;
        }
    }
}