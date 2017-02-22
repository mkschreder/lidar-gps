#include <string.h>
#include "console.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>

#include <FreeRTOS.h>
#include <task.h>

#include "serial.h"
#include "time.h"

#include <errno.h>

#define CONSOLE_WRITE_TIMEOUT 10
#define CONSOLE_MAX_ARGS 8
#define CONSOLE_MAX_LINE 80

static int strtokenize(char *buf, size_t len, char *tokens[], uint8_t ntokens){
	if(!buf || (len <= 0) || !tokens || !ntokens) return -1;

	uint8_t tok = 0;
	size_t c = 0;
	while(c < len){
		// skip leading spaces and special chars
		while((buf[c] == ' ' || buf[c] < 0x0f) && c < len) {
			if(buf[c] == 0) break;
			buf[c] = 0;
			c++;
		}
		// if reached end of string or end of buffer then break
		if(buf[c] == 0 || c >= len) break;
		// store current position in the string and increase token counter
		tokens[tok++] = buf + c;
		// break if we are out of tokens
		if(tok == ntokens) break;
		// skip the word until next space or end of string
		while(buf[c] != ' ' && buf[c] > 0x0f && c < len) c++;
	}
	return tok;
}

void con_printf(struct console *self, const char *fmt, ...){
	char dest[CONSOLE_MAX_LINE];
	memset(dest, 0, sizeof(dest));
	va_list argptr;
    va_start(argptr, fmt);
    vsnprintf(dest, sizeof(dest), fmt, argptr);
    va_end(argptr);
    serial_write(self->serial, dest, strlen(dest), CONSOLE_WRITE_TIMEOUT);
}
static int _compare_tasks(const void *a, const void *b){
	TaskStatus_t *ta = (TaskStatus_t*)a;
	TaskStatus_t *tb = (TaskStatus_t*)b;
	return tb->xTaskNumber < ta->xTaskNumber;
}

static int _cmd_ps(struct console *self, int argc, char **argv){
	(void)argc;
	(void)argv;
	// realtime tasks
	static TaskStatus_t status[CONSOLE_MAX_PS_TASKS];
	memset(status, 0, sizeof(status));
	uint32_t total_time;
	con_printf(self, "== realtime tasks\r\n");
	int16_t ret = uxTaskGetSystemState(status, sizeof(status)/sizeof(status[0]), &total_time);
	if(ret > 0){
		qsort(status, ret, sizeof(status[0]), _compare_tasks);
		uint32_t total_calculated = 0;
		uint32_t prev_total_calculated = 0;
		for(int c = 0; c < ret; c++){
			TaskStatus_t *stat = &status[c];
			TaskStatus_t *prev_stat = &self->prev_status[c];
			total_calculated += stat->ulRunTimeCounter;
			prev_total_calculated += prev_stat->ulRunTimeCounter;
		}
		con_printf(self, "time elapsed: %u, time usr: %u\r\n", total_time, total_calculated);
		con_printf(self, "heap: %lu free of %lu bytes\r\n", xPortGetFreeHeapSize(), configTOTAL_HEAP_SIZE);
		con_printf(self, "%5s%5s%8s%8s%10s%8s%8s\r\n", "id", "prio", "name", "stack", "cpu (us)", "cpu (%)", "load");
		for(int c = 0; c < ret; c++){
			TaskStatus_t *stat = &status[c];
			TaskStatus_t *prev_stat = &self->prev_status[c];
			uint32_t dtotal = total_calculated - prev_total_calculated;
			uint32_t run_time = stat->ulRunTimeCounter;
			int32_t drun_time = stat->ulRunTimeCounter - prev_stat->ulRunTimeCounter;

			if(drun_time < 0) drun_time = 0;

			uint32_t cpu_percent = 0;
			uint32_t dcpu_percent = 0;

			if(total_calculated && dtotal){
				cpu_percent = (uint32_t)((uint64_t)run_time * 10000 / total_calculated);
				dcpu_percent = (uint32_t)(((uint64_t)drun_time * 10000) / dtotal);
			}

			uint32_t cpu_whole = cpu_percent / 100;
			uint32_t cpu_frac = cpu_percent % 100;

			uint32_t dcpu_whole = dcpu_percent / 100;
			uint32_t dcpu_frac = dcpu_percent % 100;

			con_printf(self, "%5u%5u%8s%8u%10u%5u.%02u%5u.%02u\r\n",
					stat->xTaskNumber, stat->uxBasePriority, stat->pcTaskName, stat->usStackHighWaterMark, stat->ulRunTimeCounter, cpu_whole, cpu_frac, dcpu_whole, dcpu_frac);
		}
		memcpy(self->prev_status, status, sizeof(self->prev_status));
	} else {
		con_printf(self, "(none)\n");
	}
	return 0;
}

static int _cmd_reboot(struct console *self, int argc, char **argv){
	if(argc == 2 && strcmp(argv[1], "bootloader") == 0){
		con_printf(self, "Rebooting to bootloader..\n");
		sleep_ms(1000);
//		sys_reset_to_bootloader();
	} else {
		//sys_reset();
	}
	return 0;
}

static int _cmd_help(struct console *self, int argc, char **argv){
	(void)argc; (void)argv;
	for(size_t c = 0; c < self->ncommands; c++){
		struct console_command *cmd = self->commands + c;
		con_printf(self, "%s", cmd->name);
		if(cmd->options) con_printf(self, " %s", cmd->options);
		con_printf(self, "\n");
		if(cmd->description) con_printf(self, "\t%s\n", cmd->description);
	}
	con_printf(self, "ps\n");
	con_printf(self, "\tshow running tasks\n");
	con_printf(self, "help\n");
	con_printf(self, "\tshow this help\n");
	return 0;
}

void console_init(struct console *self, struct console_command *cmds, size_t ncommands){
	memset(self, 0, sizeof(*self));
	self->commands = cmds;
	self->ncommands = ncommands;
}

static int con_readline(struct console *self, char *line, size_t size){
	// buffer must be at least 1 char to accomodate for a \0
	if(size < 1) return -1;
	int rd;
	char ch;
	size_t pos = 0;
	while((rd = serial_read(self->serial, &ch, 1, portMAX_DELAY)) > 0){
		// emulate backspace correctly
		if(ch == 0x08 || ch == 0x7f){
			//serial_write(self->serial, "\x1b[D \x1b[D", 7);
			if(pos) {
				serial_write(self->serial, "\x08 \x08", 3, CONSOLE_WRITE_TIMEOUT);
				line[--pos] = 0;
			}
			continue;
		}
		serial_write(self->serial, &ch, 1, CONSOLE_WRITE_TIMEOUT);
		if(ch == '\n') break;
		line[pos++] = ch;
		if(pos == (size - 1)) break;
	}
	if(rd < 0) return rd;
	line[pos] = 0;
	return pos;
}

static void _console_task(void *ptr){
	struct console *self = (struct console*)ptr;
	while(1){
		con_printf(self, "# ");
		char line[CONSOLE_MAX_LINE];
		int rd;
		memset(line, 0, sizeof(line));

		rd = con_readline(self, line, sizeof(line));
		if(rd < 0) {
			con_printf(self, "Internal error\n");
			continue;
		}

		char *argv[CONSOLE_MAX_ARGS];
		memset(argv, 0, sizeof(argv));
		int argc = strtokenize(line, rd, argv, 8);

		if(argc == 0)
			continue;

		// reset optind because getopt uses it for first parameter and we need to start from the begining for each command
		optind = 1;
		for(int c = 0; c < argc; c++){
			con_printf(self, "arg %d: %s\n", c, argv[c]);
		}
		// find the command and run it
		uint8_t handled = 0;
		for(size_t c = 0; c < self->ncommands; c++){
			struct console_command *cmd = self->commands + c;
			if(strcmp(cmd->name, argv[0]) == 0 && cmd->proc){
				if(cmd->proc(self, argc, argv) < 0){
					con_printf(self, "Invalid arguments to command\n");
				}
				handled = 1;
				break;
			}
		}
		if(!handled){
			if(strcmp("ps", argv[0]) == 0){
				_cmd_ps(self, argc, argv);
			} else if(strcmp("reboot", argv[0]) == 0){
				_cmd_reboot(self, argc, argv);
			} else {
				_cmd_help(self, 0, NULL);
			}
		}
	}
}

void console_start(struct console *self, struct serial *port){
	(void)self;
	self->serial = port;

	xTaskCreate(
		  _console_task,
		  "shell",
		  configMINIMAL_STACK_SIZE * 3,
		  self,
		  tskIDLE_PRIORITY + 3UL,
		  NULL);
}

