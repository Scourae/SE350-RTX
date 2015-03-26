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

extern volatile uint32_t g_timer_count;

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
	g_test_procs[4].m_priority=LOW;
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

Expected behaviour:
process 1 sends delayed message to process 2 and process 3
preempt process 2 by making it the highest
process 2 becomes blocked on receive
goes to process 3
process 3 becomes blocked on receive
looping 1 until 3 becomes unblocked
3 becomes unblocked
alternating between 1 and 3 until 2 becomes unblocked
2 becomes unblocked
looping 2 forever

*/

// Assuming pid 1
void send_delayed_message(void)
{
	int sender_pid = 1;
	int receiver_pid = 3;
	ENVELOPE* message = (ENVELOPE*) request_memory_block();
	char msg2 = 'x';
	char msg3 = 'y';
	int result1, result2;
	message->sender_pid = sender_pid;
	message->destination_pid = receiver_pid;
	message->nextMsg = NULL;
	message->message_type = 0;
	message->delay = 0;
	set_message(message, &msg2, sizeof(char));
	result1 = delayed_send(receiver_pid, message, 6000);
	
	receiver_pid = 2;
	message = (ENVELOPE*) request_memory_block();
	message->sender_pid = sender_pid;
	message->destination_pid = receiver_pid;
	message->nextMsg = NULL;
	message->message_type = 0;
	message->delay = 0;
	set_message(message, &msg3, sizeof(char));
	result2 = delayed_send(receiver_pid, message, 4000);
	if (result1 == 0 && result2 == 0)
	{
		uart0_put_string("G009_test: test 1 OK\n\r");
		passed++;
	}
	else
		uart0_put_string("G009_test: test 1 FAIL\n\r");
	
	set_process_priority(3, 0);
	while (1)
	{
			release_processor();
	}	
}



// Assuming pid 2
void receive_delayed_message(void)
{
	ENVELOPE* message = receive_message(NULL);
	char* char_message = (char*) message->message;
	if (*char_message == 'y') 
	{
		uart0_put_string("G009_test: test 2 OK\n\r");
		passed++;
	}
	else
	{
		uart0_put_string("G009_test: test 2 FAIL\n\r");
	}
	release_memory_block(message);
	set_process_priority(2, LOWEST);
	while (1)
	{
		release_processor();
	}	
}

// Assuming pid 3
void receive_delayed_message_preemption(void)
{
	ENVELOPE* message = receive_message(NULL);
	char* char_message = (char*) message->message;
	
	if (*char_message == 'x') 
	{
		uart0_put_string("G009_test: test 3 OK\n\r");
		passed++;
	}
	else
	{
		uart0_put_string("G009_test: test 3 FAIL\n\r");
	}
	release_memory_block(message);
	set_process_priority(1, LOWEST);
	set_process_priority(3, LOWEST);
	
	while (1)
	{
		release_processor();
	}	
}



// Assuming pid 4
void send_message_to_blocked(void)
{
	int sender_pid = 4;
	int receiver_pid = 5;
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
		uart0_put_string("G009_test: test 4 OK\n\r");
		passed++;
	}
	else
		uart0_put_string("G009_test: test 4 FAIL\n\r");
	
	set_process_priority(sender_pid, 3);
	release_memory_block(message);
	set_process_priority(sender_pid, 3);
	while (1)
	{
		release_processor();
	}	
}

// Assuming pid 5
void receive_message_to_blocked(void)
{
	int sender_pid = 4;
	int receiver_pid = 5;
	ENVELOPE* message = receive_message(NULL);
	char* char_message = (char*) message->message;
	// Change this depending on the pid of this test
	if (*char_message == 'x') 
	{
		uart0_put_string("G009_test: test 5 OK\n\r");
		passed++;
	}
	else
	{
		uart0_put_string("G009_test: test 5 FAIL\n\r");
	}
	set_process_priority(sender_pid, 0);
	set_process_priority(receiver_pid, 3);
	while (1)
	{
		release_processor();
	}	
}

// Assuming pid 6
void request_all_memory_block(void)
{
	int i = 0;
	char failed;
	char success;
	void *lope [32];
	while (i < 32){
		lope[i] = request_memory_block();
		i++;
	}
	if (i == 32 && lope != NULL)
	{
		uart0_put_string("G009_test: test 6 OK\n\r");
		passed++;
	}
	else
	{
		uart0_put_string("G009_test: test 6 FAIL\n\r");
	}
	i--;
	while (i >= 0){
		release_memory_block(lope[i]);
		i--;
	}
	failed = '0' + (6 - passed);
	success = '0' + passed;
	uart0_put_string("G009_test: ");
	uart0_put_char(success);
  uart0_put_string("/6 tests OK\n\r");
	uart0_put_string("G009_test: ");
	uart0_put_char(failed);
  uart0_put_string("/6 tests FAILED\n\r");
	set_process_priority(6, LOWEST);
	set_process_priority(4, LOWEST);
	while (1)
	{
		release_processor();
	}	
}
