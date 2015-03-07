/**
 * @file:   usr_proc.c
 * @brief:  Two user processes: proc1 and proc2
 * @author: Yiqing Huang
 * @date:   2014/01/17
 * NOTE: Each process is in an infinite loop. Processes never terminate.
 */

#include "rtx.h"
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
	g_test_procs[4].m_priority=LOW;
	g_test_procs[4].m_stack_size=0x100;
	
	g_test_procs[5].m_pid=(U32)(6);
	g_test_procs[5].m_priority=LOW;
	g_test_procs[5].m_stack_size=0x100;
  
	g_test_procs[0].mpf_start_pc = &proc1;
	g_test_procs[1].mpf_start_pc = &proc2;
	g_test_procs[2].mpf_start_pc = &proc3;
	g_test_procs[3].mpf_start_pc = &proc4;
	g_test_procs[4].mpf_start_pc = &proc5;
	g_test_procs[5].mpf_start_pc = &proc6;
}


/**
 * @brief: a process that switches to another one
 */
void proc1(void)
{
	int i = 0;
	uart0_put_string("G009_test: START\n\r");
	uart0_put_string("G009_test: total 6 tests\n\r");
	set_process_priority(2, 0);
	while (i != 2) {
		uart0_put_string("back to process 1");
		set_process_priority(2, 0);
		set_process_priority(1, 3);
		i++;
	}
	uart0_put_string("G009_test: test 1 OK\n\r");
	passed++;
	set_process_priority(1, 3);
	set_process_priority(2, 0);
	while (1)
	{
		uart0_put_string("In process 1\n\r");
		release_processor();
	}	
}

/**
 * @brief: a process that alternates with the first one
 */
void proc2(void)
{
	int i = 0;
	uart0_put_string("start process 2\n\r");
	while (i != 2) {
		uart0_put_string("back to process 2");
		set_process_priority(1, 0); 
		set_process_priority(2, 3);
		i++;
	}
	uart0_put_string("G009_test: test 2 OK\n\r");
	passed++;
	set_process_priority(1, 0);
	set_process_priority(2, 3);
	while (1)
	{
		uart0_put_string("In process 2\n\r");
		release_processor();
	}	
}

/**
 * Process that aquires one block of memory and prints it
 */
 void proc3(void)
 {
	void * pointer;
	int status;
	set_process_priority(3, HIGH);
	pointer = request_memory_block();
	status = release_memory_block(pointer);
	if (status == 0)
	{
		uart0_put_string("G009_test: test 3 OK\n\r");
		passed++;
	}
	else
	{
		 uart0_put_string("G009_test: test 3 FAIL\n\r");
	}
	set_process_priority(3, 3);
	while (1)
	{
		release_processor();
	}	
}

void proc4(void){
	void* temp;
	int i = 0;
	for(i = 0; i < 20; i++){
		memory_blocks[i] = request_memory_block();
	}
	set_process_priority(5, HIGH);
	set_process_priority(4, HIGH);
	temp = request_memory_block();
	passed++;
	uart0_put_string("G009_test: test 4 OK\n\r");
	release_memory_block(temp);
	set_process_priority(4, LOWEST);
	while (1)
	{
		release_processor();
	}	
}

void proc5(void){
	int result;
	void * temp;
	int i = 0;
	for(i = 0; i < 20; i++){
		result = release_memory_block(memory_blocks[i]);
		if (result != 0)
		{
			uart0_put_string("G009_test: test 5 FAIL\n\r");
			set_process_priority(5, LOWEST);
			set_process_priority(4, LOWEST);
		}
	}
	temp = request_memory_block();
	passed++;
	uart0_put_string("G009_test: test 5 OK\n\r");
	release_memory_block(temp);
	set_process_priority(5, LOWEST);
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
	int prioSwitch = 0;
	int pid = 6;
	int currPrio;
	int failed = 0;
	char result;
	char failure;
	set_process_priority(pid, prioSwitch);
	currPrio = get_process_priority(pid);
	if (prioSwitch != currPrio)
	{
		failed = 1;
		uart0_put_string("G009_test: test 6 FAIL\n\r");
	}
	if (failed == 0)
	{
		uart0_put_string("G009_test: test 6 OK\n\r");
		passed++;
	}
	set_process_priority(pid, 3);
	result = '0' + passed;
	failure = '0' + 6-passed;
	uart0_put_string("G009_test: ");
	uart0_put_char(result);
	uart0_put_string("/6 tests OK\n\r");
	uart0_put_string("G009_test: ");
	uart0_put_char(failure);
	uart0_put_string("/6 tests FAIL\n\r");
	uart0_put_string("G009_test: END\n");
	while (1)
	{
		release_processor();
	}	
}
