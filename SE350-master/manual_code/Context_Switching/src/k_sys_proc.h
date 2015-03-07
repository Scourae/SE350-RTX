#ifndef K_SYSTEM_PROC_H
#define K_SYSTEM_PROC_H

#include "k_rtx.h"

extern volatile U32 g_timer_count;

typedef struct env_queue{
	ENVELOPE* head;
	ENVELOPE* tail;
} ENV_QUEUE;

/* indefinitely releases the processor */
void null_proc(void);

/* timing service i-process */
void timer_i_proc(void);

/* uart i-process */
void uart_i_proc(void);

/* KCD process */
void kcd_proc(void);

/* CRT process */
void crt_proc(void);

#endif /*K_SYSTEM_PROC_H*/
