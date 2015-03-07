#include <LPC17xx.h>
#include "string.h"
#include "k_sys_proc.h"
#include "k_rtx.h"
#include "k_memory.h"
#include "k_process.h"


/**
 * The Null Process with priority 4
 */
void null_proc(void) {
	while (1) {
		k_release_processor();
	}
}

void enqueue_env(PCB *p, ENVELOPE *env) {
	env->next = NULL;
	if (p->first_msg == NULL)
	{
		p->first_msg = env;
		p->last_msg = env;
	}
	else
	{
		p->last_msg->next = env;
		p->last_msg = p->last_msg->next;
	}
}

/**
 * Dequeues the head of the queue
 * Returns a pointer to the dequeued PCB node
 */
ENVELOPE* dequeue_env(PCB *p) {
	ENVELOPE *curHead = p->first_msg;
	if (p->first_msg == p->last_msg)
	{
		p->first_msg = NULL;
		p->last_msg = NULL;
	}
	else
		p->first_msg = p->first_msg->next;
	curHead->next = NULL;
	return curHead;
}

int is_empty_env(PCB *p) {
	return (p->first_msg == NULL);
}

ENVELOPE* k_non_blocking_receive_message(int pid){
	PCB* timer;
	
	timer = gp_pcbs[pid];
	if(!is_empty_env(timer)){
		return dequeue(timer);
	}
	return NULL;
}

void timer_i_proc(void) {
	ENVELOPE* lope;
	lope = k_non_blocking_receive_message(TIMER_PID);
	while (lope != NULL){
		
		lope = k_non_blocking_receive_message(TIMER_PID);
	}
	
}

void uart_i_proc(void) {}

void kcd_proc(void) {}

void crt_proc(void) {}

