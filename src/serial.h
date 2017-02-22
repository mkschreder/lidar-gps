#pragma once

#include <stdint.h>
#include <stddef.h>

struct serial {
	int (*write)(struct serial *self, const void *ptr, size_t size, uint32_t timeout_ms);
	int (*read)(struct serial *self, void *ptr, size_t size, uint32_t timeout_ms);
};

#define serial_read(s, b, sz, t) (s)->read(s, b, sz, t)
#define serial_write(s, b, sz, t) (s)->write(s, b, sz, t)

int serial_readline(struct serial *self, char *line, size_t size);
int serial_printf(struct serial *self, const char *fmt, ...);

