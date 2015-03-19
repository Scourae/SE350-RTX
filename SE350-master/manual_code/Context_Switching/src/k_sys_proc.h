#ifndef K_SYSTEM_PROC_H
#define K_SYSTEM_PROC_H

#include "k_rtx.h"
#include "k_memory.h"
#include "k_ipc.h"

#define INPUT_BUFFER_SIZE (MEMORY_BLOCK_SIZE-HEADER_OFFSET)

extern volatile U32 g_timer_count;

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

/* Wall clock process */
void wall_clock_proc(void);

/* Set priority process */
void set_priority_proc(void);
#endif /*K_SYSTEM_PROC_H*/
