#include <FreeRTOS.h>
#include <task.h>

#include <errno.h>

#include <stm32f10x_tim.h>
#include <stm32f10x_rcc.h>
#include "time.h"

// cycles per microsecond
static uint32_t usTicks = 0;
// current uptime for 1kHz systick timer. will rollover after 49 days. hopefully we won't care.
static volatile int32_t sysTickUptime = 0;

void time_init(void){
	RCC_ClocksTypeDef clocks;
	RCC_GetClocksFreq(&clocks);
	usTicks = clocks.SYSCLK_Frequency / 1000000;
    SysTick_Config(SystemCoreClock / 1000);
}

void vApplicationTickHook(void){
	__sync_fetch_and_add(&sysTickUptime, 1);
    //sysTickUptime++;
}

// Return system uptime in microseconds (rollover in 70minutes)
int32_t micros(void){
    register int32_t ms, cycle_cnt;
    do {
        ms = sysTickUptime;
        cycle_cnt = SysTick->VAL;

        /*
         * If the SysTick timer expired during the previous instruction, we need to give it a little time for that
         * interrupt to be delivered before we can recheck sysTickUptime:
         */
        asm volatile("\tnop\n");
    } while (ms != sysTickUptime);
    return (ms * 1000) + (usTicks * 1000 - cycle_cnt) / usTicks;
}

int sleep_ms(uint32_t ms){
	vTaskDelay((ms)/portTICK_PERIOD_MS);
	return 0;
}


