#include <LPC17xx.h>
#include "string.h"
#include "k_sys_proc.h"
#include "k_rtx.h"
#include "k_ipc.h"
#include "k_memory.h"
#include "k_process.h"

ENV_QUEUE t_queue;
extern PCB* gp_current_process;
int send_message_preemption_flag = 1; // 0 for not preempting and 1 otherwise
int uart_preemption_flag = 1; // 0 for not preempting and 1 otherwise

char g_input_buffer[INPUT_BUFFER_SIZE]; // buffer char array to hold the input
int g_input_buffer_index = 0; // current index of the buffer such that all indices before this one holds a char
U8 g_char_in;

int delayed_send(int process_id, void * env, int delay){
	ENVELOPE *lope = (ENVELOPE *) env;
	int response = 0;
	lope->delay = g_timer_count + delay;
	send_message_preemption_flag = 0;
	response = k_send_message(TIMER_PID, env);
	send_message_preemption_flag = 1;
	return response;
}

/**
 * The Null Process with priority 4
 */
void null_proc(void) {
	while (1) {
		k_release_processor();
	}
}
/*
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
}*/

/**
 * Dequeues the head of the queue
 * Returns a pointer to the dequeued PCB node
 */


ENVELOPE* timer_dequeue(ENV_QUEUE *p) {
	ENVELOPE *curHead = p->head;
	if (msg_empty(p)) return NULL;
	if (p->head == p->tail)
	{
		p->head = NULL;
		p->tail = NULL;
	}
	else
		p->head = p->head->nextMsg;
	curHead->nextMsg = NULL;
	return curHead;
}

/*int is_empty_env(ENV_QUEUE *q) {
	return (q->head == NULL);
}*/

ENVELOPE* k_non_blocking_receive_message(int pid){
	PCB* timer = gp_pcbs[pid];
	ENV_QUEUE* env_q = &(timer->env_q);
	if(!msg_empty(env_q)){
		return timer_dequeue(env_q);
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
	int preemption_flag = 0;
	__disable_irq(); // make this process non blocking
	
	// TODO: figure out the LPC_TIM0
	//LPC_TIM0->IR = BIT(0);
	
	lope = k_non_blocking_receive_message(TIMER_PID);
	
	// request all of the new envelopes that need to be send later
	while (lope != NULL){
		sorted_insert(lope);
		lope = k_non_blocking_receive_message(TIMER_PID);
	}
	
	send_message_preemption_flag = 0;
	while (t_queue.head != NULL && t_queue.head->delay <= g_timer_count){
		ENVELOPE* cur = dequeue_env_queue(&t_queue);
		k_send_message (cur->destination_pid, (void *) cur);
		if (gp_pcbs[cur->destination_pid]->m_priority > gp_current_process->m_priority){
			preemption_flag = 1;
		}
	}
	send_message_preemption_flag = 1;
	g_timer_count++;
	__enable_irq();
	
	// TODO: figure how to switch out of this process
	
	if (preemption_flag){
		k_release_processor();
	}
}

void uart_i_proc(void) {
	uint8_t IIR_IntId;	    // Interrupt ID from IIR 		 
	LPC_UART_TypeDef *pUart = (LPC_UART_TypeDef *)LPC_UART0;
	ENVELOPE* msg;
	__disable_irq();
	
	/* Reading IIR automatically acknowledges the interrupt */
	IIR_IntId = (pUart->IIR) >> 1 ; // skip pending bit in IIR 
	
	if (IIR_IntId & IIR_RDA) { // Receive Data Available
		
		/* read UART. Read RBR will clear the interrupt */
		g_char_in = pUart->RBR;
		
		???uart_preemption_flag = 0;
#ifdef DEBUG_HOTKEYS		
		if (g_char_in == DEBUG_ //TODO
#endif			
	
		if (g_char_in != '\r') // Any char not an enter
		{
#ifdef DEBUG_HOTKEYS
			if (g_char_in != DEBUG_ //TODO
			{
				g_input_buffer[g_input_buffer_index] = g_char_in;
				g_input_buffer_index++;
			}
#endif
			g_input_buffer[g_input_buffer_index] = g_char_in;
			g_input_buffer_index++;
		}
		else // Enter is pressed
		{
			g_input_buffer[g_input_buffer_index] = '\0';
			
			// Avoid interuption by only sending when mem blocks are avaliable
			if (mem_empty() == 0)
			{
				msg = (ENVELOPE*) request_memory_block();
				msg->sender_pid = uart_iproc_pid;
				msg->destination_pid = receiver_pid;
				msg->nextMsg = NULL;
				msg->message_type = 0;
				msg->delay = 0;
				set_message(message, &msg, sizeof(char));	
			}
		}
		
		g_input_buffer[g_input_buffer_index] = g_char_in;

	} else if (IIR_IntId & IIR_THRE) {
	/* THRE Interrupt, transmit holding register becomes empty */

		if (*gp_buffer != '\0' ) {
			g_char_out = *gp_buffer;
#ifdef DEBUG_0
			
			// you could use the printf instead
			printf("Writing a char = %c \n\r", g_char_out);
#endif // DEBUG_0			
			pUart->THR = g_char_out;
			gp_buffer++;
		} else {
#ifdef DEBUG_0
			uart1_put_string("Finish writing. Turning off IER_THRE\n\r");
#endif // DEBUG_0
			pUart->IER ^= IER_THRE; // toggle the IER_THRE bit 
			pUart->THR = '\0';
			g_send_char = 0;
			gp_buffer = g_buffer;		
		}    
	} 
	__enable_irq();
	uart_preemption_flag = 1;
}

void kcd_proc(void) {}

void crt_proc(void) {}

