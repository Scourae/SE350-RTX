/**
 * @file:   usr_proc.c
 * @brief:  Proc5 and Prc6 are used for timing analysis
 * @author: Yiqing Huang
 * @date:   2014/01/17
 * NOTE: Each process is in an infinite loop. Processes never terminate.
 */

#include "rtx.h"
#include "k_ipc.h"
#include "uart_polling.h"
#include "usr_proc.h"

#ifdef DEBUG_0
#include "printf.h"
#endif /* DEBUG_0 */

extern volatile uint32_t g_timer_count;
extern volatile uint32_t* function_timer;

/* initialization table item */
PROC_INIT g_test_procs[NUM_TEST_PROCS];
int passed = 0;

void * memory_blocks[20];

void set_test_procs() {
	g_test_procs[0].m_pid=(U32)(1);
	g_test_procs[0].m_priority=LOW;
	g_test_procs[0].m_stack_size=0x100;

	g_test_procs[1].m_pid=(U32)(2);
	g_test_procs[1].m_priority=LOW;
	g_test_procs[1].m_stack_size=0x100;
	
	g_test_procs[2].m_pid=(U32)(3);
	g_test_procs[2].m_priority=LOW;
	g_test_procs[2].m_stack_size=0x100;

	g_test_procs[3].m_pid=(U32)(4);
	g_test_procs[3].m_priority=LOW;
	g_test_procs[3].m_stack_size=0x100;
	
	g_test_procs[4].m_pid=(U32)(5);
	g_test_procs[4].m_priority=HIGH;
	g_test_procs[4].m_stack_size=0x100;
	
	g_test_procs[5].m_pid=(U32)(6);
	g_test_procs[5].m_priority=LOW;
	g_test_procs[5].m_stack_size=0x100;
  
	g_test_procs[0].mpf_start_pc = &send_delayed_message;
	g_test_procs[1].mpf_start_pc = &receive_delayed_message;
	g_test_procs[2].mpf_start_pc = &receive_delayed_message_preemption;
	g_test_procs[3].mpf_start_pc = &send_message_to_blocked;
	g_test_procs[4].mpf_start_pc = &receive_message_to_blocked;
	g_test_procs[5].mpf_start_pc = &request_all_memory_block;
}

/* 

Timing anaylsis
Proc 5: experiment 1
Proc 6: experiment 2

*/

// Assuming pid 1
void send_delayed_message(void)
{
	while (1)
	{
			release_processor();
	}	
}



// Assuming pid 2
void receive_delayed_message(void)
{
	while (1)
	{
		release_processor();
	}	
}

// Assuming pid 3
void receive_delayed_message_preemption(void)
{
	while (1)
	{
		release_processor();
	}	
}



// Assuming pid 4
void send_message_to_blocked(void)
{
	while (1)
	{
		release_processor();
	}	
}

// Assuming pid 5
void receive_message_to_blocked(void)
{
	int start;
	int finish;
	int request;
	int send;
	int receive;
	int i;
	char msg = 'x';
	int result;
	ENVELOPE* message;
	
	start = 0;
	finish = 0;
	printf("Proc 5\n\r");
	for (i = 0; i < 29; i++){
		start = *function_timer;
		message = (ENVELOPE*) request_memory_block();
		finish = *function_timer;
		request = finish - start;
		
		message->sender_pid = 6;
		message->destination_pid = 6;
		message->nextMsg = NULL;
		message->message_type = 0;
		message->delay = 0;
		set_message(message, &msg, sizeof(char));

		start = *function_timer;
		result = send_message(5, message);
		finish = *function_timer;
		send = finish - start;
		
		start = *function_timer;
		message = receive_message(NULL);
		finish = *function_timer;
		receive = finish - start;
		release_memory_block(message);
		printf("%d,%d,%d\n\r", request, send, receive);
	}
	set_process_priority(5, LOWEST);
	while (1)
	{
		release_processor();
	}	
}

// Assuming pid 6
void request_all_memory_block(void)
{
	int start;
	int finish;
	int request;
	int send;
	int receive;
	int i;
	char msg = 'x';
	int result;
	ENVELOPE* message;
	
	start = 0;
	finish = 0;
	printf("Proc 6\n\r");
	for (i = 0; i < 29; i++){
		start = *function_timer;
		message = (ENVELOPE*) request_memory_block();
		finish = *function_timer;
		request = finish - start;
		
		message->sender_pid = 6;
		message->destination_pid = 6;
		message->nextMsg = NULL;
		message->message_type = 0;
		message->delay = 0;
		set_message(message, &msg, sizeof(char));

		start = *function_timer;
		result = send_message(6, message);
		finish = *function_timer;
		send = finish - start;
		
		start = *function_timer;
		message = receive_message(NULL);
		finish = *function_timer;
		receive = finish - start;
		printf("%d,%d,%d\n\r", request, send, receive);
	}
	while (1)
	{
		release_processor();
	}	
}
