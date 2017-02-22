#pragma once

#include <FreeRTOS.h>
#include <task.h>

#define CONSOLE_MAX_PS_TASKS 8

struct console {
	struct serial *serial;
	struct console_command *commands;
	size_t ncommands;
	//! this is used for displaying task stats
	TaskStatus_t prev_status[CONSOLE_MAX_PS_TASKS];
};

struct console_command {
	const char *name;
	int (*proc)(struct console *self, int argc, char **argv);
	const char *options;
	const char *description;
};


void console_init(struct console *self, struct console_command *commands, size_t ncommands);
void console_start(struct console *self, struct serial *uart);

void con_printf(struct console *self, const char *fmt, ...);
