#include <LPC17xx.h>
#include "string.h"
#include "k_sys_proc.h"
#include "k_rtx.h"
#include "k_memory.h"
#include "k_process.h"

ENV_QUEUE t_queue;

int delayed_send(int process_id, void * env, int delay){
	ENVELOPE *lope = (ENVELOPE *) env;
	lope->delay = delay;
	//return k_send_message(TIMER_PID, env);
	return 0;
}

/**
 * The Null Process with priority 4
 */
void null_proc(void) {
	while (1) {
		k_release_processor();
	}
}

void enqueue_env(PCB *p, ENVELOPE *env) {
	env->nextMsg = NULL;
	if (p->first_msg == NULL)
	{
		p->first_msg = env;
		p->last_msg = env;
	}
	else
	{
		p->last_msg->nextMsg = env;
		p->last_msg = p->last_msg->nextMsg;
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
		p->first_msg = p->first_msg->nextMsg;
	curHead->nextMsg = NULL;
	return curHead;
}

int is_empty_env(PCB *p) {
	return (p->first_msg == NULL);
}

ENVELOPE* k_non_blocking_receive_message(int pid){
	PCB* timer;
	
	timer = gp_pcbs[pid];
	if(!is_empty_env(timer)){
		return dequeue_env(timer);
	}
	return NULL;
}

void sorted_insert(ENVELOPE* lope){
	lope->nextMsg = NULL;
	if (t_queue.head == NULL){
		t_queue.head = lope;
		t_queue.tail = lope;
	}
	//Only one element currently in the queue
	else if (t_queue.head == t_queue.tail){
		if (t_queue.head->delay < lope->delay){
			t_queue.head->nextMsg = lope;
			t_queue.tail = lope;
		}
		else {
			t_queue.head = lope;
			lope->nextMsg = t_queue.tail;
			t_queue.tail->nextMsg = NULL;
		}
	}
	else {
		//Checking bounds for when lope expiry is less than all
		if (lope->delay < t_queue.head->delay){
			lope->nextMsg = t_queue.head;
			t_queue.head = lope;
		}
		//Checking bounds for when lope expiry is >= to all
		else if (lope->delay >= t_queue.tail->delay){
			t_queue.tail->nextMsg = lope;
			t_queue.tail = lope;
		}
		else {
			ENVELOPE* cur = t_queue.head;
			ENVELOPE* prev = t_queue.head;
			while ((cur != NULL) && (lope->delay >= cur->delay)){
				prev = cur;
				cur = cur->nextMsg;
			}
			prev->nextMsg = lope;
			lope->nextMsg = cur;
		}
	}
}

ENVELOPE* dequeue_env_queue(ENV_QUEUE *q){
	ENVELOPE *curHead = q->head;
	if (q->head == q->tail)
	{
		q->head = NULL;
		q->tail = NULL;
	}
	else
		q->head = q->head->nextMsg;
	curHead->nextMsg = NULL;
	return curHead;
}

void timer_i_proc(void) {
	ENVELOPE* lope = NULL;
	
	__disable_irq(); // make this process non blocking
	
	lope = k_non_blocking_receive_message(TIMER_PID);
	
	// request all of the new envelopes that need to be send later
	while (lope != NULL){
		sorted_insert(lope);
		lope = k_non_blocking_receive_message(TIMER_PID);
	}
	
	while (t_queue.head != NULL && t_queue.head->delay <= g_timer_count){
		ENVELOPE* cur = dequeue_env_queue(&t_queue);
		//k_send_message (cur->destination_pid, (void *) cur);
	}
	
	__enable_irq();
}

void uart_i_proc(void) {}

void kcd_proc(void) {}

void crt_proc(void) {}

