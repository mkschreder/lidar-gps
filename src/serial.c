#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include <FreeRTOS.h>
#include <task.h>

#include "serial.h"

#define SERIAL_WRITE_TIMEOUT 100
#define SERIAL_MAX_LINE 80

int serial_readline(struct serial *self, char *line, size_t size){
	// buffer must be at least 1 char to accomodate for a \0
	if(size < 1) return -1;
	int rd;
	char ch;
	size_t pos = 0;
	while((rd = serial_read(self, &ch, 1, portMAX_DELAY)) > 0){
		serial_write(self, &ch, 1, SERIAL_WRITE_TIMEOUT);
		if(ch == '\n') break;
		line[pos++] = ch;
		if(pos == (size - 1)) break;
	}
	if(rd < 0) return rd;
	line[pos] = 0;
	return pos;
}

int serial_printf(struct serial *self, const char *fmt, ...){
	char dest[SERIAL_MAX_LINE];
	memset(dest, 0, sizeof(dest));
	va_list argptr;
    va_start(argptr, fmt);
    int ret = vsnprintf(dest, sizeof(dest), fmt, argptr);
    va_end(argptr);
    serial_write(self, dest, strlen(dest), SERIAL_WRITE_TIMEOUT);
	return ret;
}

