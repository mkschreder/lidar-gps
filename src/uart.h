#pragma once

#include <stdint.h>
#include <stddef.h>

#include "serial.h"

struct uart {
	// leave this at the top!
	struct serial serial;

	void *driver_data;
};

int uart_init(struct uart *self, uint8_t hw_idx);
int uart_configure(struct uart *self, uint32_t baud, uint8_t data_bits, uint8_t parity);
//int uart_write(struct uart *self, const void *data, size_t size, uint32_t timeout_ms);
//int uart_read(struct uart *self, void *data, size_t size, uint32_t timeout_ms);
