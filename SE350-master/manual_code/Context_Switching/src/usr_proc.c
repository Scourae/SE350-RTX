/**
 * @file:   usr_proc.c
 * @brief:  Two user processes: proc1 and proc2
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
	g_test_procs[4].m_priority=LOWEST;
	g_test_procs[4].m_stack_size=0x100;
	
	g_test_procs[5].m_pid=(U32)(6);
	g_test_procs[5].m_priority=LOWEST;
	g_test_procs[5].m_stack_size=0x100;
  
	g_test_procs[0].mpf_start_pc = &send_message_test;
	g_test_procs[1].mpf_start_pc = &receive_message_test;
	g_test_procs[2].mpf_start_pc = &send_message_to_blocked;
	g_test_procs[3].mpf_start_pc = &receive_message_to_blocked;
	g_test_procs[4].mpf_start_pc = &proc5;
	g_test_procs[5].mpf_start_pc = &proc6;
}

void proc5(void){
	while (1)
	{
		release_processor();
	}	
}

/**
 * Change priority of current process
 */
void proc6(void)
{
	while (1)
	{
		release_processor();
	}	
}

// Assuming pid 1
void send_message_test(void)
{
	int sender_pid = 1;
	int receiver_pid = 2;
	ENVELOPE* message = (ENVELOPE*) request_memory_block();
	char msg = 'x';
	int result;
	message->sender_pid = sender_pid;
	message->destination_pid = receiver_pid;
	message->nextMsg = NULL;
	message->message_type = 0;
	message->delay = 0;
	set_message(message, &msg, sizeof(char));
	result = delayed_send(receiver_pid, message, 5000);
	//result = send_message(receiver_pid, message);
	// Change this depending on the pid of this test
	if (result == 0)
	{
		uart0_put_string("G009_test: test 1 OK\n\r");
		passed++;
	}
	else
		uart0_put_string("G009_test: test 1 FAIL\n\r");
	
	set_process_priority(receiver_pid, 0);
	release_memory_block(message);
	set_process_priority(sender_pid, 3);
	while (1)
	{
		release_processor();
	}	
}

// Assuming pid 2
void receive_message_test(void)
{
	int sender_pid = 1;
	int receiver_pid = 2;
	ENVELOPE* message = receive_message(&sender_pid);
	char* char_message = (char*) message->message;
	// Change this depending on the pid of this test
	if (*char_message == 'x') 
	{
		uart0_put_string("G009_test: test 2 OK\n\r");
		passed++;
	}
	else
	{
		uart0_put_string("G009_test: test 2 FAIL\n\r");
	}
	set_process_priority(sender_pid, 0);
	set_process_priority(receiver_pid, 3);
	while (1)
	{
		release_processor();
	}	
}

// Assuming pid 3
void send_message_to_blocked(void)
{
	int sender_pid = 3;
	int receiver_pid = 4;
	ENVELOPE* message = (ENVELOPE*) request_memory_block();
	char msg = 'x';
	int result;
	set_process_priority(receiver_pid, 0);
	message->sender_pid = sender_pid;
	message->destination_pid = receiver_pid;
	message->nextMsg = NULL;
	message->message_type = 0;
	message->delay = 0;
	set_message(message, &msg, sizeof(char));
	result = send_message(receiver_pid, message);
	
	// Change this depending on the pid of this test
	if (result == 0)
	{
		uart0_put_string("G009_test: test 3 OK\n\r");
		passed++;
	}
	else
		uart0_put_string("G009_test: test 3 FAIL\n\r");
	
	set_process_priority(sender_pid, 3);
	release_memory_block(message);
	set_process_priority(sender_pid, 3);
	while (1)
	{
		release_processor();
	}	
}

// Assuming pid 4
void receive_message_to_blocked(void)
{
	int sender_pid = 3;
	int receiver_pid = 4;
	ENVELOPE* message = receive_message(&sender_pid);
	char* char_message = (char*) message->message;
	// Change this depending on the pid of this test
	if (*char_message == 'x') 
	{
		uart0_put_string("G009_test: test 4 OK\n\r");
		passed++;
	}
	else
	{
		uart0_put_string("G009_test: test 4 FAIL\n\r");
	}
	set_process_priority(sender_pid, 0);
	set_process_priority(receiver_pid, 3);
	while (1)
	{
		release_processor();
	}	
}
