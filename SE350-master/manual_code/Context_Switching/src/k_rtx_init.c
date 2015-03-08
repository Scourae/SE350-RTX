/** 
 * @file:   k_rtx_init.c
 * @brief:  Kernel initialization C file
 * @auther: Yiqing Huang
 * @date:   2014/01/17
 */

#include "k_rtx_init.h"
#include "uart_polling.h"
#include "k_memory.h"
#include "k_process.h"
#include "timer.h"
#include "uart_def.h"

void k_rtx_init(void)
{
	//SystemInit();
	__disable_irq();
	uart_irq_init(0);
	timer_init(0); /* initialize timer 0 */
	uart0_init();   
	memory_init();
	process_init();
	__enable_irq();

	/* start the first process */

	k_release_processor();
}
