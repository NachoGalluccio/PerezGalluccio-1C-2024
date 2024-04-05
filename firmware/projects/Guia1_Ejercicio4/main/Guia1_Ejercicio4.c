/*! @mainpage Guia1_Ejercicio4
 *
 * @section genDesc General Description
 *
 * La aplicacion permite mostar un numero de hasta tres cifras en una pantalla LCD.
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
 * | 05/04/2024 | Document creation		                         |
 *
 * @author Perez Ignacio
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
#include "switch.h"
/*==================[macros and definitions]=================================*/
/** @def CONFIG_PERIOD
 *  @brief Tiempo en ms de actualizacion
*/
#define CONFIG_PERIOD 500

/** @def gpioConf_t
 *  @brief struct que define un pin GPIO y su direccion, entrada o salida.
*/
typedef struct
{
	gpio_t pin;		/*!< GPIO  pin number*/
	io_t dir;		/*!< GPIO direction '0' IN; '1' OUT*/	
} gpioConf_t;

/** @def BCDPins
 *  @brief Arreglo de GPIO de salidas
*/
gpioConf_t BCDPins[4] = {{GPIO_20,GPIO_OUTPUT},{GPIO_21,GPIO_OUTPUT},{GPIO_22,GPIO_OUTPUT},{GPIO_23,GPIO_OUTPUT}};

/** @def LCDPins
 *  @brief Arreglo de GPIO de salidas LCD
*/
gpioConf_t LCDPins[3] = {{GPIO_9,GPIO_OUTPUT},{GPIO_18,GPIO_OUTPUT},{GPIO_19,GPIO_OUTPUT}};
/*==================[internal data definition]===============================*/

/*==================[internal functions declaration]=========================*/
/** @fn ConvertToBCDArray
 * 	@brief Separa un numero en sus digitos
 * 	@param [data] [digits] [bcd_number]
 * 	@return
*/
void ConvertToBCDArray(uint32_t data, uint8_t digits, uint8_t* bcd_number){
	for (int i = 0; i < digits; i++)
	{
		bcd_number[i] = data%10;
		data = data/10;
	}		
}
/** @fn BCDToGPIO
 * 	@brief Prepara un digito en los GPIO
 * 	@param [bcd_digit] [gpioConf]
 * 	@return
*/
void BCDToGPIO(uint8_t bcd_digit, gpioConf_t* gpioConf)
{
	for (int i = 0; i < 4; i++)
	{
		GPIOInit(gpioConf[i].pin,gpioConf[i].dir);
		if(bcd_digit & 1<<i){
			GPIOOn(gpioConf[i].pin);
		} else {
			GPIOOff(gpioConf[i].pin);
		}
	}	
}
/** @fn BCDToLCD
 * 	@brief Guarda un digito en la posicion del lcd
 * 	@param [data] [digits] [gpioConf] [gpioLCD]
 * 	@return
*/
void BCDToLCD(uint32_t data, uint8_t digits, gpioConf_t* gpioConf, gpioConf_t* gpioLCD)
{
	uint8_t digitos_BCD[digits];

	ConvertToBCDArray(data, digits, digitos_BCD);

	for (int i = 0; i < digits; i++)
	{
		GPIOInit(gpioLCD[i].pin,gpioLCD[i].dir);
		BCDToGPIO(digitos_BCD[i],gpioConf);
		GPIOOn(gpioLCD[i].pin);
		GPIOOff(gpioLCD[i].pin);
	}
	
}
/*==================[external functions definition]==========================*/
void app_main(void){
	uint32_t Numero = 554;
	uint8_t Digitos = 3;
	uint8_t teclas;

	BCDToLCD(Numero, Digitos, BCDPins, LCDPins);
	SwitchesInit();

	while(1){
    	teclas  = SwitchesRead();
    	switch(teclas){
    		case SWITCH_1:
    			Numero++;
				BCDToLCD(Numero, Digitos, BCDPins, LCDPins);
    		break;
    		case SWITCH_2:
				if(Numero>0){
					Numero--;
					BCDToLCD(Numero, Digitos, BCDPins, LCDPins);
				}
    		break;
			case SWITCH_1+SWITCH_2:
				Numero = 0;
			break;
		}
		vTaskDelay(CONFIG_PERIOD / portTICK_PERIOD_MS);
	}
}
/*==================[end of file]============================================*/