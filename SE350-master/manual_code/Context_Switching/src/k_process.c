/**
 * @file:   k_process.c  
 * @brief:  process management C file
 * @author: Angad Kashyap, Vishal Bollu, Mike Wang and Tong Tong
 * @author: Yiqing Huang and Thomas Reidemeister
 * @date:   2014/01/17
 * NOTE: Implementing context switching.
 *       The code only has minimal sanity check. There is no stack overflow check.
 *       The implementation assumes six user processes and NO HARDWARE INTERRUPTS.
 */

#include <LPC17xx.h>
#include <system_LPC17xx.h>
#include "uart_polling.h"
#include "k_process.h"
#include "k_sys_proc.h"
#include "k_usr_proc.h"

/* ----- Global Variables ----- */
PCB **gp_pcbs = NULL; //array of pcb pointers
PCB_NODE **gp_pcb_nodes = NULL;  // actual pcb node array
PCB *gp_current_process = NULL; // always point to the current RUN process

/* Process Initialization Table */
PROC_INIT g_proc_table[NUM_PROCS];
int uart_preemption_flag = 0;

extern PROC_INIT g_test_procs[NUM_TEST_PROCS];

/* ----- Queue Declarations ----- */
QUEUE ready_priority_queue[6];
QUEUE blocked_on_memory_queue[6];
PCB_NODE* blocked_on_receive_list = NULL;

KC_LIST g_kc_reg[KC_MAX_COMMANDS];
/**
 * Gets the process priority
 * Returns the process priority value or -1 if it does not find a process with the provide process ID
 */
int k_get_process_priority(int process_id){
	PCB_NODE *node = gp_pcb_nodes[process_id];
	if (node == NULL){
		return RTX_ERR;
	}
	return node->p_pcb->m_priority;
}
	
/**
 * Initialize all processes in the system
 */
void process_init() 
{
	int i;
	U32 *sp;
	
  /* Fill out the initialization table */
	set_test_procs();
	
	// Setting the Null Process in the initialization table
	g_proc_table[0].m_pid = 0;
	g_proc_table[0].m_priority = NULL_PROC;
	g_proc_table[0].mpf_start_pc = &null_proc;
	g_proc_table[0].m_stack_size = 0x100;
	
	// Setting the stress_test_a Process in the initialization table
	g_proc_table[7].m_pid = 7;
	g_proc_table[7].m_priority = HIGH;
	g_proc_table[7].mpf_start_pc = &stress_test_a;
	g_proc_table[7].m_stack_size = 0x100;
	
	// Setting the stress_test_b Process in the initialization table
	g_proc_table[8].m_pid = 8;
	g_proc_table[8].m_priority = HIGH;
	g_proc_table[8].mpf_start_pc = &stress_test_b;
	g_proc_table[8].m_stack_size = 0x100;
	
	// Setting the stress_test_c Process in the initialization table
	g_proc_table[9].m_pid = 9;
	g_proc_table[9].m_priority = HIGH;
	g_proc_table[9].mpf_start_pc = &stress_test_c;
	g_proc_table[9].m_stack_size = 0x100;
	
	// Setting the set_priority_proc Process in the initialization table
	g_proc_table[10].m_pid = 10;
	g_proc_table[10].m_priority = SYS_PROC;
	g_proc_table[10].mpf_start_pc = &set_priority_proc;
	g_proc_table[10].m_stack_size = 0x100;
	
	// Setting the wall_clock_display Process in the initialization table
	g_proc_table[11].m_pid = 11;
	g_proc_table[11].m_priority = HIGH;
	g_proc_table[11].mpf_start_pc = &wall_clock_proc;
	g_proc_table[11].m_stack_size = 0x100;
	
	// Setting the kcd_proc Process in the initialization table
	g_proc_table[12].m_pid = 12;
	g_proc_table[12].m_priority = SYS_PROC;
	g_proc_table[12].mpf_start_pc = &kcd_proc;
	g_proc_table[12].m_stack_size = 0x100;
	
	// Setting the crt_proc Process in the initialization table
	g_proc_table[13].m_pid = 13;
	g_proc_table[13].m_priority = SYS_PROC;
	g_proc_table[13].mpf_start_pc = &crt_proc;
	g_proc_table[13].m_stack_size = 0x100;
	
	// Setting the timer_i_proc Process in the initialization table
	g_proc_table[14].m_pid = 14;
	g_proc_table[14].m_priority = SYS_PROC;
	g_proc_table[14].mpf_start_pc = &timer_i_proc;
	g_proc_table[14].m_stack_size = 0x100;
	
	// Setting the uart_i_proc Process in the initialization table
	g_proc_table[15].m_pid = 15;
	g_proc_table[15].m_priority = SYS_PROC;
	g_proc_table[15].mpf_start_pc = &uart_i_proc;
	g_proc_table[15].m_stack_size = 0x100;
	
	// Setting the user processes in the initialization table
	for ( i = 1; i < 7; i++ ) {
		g_proc_table[i].m_pid = g_test_procs[i-1].m_pid;
		g_proc_table[i].m_priority = g_test_procs[i-1].m_priority;
		g_proc_table[i].m_stack_size = g_test_procs[i-1].m_stack_size;
		g_proc_table[i].mpf_start_pc = g_test_procs[i-1].mpf_start_pc;
	}
  
	// initilize exception stack frame (i.e. initial context) for each process
	for ( i = 0; i < NUM_PROCS; i++ ) {
		int j;
		(gp_pcbs[i])->m_pid = (g_proc_table[i]).m_pid;
		(gp_pcbs[i])->m_state = NEW;
		(gp_pcbs[i])->m_priority = (g_proc_table[i]).m_priority;
		
		// Message queue init
		(gp_pcbs[i])->env_q.head = NULL;
		(gp_pcbs[i])->env_q.tail = NULL;
		
		sp = alloc_stack((g_proc_table[i]).m_stack_size);
		*(--sp)  = INITIAL_xPSR; // user process initial xPSR  
		*(--sp)  = (U32)((g_proc_table[i]).mpf_start_pc); // PC contains the entry point of the process
		for ( j = 0; j < 6; j++ ) { // R0-R3, R12 are cleared with 0
			*(--sp) = 0x0;
		}
		(gp_pcbs[i])->mp_sp = sp;
		
		(gp_pcb_nodes[i])->next = NULL;
		(gp_pcb_nodes[i])->p_pcb = gp_pcbs[i];
	}
	
	// Setting all ready queues to be empty
	for (i = 0; i < 6; i++){
		ready_priority_queue[i].head = NULL;
		ready_priority_queue[i].tail = NULL;
	}
	
	// Adding the processes to the appropriate ready queue
	for (i = 0; i <= 13; i++) {
		enqueue(&(ready_priority_queue[(gp_pcbs[i])->m_priority]), gp_pcb_nodes[i]);
	}

	// Setting everything in the blocked queue to be null
	blocked_on_receive_list = NULL;
	
	// Setting blocked on memory queues to be empty
	for (i = 0; i < 6; i++){
		blocked_on_memory_queue[i].head = NULL;
		blocked_on_memory_queue[i].tail = NULL;
	}
	
	// Keyboard commands initialization
	for (i = 0; i < KC_MAX_COMMANDS; i++)
	{
		g_kc_reg[i].command[0] = '\0';
		g_kc_reg[i].pid = -1;
	}
}

/**
 * Enqueues the provided PCB node in the given queue
 */
void enqueue(QUEUE *q, PCB_NODE *n) {
	n->next = NULL;
	if (q->head == NULL)
	{
		q->head = n;
		q->tail = n;
	}
	else
	{
		q->tail->next = n;
		q->tail = q->tail->next;
	}
}

/**
 * Dequeues the head of the queue
 * Returns a pointer to the dequeued PCB node
 */
PCB_NODE* dequeue(QUEUE *q) {
	PCB_NODE *curHead = q->head;
	if (q->head == q->tail)
	{
		q->head = NULL;
		q->tail = NULL;
	}
	else
		q->head = q->head->next;
	curHead->next = NULL;
	return curHead;
}

/**
 * Peeks at the head of the queue
 * Returns a pointer to the PCB node
 */
PCB_NODE* peek(QUEUE *q) {
	return q->head;
}

/**
 * Removes the provided PCB node in the given queue
 * Returns -1 or 0 corresponding to failure or success in removal
 */
int remove (QUEUE *q, PCB_NODE * n){
	if (q->head == n){
		if (q->head == q->tail){
			q->tail = NULL;	
		}
		q->head = n->next;
		return RTX_OK;
	}
	else {
		PCB_NODE * prev = q->head;
		PCB_NODE * cur = q->head->next;
		while (cur  != NULL){
			if (cur == n){
				/* our changes */
				if(cur->next == NULL){
					q->tail = prev;
				}
				/*****/
				prev->next = cur->next;
				cur->next = NULL;
				return RTX_OK;
			}
			else {
				cur = cur->next;
				prev = prev->next;
			}
		}
	}
	return RTX_ERR;
}

/**
 * Checks whether the provided queue is empty
 * Returns 1 if queue is empty or 0 otherwise
 */
int isEmpty(QUEUE *q) {
	return (q->head == NULL);
}

/**
 * Sets the process priority
 * Returns -1 if it fails or 0 otherwise
 */
int k_set_process_priority(int process_id, int priority){
	PCB_NODE * node = NULL;
	if (process_id < 1 || process_id > NUM_TEST_PROCS || priority < 0 || priority > 3){
		return RTX_ERR;
	}
	node = gp_pcb_nodes[process_id];
	
	if(node->p_pcb->m_priority != priority){
		if (node->p_pcb->m_state != BLOCKED_ON_MEMORY && node->p_pcb != gp_current_process){
			if (remove(&ready_priority_queue[node->p_pcb->m_priority], node) != RTX_OK){
				return RTX_ERR;
			}
			node->p_pcb->m_priority = priority;
			enqueue(&ready_priority_queue[node->p_pcb->m_priority], node);
		}

		node->p_pcb->m_priority = priority;
		//uart0_put_string("priority set\n\r");
		k_release_processor();
	}
	
	return RTX_OK;
}

/**
 * Picks the process ID of the next to run process
 * Returns a PCB pointer of the next to run process or NULL if error occurs
 * If gp_current_process was NULL, then it gets set to pcbs[0]
 */
PCB *scheduler(void)
{
	int i = 0;
	if (uart_preemption_flag == 1){
		uart_preemption_flag = 0;
		return gp_pcbs[UART_IPROC_PID];
	}
	// Checks if there are any system processes ready to run
	if(!isEmpty(&ready_priority_queue[SYS_PROC])){
		// Returns any system processes first
		return dequeue(&ready_priority_queue[SYS_PROC])->p_pcb;
	}
	
	// Then checks the user procs (last/default is null process)
	for (i = 0; i < 5; i++){
		if(!isEmpty(&ready_priority_queue[i])){
			// Returns the PCB of the highest priority
			return dequeue(&ready_priority_queue[i])->p_pcb;
		}
	}
	return NULL;
}

/**
 * Switches out the old pcb (p_pcb_old) and runs the new pcb (gp_current_process)
 * Returns 0 upon success or -1 upon failure
 * If gp_current_process was NULL, then it gets set to pcbs[0]
 */
int process_switch(PCB *p_pcb_old) 
{
	PROC_STATE_E state = gp_current_process->m_state;
	
	if (state == NEW) {
		if (gp_current_process != p_pcb_old && p_pcb_old->m_state != NEW) {
			//p_pcb_old->m_state = RDY;
			p_pcb_old->mp_sp = (U32 *) __get_MSP();
		}
		gp_current_process->m_state = RUN;
		__set_MSP((U32) gp_current_process->mp_sp);
		__rte(); // pop exception stack frame from the stack for a new processes
	} 
	
	/* The following will only execute if the if block above is FALSE */
	
	if (gp_current_process != p_pcb_old) {
		if (state == RDY){ 		
			//p_pcb_old->m_state = RDY; 
			p_pcb_old->mp_sp = (U32 *) __get_MSP(); // save the old process's sp
			gp_current_process->m_state = RUN;
			__set_MSP((U32) gp_current_process->mp_sp); //switch to the new proc's stack    
		} else {
			gp_current_process = p_pcb_old; // revert back to the old proc on error
			return RTX_ERR;
		}
	}
	
	if(gp_current_process == NULL){
		gp_current_process = gp_pcbs[0];
	}
	return RTX_OK;
}

/**
 * Releases the processor
 * Returns -1 on error or 0 on success
 * gp_current_process gets updated to the next process to run
 */
int k_release_processor(void)
{
	PCB *p_pcb_old = NULL;
	
	//uart0_put_string("in release processor\n\r");
	
	p_pcb_old = gp_current_process;
	if (gp_current_process != NULL)
	{
		if(gp_current_process->m_state == RUN){
				gp_current_process->m_state = RDY;
				enqueue(&ready_priority_queue[gp_current_process->m_priority], gp_pcb_nodes[gp_current_process->m_pid]);
		}
	}
	gp_current_process = scheduler();
	
	if ( gp_current_process == NULL  ) {
		gp_current_process = p_pcb_old; // revert back to the old process
		return RTX_ERR;
	}
  if ( p_pcb_old == NULL ) {
		p_pcb_old = gp_current_process;
	}
	
	//uart0_put_string("going to process switch\n\r");
	
	process_switch(p_pcb_old);
	gp_current_process->m_state = RUN;
	
	//uart0_put_string("done process switching\n\r");
	
	return RTX_OK;
}

/**
 *	Puts the current process into the blocked queue
 *  Marks the current process as blocked
 */
void k_block_current_processs(void)
{
	if (gp_current_process)
	{
		PCB_NODE* currPro = gp_pcb_nodes[gp_current_process->m_pid];
		gp_current_process->m_state = BLOCKED_ON_MEMORY;
		currPro->next = NULL;
		enqueue(&blocked_on_memory_queue[currPro->p_pcb->m_priority], currPro);
	}
}

/**
 * Puts the first PCB node of the blocked queue to the ready queue
 * Marks the first PCB node of the blocked queue as ready
 */
int k_ready_first_blocked(void)
{
	int i;
	
	if(!isEmpty(&blocked_on_memory_queue[SYS_PROC])){
		int priority = 0;
		PCB_NODE* nowReady = dequeue(&blocked_on_memory_queue[SYS_PROC]);
		(nowReady->p_pcb)->m_state = RDY;
		priority = nowReady->p_pcb->m_priority;
		enqueue(&ready_priority_queue[priority], nowReady);
		return priority;
	}
	
	for (i = 0; i < 5; i++){
		if (!isEmpty(&blocked_on_memory_queue[i])){
			int priority = 0;
			PCB_NODE* nowReady = dequeue(&blocked_on_memory_queue[i]);
			(nowReady->p_pcb)->m_state = RDY;
			priority = nowReady->p_pcb->m_priority;
			enqueue(&ready_priority_queue[priority], nowReady);
			return priority;
		}
	}
	return -1;
}

void k_ready_process(int pid)
{
	PCB_NODE* currPro = gp_pcb_nodes[pid];
	gp_pcb_nodes[pid]->p_pcb->m_state = RDY;
	currPro->next = NULL;
	enqueue(&ready_priority_queue[currPro->p_pcb->m_priority], currPro);
}

PCB* k_get_current_process()
{
	return gp_current_process;
}

// HotKey #1: printing to the RTX system debug terminal all the procs currently on the ready queue
void k_print_ready_queue()
{
	int i = 0;
	char num = '0';
	uart1_put_string("\n\r\n\r----- PROCESSES CURRENTLY IN READY QUEUE -----\n\r\n\r");
	uart1_put_string("Current running process with PID ");
	uart1_put_char(num+gp_current_process->m_pid);
	uart1_put_string("\n\r");
	
	for (i = 0; i < 4; i++){
		if(!isEmpty(&ready_priority_queue[i])){
			PCB_NODE* cur = ready_priority_queue[i].head;
			uart1_put_string("\n\rPriority ");
			uart1_put_char(num+i);
			uart1_put_string(":\n\r");
			
			while(cur != NULL){
				uart1_put_string("\t Process with PID ");
				uart1_put_char(num+cur->p_pcb->m_pid);
				uart1_put_string("\n\r");
				cur = cur->next;
			}
		}
	}
	
	if(!isEmpty(&ready_priority_queue[SYS_PROC])){
		PCB_NODE* cur = ready_priority_queue[SYS_PROC].head;
		uart1_put_string("\n\rSystem Priority:\n\r");
		
		while(cur != NULL){
			uart1_put_string("\t Process with PID ");
			uart1_put_char(num+cur->p_pcb->m_pid);
			uart1_put_string("\n\r");
			cur = cur->next;
		}
	}
}

// HotKey #2: printing to the RTX system debug terminal all the procs currently on the blocked on memory queue
void k_print_blocked_on_memory_queue()
{
	int i = 0;
	char num = '0';
	uart1_put_string("\n\r\n\r----- PROCESSES CURRENTLY IN BLOCKED ON MEMORY QUEUE -----\n\r");
	
	for (i = 0; i < 4; i++){
		if(!isEmpty(&blocked_on_memory_queue[i])){
			PCB_NODE* cur = blocked_on_memory_queue[i].head;
			uart1_put_string("\n\rPriority ");
			uart1_put_char(num+i);
			uart1_put_string(":\n\r");
			
			while(cur != NULL){
				uart1_put_string("\t Process with PID ");
				uart1_put_char(num+cur->p_pcb->m_pid);
				uart1_put_string("\n\r");
				cur = cur->next;
			}
		}
	}
	
	if(!isEmpty(&blocked_on_memory_queue[SYS_PROC])){
		PCB_NODE* cur = blocked_on_memory_queue[SYS_PROC].head;
		uart1_put_string("\n\rSystem Priority:\n\r");
		
		while(cur != NULL){
			uart1_put_string("\t Process with PID ");
			uart1_put_char(num+cur->p_pcb->m_pid);
			uart1_put_string("\n\r");
			cur = cur->next;
		}
	}
}
