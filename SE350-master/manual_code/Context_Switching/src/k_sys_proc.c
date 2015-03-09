#include <LPC17xx.h>
#include "string.h"
#include "k_sys_proc.h"
#include "k_rtx.h"
#include "k_ipc.h"
#include "k_memory.h"
#include "k_process.h"
#include "uart_def.h"
#include "printf.h"

ENV_QUEUE t_queue;
extern volatile uint32_t g_timer_count;
extern PCB* gp_current_process;
extern KC_LIST g_kc_reg[KC_MAX_COMMANDS];
int send_message_preemption_flag = 1; // 0 for not preempting and 1 otherwise

char g_input_buffer[INPUT_BUFFER_SIZE]; // buffer char array to hold the input
int g_input_buffer_index = 0; // current index of the buffer such that all indices before this one holds a char
U8 g_char_in;
U32 g_char_out_index = 0;
ENVELOPE* g_curr_p = NULL;

int k_delayed_send(int process_id, void * env, int delay){
	ENVELOPE *lope = (ENVELOPE *) env;
	int response = 0;
	__disable_irq();
	lope->delay = g_timer_count + delay;
	send_message_preemption_flag = 0;
	response = k_send_message(TIMER_PID, env);
	send_message_preemption_flag = 1;
	__enable_irq();
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

ENVELOPE* dequeue_env_queue(ENV_QUEUE *q){
	ENVELOPE *curHead = q->head;
	if (msg_empty(q)) return NULL;
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

ENVELOPE* k_non_blocking_receive_message(int pid){
	PCB* timer = gp_pcbs[pid];
	ENV_QUEUE* env_q = &(timer->env_q);
	if(!msg_empty(env_q)){
		return dequeue_env_queue(env_q);
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

void timer_i_proc(void) {
	ENVELOPE* lope = NULL;
	int preemption_flag = 0;
	__disable_irq(); // make this process non blocking
	
	// TODO: figure out the LPC_TIM0
	LPC_TIM0->IR = (1 << 0);
	
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
		
		// Avoid interuption by only sending when mem blocks are avaliable
		// Sends input to crt display
		if (mem_empty() == 0)
		{
			int display_size;
			char display_msg[3];
			if (g_char_in == '\r')
			{
				display_size = 3;
				display_msg[0] = '\n';
				display_msg[1] = g_char_in;
				display_msg[2] = '\0';
			}
			else
			{
				display_size = 2;
				display_msg[0] = g_char_in;
				display_msg[1] = '\0';
			}
			
			msg = (ENVELOPE*) k_request_memory_block();
			msg->sender_pid = UART_IPROC_PID;
			msg->destination_pid = CRT_PID;
			msg->nextMsg = NULL;
			msg->message_type = MSG_CRT_DISPLAY;
			msg->delay = 0;
			set_message(msg, display_msg, display_size*sizeof(char));	
			send_message(CRT_PID, msg);
			g_input_buffer_index = 0;
		}
		
#ifdef DEBUG_HOTKEYS		
		if (g_char_in == DEBUG_HOTKEY_1)
			k_print_ready_queue();
		else if (g_char_in == DEBUG_HOTKEY_2)
			k_print_blocked_on_memory_queue();
		else if (g_char_in == DEBUG_HOTKEY_3)
			k_print_blocked_on_receive_queue();
#endif			
	
		if (g_char_in != '\r') // Any char not an enter
		{
#ifdef DEBUG_HOTKEYS
			if ((g_char_in != DEBUG_HOTKEY_1)&&(g_char_in != DEBUG_HOTKEY_2)&&(g_char_in != DEBUG_HOTKEY_3))
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
			g_input_buffer_index++;
			
			// Avoid interuption by only sending when mem blocks are avaliable
			if (mem_empty() == 0)
			{
				msg = (ENVELOPE*) k_request_memory_block();
				msg->sender_pid = UART_IPROC_PID;
				msg->destination_pid = KCD_PID;
				msg->nextMsg = NULL;
				msg->message_type = MSG_CONSOLE_INPUT;
				msg->delay = 0;
				set_message(msg, g_input_buffer, g_input_buffer_index*sizeof(char));	
				send_message(KCD_PID, msg);
				g_input_buffer_index = 0;
			}
		}
		
		g_input_buffer[g_input_buffer_index] = g_char_in;

	} 
	else if (IIR_IntId & IIR_THRE) 
	{
			char* g_input;
			if (g_curr_p == NULL)
				g_curr_p = (ENVELOPE*) non_block_receive_message(NULL);
			if (g_curr_p != NULL)
			{
				g_input = (char*) g_curr_p->message;
				if (g_input[g_char_out_index] != '\0')
				{
					// print normal char
					pUart->THR = g_input[g_char_out_index];
					g_char_out_index++;
				}
				else
				{
					// done printing
					pUart->THR = g_input[g_char_out_index];
					k_release_memory_block(g_curr_p);
					g_curr_p = NULL;
					g_char_out_index = 0;
				}
			}
	}    
	__enable_irq();
}

void kcd_proc(void) 
{
	ENVELOPE* msg;
	int* sender;
	int i;
	while(1)
	{
		msg = (ENVELOPE*) receive_message(sender);
		if (msg != NULL)
		{
			if (msg->message_type == MSG_COMMAND_REGISTRATION)
			{
				for (i = 0; i < KC_MAX_COMMANDS; i++)
				{
					if (g_kc_reg[i].pid == -1)
					{
						g_kc_reg[i].pid = *sender;
						strcpy(msg->message, g_kc_reg[i].command);
						break;
					}
				}
			}
			else if (msg->message_type == MSG_CONSOLE_INPUT)
			{
				int i = 0;
				int j;
				char command[KC_MAX_CHAR];
				char* message_curr = msg->message;
				while ((i < KC_MAX_CHAR)&&(message_curr[i] != ' ')&&(message_curr[i] != '\0'))
				{
					command[i] = message_curr[i];
					i++;
				}
				command[i] = '\0';
				
				// find end of message 
				while (message_curr[i] != '\0')
				{
					i++;
				}
				
				for (j = 0; j < KC_MAX_COMMANDS; j++)
				{
					if (strcmp(command,g_kc_reg[j].command) == 0)
					{
						ENVELOPE* kcd_msg = (ENVELOPE*) k_request_memory_block();
						kcd_msg->sender_pid = KCD_PID;
						kcd_msg->destination_pid = g_kc_reg[j].pid;
						kcd_msg->nextMsg = NULL;
						kcd_msg->message_type = MSG_KCD_DISPATCH;
						kcd_msg->delay = 0;
						set_message(kcd_msg, message_curr, i*sizeof(char));	
						send_message(KCD_PID, kcd_msg);
						break;
					}
				}
				k_release_memory_block(msg);
			}
		}
	}
}

void crt_proc(void) 
{
	while(1){
		ENVELOPE* env = (ENVELOPE*) receive_message(NULL);
		if (env->message_type == MSG_CRT_DISPLAY){
			send_message(15, env);
			//pUart->IER |= IER_THRE;
		} else {
			k_release_memory_block(env->message);
		}
		
	}
}
