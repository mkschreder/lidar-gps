#include <stm32f10x.h>
#include <stm32f10x_usart.h>
#include <stm32f10x_gpio.h>
#include <misc.h>

#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <FreeRTOS.h>
#include <queue.h>
#include <portable.h>

#include "uart.h"

struct uart_hw {
	USART_TypeDef *hw;
	QueueHandle_t tx_queue;
	QueueHandle_t rx_queue;
};

#define UART_NUM_DEVICES 4
static struct uart *_uart_ptr[UART_NUM_DEVICES] = {0, 0, 0, 0};

static void _uart1_init(void){
	USART_InitTypeDef uart;
	NVIC_InitTypeDef nvic;

	GPIO_InitTypeDef gpio;
	GPIO_StructInit(&gpio);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	// TX
	gpio.GPIO_Pin = GPIO_Pin_9;
	gpio.GPIO_Mode = GPIO_Mode_AF_PP;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpio);

	// RX
	gpio.GPIO_Pin = GPIO_Pin_10;
	gpio.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOA, &gpio);

	USART_StructInit(&uart);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

	// uart itself
	uart.USART_BaudRate = 9600;				// the baudrate is set to the value we passed into this init function
	uart.USART_WordLength = USART_WordLength_8b;// we want the data frame size to be 8 bits (standard)
	uart.USART_StopBits = USART_StopBits_1;		// we want 1 stop bit (standard)
	uart.USART_Parity = USART_Parity_No;		// we don't want a parity bit (standard)
	uart.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // we don't want flow control (standard)
	uart.USART_Mode = USART_Mode_Tx | USART_Mode_Rx; // we want to enable the transmitter and the receiver
	USART_Init(USART1, &uart);					// again all the properties are passed to the USART_Init function which takes care of all the bit setting

	// enable rx interrupt
	nvic.NVIC_IRQChannel = USART1_IRQn;		 // we want to configure the USART1 interrupts
	nvic.NVIC_IRQChannelPreemptionPriority = 1;// this sets the priority group of the USART1 interrupts
	nvic.NVIC_IRQChannelSubPriority = 0;		 // this sets the subpriority inside the group
	nvic.NVIC_IRQChannelCmd = ENABLE;			 // the USART1 interrupts are globally enabled
	NVIC_Init(&nvic);							 // the properties are passed to the NVIC_Init function which takes care of the low level stuff	

	// enable interrupts
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
	
	USART_Cmd(USART1, ENABLE);
}

static void _uart2_init(void){
	GPIO_InitTypeDef gpio;
	USART_InitTypeDef uart;
	NVIC_InitTypeDef nvic;

	GPIO_StructInit(&gpio);
	USART_StructInit(&uart);

	// clocks
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	// TX
	gpio.GPIO_Pin = GPIO_Pin_2;
	gpio.GPIO_Mode = GPIO_Mode_AF_PP;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpio);

	// RX
	gpio.GPIO_Mode = GPIO_Mode_IPU;
	gpio.GPIO_Pin = GPIO_Pin_3; // PA2 tx, PA3 rx
	GPIO_Init(GPIOA, &gpio);

	// alternative functions
	//GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2); //
	//GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2);

	// uart itself
	uart.USART_BaudRate = 9600;				// the baudrate is set to the value we passed into this init function
	uart.USART_WordLength = USART_WordLength_8b;// we want the data frame size to be 8 bits (standard)
	uart.USART_StopBits = USART_StopBits_1;		// we want 1 stop bit (standard)
	uart.USART_Parity = USART_Parity_No;		// we don't want a parity bit (standard)
	uart.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // we don't want flow control (standard)
	uart.USART_Mode = USART_Mode_Tx | USART_Mode_Rx; // we want to enable the transmitter and the receiver
	USART_Init(USART2, &uart);					// again all the properties are passed to the USART_Init function which takes care of all the bit setting

	// enable rx interrupt
	nvic.NVIC_IRQChannel = USART2_IRQn;		 // we want to configure the USART1 interrupts
	nvic.NVIC_IRQChannelPreemptionPriority = 1;// this sets the priority group of the USART1 interrupts
	nvic.NVIC_IRQChannelSubPriority = 0;		 // this sets the subpriority inside the group
	nvic.NVIC_IRQChannelCmd = ENABLE;			 // the USART1 interrupts are globally enabled
	NVIC_Init(&nvic);							 // the properties are passed to the NVIC_Init function which takes care of the low level stuff	

	// enable interrupts
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
	
	USART_Cmd(USART2, ENABLE);
}

/*
static void _uart3_init(void){
	GPIO_InitTypeDef gpio;
	USART_InitTypeDef uart;
	NVIC_InitTypeDef nvic;

	GPIO_StructInit(&gpio);
	USART_StructInit(&uart);

	// clocks
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

	// pins
	gpio.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
	gpio.GPIO_Mode = GPIO_Mode_AF;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	gpio.GPIO_OType = GPIO_OType_PP;
	gpio.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOC, &gpio);

	// alternative functions
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource10, GPIO_AF_USART3); //
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource11, GPIO_AF_USART3);

	// uart itself
	uart.USART_BaudRate = 9600;				// the baudrate is set to the value we passed into this init function
	uart.USART_WordLength = USART_WordLength_8b;// we want the data frame size to be 8 bits (standard)
	uart.USART_StopBits = USART_StopBits_1;		// we want 1 stop bit (standard)
	uart.USART_Parity = USART_Parity_No;		// we don't want a parity bit (standard)
	uart.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // we don't want flow control (standard)
	uart.USART_Mode = USART_Mode_Tx | USART_Mode_Rx; // we want to enable the transmitter and the receiver
	USART_Init(USART3, &uart);					// again all the properties are passed to the USART_Init function which takes care of all the bit setting

	// enable rx interrupt
	nvic.NVIC_IRQChannel = USART3_IRQn;		 // we want to configure the USART1 interrupts
	nvic.NVIC_IRQChannelPreemptionPriority = 1;// this sets the priority group of the USART1 interrupts
	nvic.NVIC_IRQChannelSubPriority = 0;		 // this sets the subpriority inside the group
	nvic.NVIC_IRQChannelCmd = ENABLE;			 // the USART1 interrupts are globally enabled
	NVIC_Init(&nvic);							 // the properties are passed to the NVIC_Init function which takes care of the low level stuff	

	// enable interrupts
	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
	
	USART_Cmd(USART3, ENABLE);
}
*/
static int _serial_write(struct serial *serial, const void *data, size_t size, uint32_t timeout){
	struct uart *self = (struct uart*)serial;
	struct uart_hw *hw = (struct uart_hw*)self->driver_data;
	if(!hw || !hw->hw || !hw->tx_queue) return -1;
	uint8_t *buf = (uint8_t*)data;
	int sent = 0;
	for(size_t c = 0; c < size; c++){
		if(xQueueSend(hw->tx_queue, &buf[c], timeout / portTICK_PERIOD_MS) == pdFALSE){
			// on timeout we break and just return the number of bytes sent so far
			break;
		}
		// after putting the data into the queue, we enable the interrupt
		USART_ITConfig(hw->hw, USART_IT_TXE, ENABLE);
		sent++;
	}
	return sent;
}

static int _serial_read(struct serial *serial, void *data, size_t size, uint32_t timeout){
	struct uart *self = (struct uart*)serial;
	struct uart_hw *hw = (struct uart_hw*)self->driver_data;
	if(!hw) return -1;
	char ch;
	char *buf = (char*)data;
	size_t pos = 0;
	// pop characters off the queue
	while(size && xQueueReceive(hw->rx_queue, &ch, (pos == 0)?(timeout / portTICK_PERIOD_MS):0) == pdTRUE){
		*(buf + pos) = ch;
		pos++;
		size--;
	}
	if(pos == 0) return -ETIMEDOUT;
	return pos;
}

static inline struct uart_hw *_get_hw(uint8_t id){
	struct uart *ptr = _uart_ptr[id - 1];
	if(!ptr) return NULL;
	struct uart_hw *hw = (struct uart_hw*)ptr->driver_data;
	return hw;
}

static BaseType_t _uart_irq(struct uart_hw *hw){
	BaseType_t wake = 0;
	// we check for incoming data on this device, ack the interrupt and copy data into the queue
	if( USART_GetITStatus(hw->hw, USART_IT_RXNE) ){
		USART_ClearITPendingBit(hw->hw, USART_IT_RXNE);
		char t = hw->hw->DR;
		if(hw && hw->rx_queue) xQueueSendFromISR(hw->rx_queue, &t, &wake);
	}

	// we check for transmission read on this device, ack the interrupt and either send next byte or turn off the interrupt
	if( USART_GetITStatus(hw->hw, USART_IT_TXE) ){
		USART_ClearITPendingBit(hw->hw, USART_IT_TXE);
		char ch;
		if(hw && hw->tx_queue) {
			if(xQueueReceiveFromISR(hw->tx_queue, &ch, &wake) == pdTRUE){
				hw->hw->DR = ch;
			} else {
				USART_ITConfig(hw->hw, USART_IT_TXE, DISABLE);
			}
		}
	}
	return wake;
}

void USART1_IRQHandler(void){
	struct uart_hw *hw = _get_hw(1);
	BaseType_t wake = _uart_irq(hw);
	portYIELD_FROM_ISR(wake);
}

void USART2_IRQHandler(void){
	struct uart_hw *hw = _get_hw(2);
	BaseType_t wake = _uart_irq(hw);
	portYIELD_FROM_ISR(wake);
}

void USART3_IRQHandler(void){
	struct uart_hw *hw = _get_hw(3);
	BaseType_t wake = _uart_irq(hw);
	portYIELD_FROM_ISR(wake);
}

int uart_configure(struct uart *self, uint32_t baud, uint8_t data_bits, uint8_t parity){
	struct uart_hw *hw = (struct uart_hw*)self->driver_data;
	if(!hw) return -1;

	USART_InitTypeDef uart;
	// uart itself
	uart.USART_BaudRate = baud;				// the baudrate is set to the value we passed into this init function
	uart.USART_WordLength = (data_bits == 9)?USART_WordLength_9b:USART_WordLength_8b;// we want the data frame size to be 8 bits (standard)
	uart.USART_StopBits = USART_StopBits_1;		// we want 1 stop bit (standard)
	if(parity == 0)
		uart.USART_Parity = USART_Parity_No;
	else if(parity == 1)
		uart.USART_Parity = USART_Parity_Odd;
	else if(parity == 2)
		uart.USART_Parity = USART_Parity_Even;
	uart.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // we don't want flow control (standard)
	uart.USART_Mode = USART_Mode_Tx | USART_Mode_Rx; // we want to enable the transmitter and the receiver
	USART_Init(hw->hw, &uart);					// again all the properties are passed to the USART_Init function which takes care of all the bit setting
	return 0;
}

int uart_init(struct uart *self, uint8_t hw_id){
	memset(self, 0, sizeof(*self));

	USART_TypeDef *uart = NULL;
	
	switch(hw_id){
		case 1:
			if(_uart_ptr[0]) return -1;
			_uart1_init();
			uart = USART1;
			_uart_ptr[0] = self;
			break;
		case 2:
			if(_uart_ptr[1]) return -1;
			_uart2_init();
			uart = USART2;
			_uart_ptr[1] = self;
			break;
		case 3:
			if(_uart_ptr[2]) return -1;
			//_uart3_init();
			uart = USART3;
			_uart_ptr[2] = self;
			break;
		default:
			return -1;
	}

	self->serial = (struct serial){
		.read = _serial_read,
		.write = _serial_write
	};

	struct uart_hw *hw = pvPortMalloc(sizeof(struct uart_hw));
	hw->tx_queue = xQueueCreate(64, sizeof(char));
	hw->rx_queue = xQueueCreate(128, sizeof(char));

	hw->hw = uart;
	self->driver_data = hw;

	return 0;
}


