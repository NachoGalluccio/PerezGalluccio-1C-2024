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
/*==================[macros and definitions]=================================*/
#define ON 1
#define OFF 2
#define TOGGLE 3
/*==================[internal data definition]===============================*/
struct leds{
	uint8_t n_led;		/* Indica el led a controlar*/
	uint8_t n_ciclos; 	/* Indica cantidad de ciclos en modo TOGGLE*/
	uint16_t periodo;	/* Indica tiempo de cada ciclo en ms en modo TOGGLE*/
	uint8_t mode; 		/* ON, OFF, TOGGLE*/
} my_leds;
/*==================[internal functions declaration]=========================*/
void controlLeds (struct leds *ledsP){
	switch(ledsP->mode){
		case ON:
			switch(ledsP->n_led){
				case 1:
					LedOn(LED_1);
				break;
				case 2:
					LedOn(LED_2);
				break;
				case 3:
					LedOn(LED_3);
				break;
			}
			/*LedOn(ledsP->n_led);*/
		break;
		case OFF:
			switch(ledsP->n_led){
				case 1:
					LedOff(LED_1);
				break;
				case 2:
					LedOff(LED_2);
				break;
				case 3:
					LedOff(LED_3);
				break;
			}
			/*LedOff(ledsP->n_led);*/
		break;
		case TOGGLE:
			uint16_t retardo = (ledsP->periodo)/2;
			for (uint8_t i = 0; i < ledsP->n_ciclos; i++)
			{
				switch(ledsP->n_led){
				case 1:
					LedToggle(LED_1);
				break;
				case 2:
					LedToggle(LED_2);
				break;
				case 3:
					LedToggle(LED_3);
				break;
				}
				vTaskDelay(retardo / portTICK_PERIOD_MS);	
				switch(ledsP->n_led){
				case 1:
					LedToggle(LED_1);
				break;
				case 2:
					LedToggle(LED_2);
				break;
				case 3:
					LedToggle(LED_3);
				break;
				}
				vTaskDelay(retardo / portTICK_PERIOD_MS);				
			}
			
			/*for (size_t i = 0; i < ledsP->n_ciclos; i++)
			{
				LedToggle(ledsP->n_led);
				for (int j = 0; j < ledsP->periodo; j++)
				{
					vTaskDelay(CONFIG_TASK_DELAY / portTICK_PERIOD_MS);
				}
			}*/
		break;
	}
}
/*==================[external functions definition]==========================*/
void app_main(void){
	LedsInit();
	my_leds.mode = TOGGLE;
	my_leds.n_ciclos = 100;
	my_leds.periodo = 500;
	my_leds.n_led = 2;
	controlLeds(&my_leds);
	my_leds.mode = ON;
	/*while(1){
		for (int i = 1; i <= 3 ; i++)
		{
			my_leds.n_led = i;
			controlLeds(&my_leds);
			vTaskDelay(100 / portTICK_PERIOD_MS);
		}
		if (my_leds.mode == ON){
			my_leds.mode = OFF;
		} else {
			my_leds.mode = ON;
		} 
	}*/
}
/*==================[end of file]============================================*/