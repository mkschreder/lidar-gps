#include <stddef.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>
#include <misc.h>
#include "led.h"

struct led_config {
	GPIO_TypeDef *gpio;
	uint16_t pin;
};

static const struct led_config _leds[] = {
	{ .gpio = GPIOC, .pin = GPIO_Pin_13 },
};

void led_on(uint8_t id){
	if(id > (sizeof(_leds)/sizeof(_leds[0]))) return;
	GPIO_SetBits(_leds[id].gpio, _leds[id].pin);
}

void led_off(uint8_t id){
	if(id > (sizeof(_leds)/sizeof(_leds[0]))) return;
	GPIO_ResetBits(_leds[id].gpio, _leds[id].pin);
}

void led_init(void){
	GPIO_InitTypeDef gpio;

	for(size_t c = 0; c < (sizeof(_leds)/sizeof(_leds[0])); c++){
		// TODO
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOG, ENABLE);
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

		gpio.GPIO_Pin = _leds[c].pin;
		gpio.GPIO_Mode = GPIO_Mode_Out_PP;
		gpio.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_Init(_leds[c].gpio, &gpio);
	}
}
