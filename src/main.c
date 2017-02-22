#include <stdlib.h>
#include <stdio.h>

#include <misc.h>
#include <FreeRTOS.h>
#include <task.h>

#include "uart.h"
#include "led.h"
#include "time.h"
#include "console.h"

struct gps_converter {
	struct uart uart1, uart2, uart3;
	struct console console;
};

static struct gps_converter _gps_converter;

void vApplicationStackOverflowHook(void){
	portENTER_CRITICAL();
	while(1);
}

void vApplicationMallocFailedHook(void){
	while(1);
}

static void _gps_task(void *ptr){
	struct gps_converter *self = (struct gps_converter*)ptr;;
	(void)self;
	while(1){
		led_on(0);
		sleep_ms(500);
		led_off(0);
		sleep_ms(500);
		//serial_write(&_gps_converter.uart1.serial, "faa\r\n", 5, 10);
		//serial_write(&_gps_converter.uart2.serial, "bar", 3, 10);
	}
}

static void _lidar_task(void *ptr){
	struct gps_converter *self = (struct gps_converter*)ptr;;

	static char line[255];

	while(1){
		int len = serial_readline(&self->uart1.serial, line, sizeof(line));
		if(len > 0){
			static const int max_parts = 3 * 4;
			float values[max_parts];
			sscanf(line, "%f %f %f %f %f %f %f %f %f %f %f %f",
				&values[0], &values[1], &values[2],
				&values[3], &values[4], &values[5],
				&values[6], &values[7], &values[8],
				&values[9], &values[10], &values[11]);
			serial_printf(&self->uart2.serial, "%d\n", (uint32_t)(values[0] * 1000));
		} else {
			sleep_ms(100);
		}
	}
}


struct console_command _commands[] = {
	
};

void SetSysClock();
int main(void){
	struct gps_converter *self = &_gps_converter;
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

	SetSysClock();

	time_init();
	led_init();

	uart_init(&self->uart1, 1);
	uart_configure(&self->uart1, 115200, 8, 0);

	uart_init(&self->uart2, 2);
	uart_configure(&self->uart2, 115200, 8, 0);

	//console_init(&self->console, _commands, sizeof(_commands)/sizeof(_commands[0]));
	//console_start(&self->console, &self->uart1.serial);

	xTaskCreate(
		  _gps_task,
		  "gps",
		  configMINIMAL_STACK_SIZE * 3,
		  &_gps_converter,
		  tskIDLE_PRIORITY + 2UL,
		  NULL
	);

	xTaskCreate(
		  _lidar_task,
		  "lidar",
		  configMINIMAL_STACK_SIZE * 3,
		  &_gps_converter,
		  tskIDLE_PRIORITY + 2UL,
		  NULL
	);

	vTaskStartScheduler();

	while(1);
	return 0;
}
