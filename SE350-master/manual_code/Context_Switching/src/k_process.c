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

#ifdef DEBUG_0
	#include "printf.h"
#endif /* DEBUG_0 */

/* ----- Global Variables ----- */
PCB **gp_pcbs; //array of pcb pointers
PCB_NODE **gp_pcb_nodes;  // actual pcb node array
PCB *gp_current_process = NULL; // always point to the current RUN process

/* Process Initialization Table */
PROC_INIT g_proc_table[NUM_TEST_PROCS + 1];
extern PROC_INIT g_test_procs[NUM_TEST_PROCS];

/* ----- Queue Declarations ----- */
QUEUE ready_priority_queue[5];
QUEUE blocked_priority_queue;

/**
 * The Null Process with priority 4
 */
void null_process() {
	while (1) {
		k_release_processor();
	}
}

/**
 * Enqueues the provided PCB node in the given queue
 */
void enqueue(QUEUE *q, PCB_NODE *n) {
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
int set_process_priority(int process_id, int priority){
	PCB_NODE * node = NULL;
	if (process_id < 1 || process_id > NUM_TEST_PROCS || priority < 0 || priority > 3){
		return RTX_ERR;
	}
	node = gp_pcb_nodes[process_id];

	if (remove(&ready_priority_queue[node->p_pcb->m_priority], node) != RTX_OK){
		return RTX_ERR;
	}
	
	node->p_pcb->m_priority = priority;
	enqueue(&ready_priority_queue[node->p_pcb->m_priority], node);

	return RTX_OK;
}

/**
 * Gets the process priority
 * Returns the process priority value or -1 if it does not find a process with the provide process ID
 */
int get_process_priority(int process_id){
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
	g_proc_table[0].m_priority = 4;
	g_proc_table[0].mpf_start_pc = &null_process;
	g_proc_table[0].m_stack_size = 0x100;
	
	// Setting the user processes in the initialization table
	for ( i = 1; i < NUM_TEST_PROCS + 1; i++ ) {
		g_proc_table[i].m_pid = g_test_procs[i-1].m_pid;
		g_proc_table[i].m_priority = g_test_procs[i-1].m_priority;
		g_proc_table[i].m_stack_size = g_test_procs[i-1].m_stack_size;
		g_proc_table[i].mpf_start_pc = g_test_procs[i-1].mpf_start_pc;
	}
  
	// initilize exception stack frame (i.e. initial context) for each process
	for ( i = 0; i < NUM_TEST_PROCS+1; i++ ) {
		int j;
		(gp_pcbs[i])->m_pid = (g_proc_table[i]).m_pid;
		(gp_pcbs[i])->m_state = NEW;
		(gp_pcbs[i])->m_priority = (g_proc_table[i]).m_priority;
		
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
	
	// Setting all queues to be empty
	for (i = 0; i < 5; i++){
		ready_priority_queue[i].head = NULL;
		ready_priority_queue[i].tail = NULL;
	}
	
	// Adding processes to the appropriate ready queue
	for (i = 0; i < NUM_TEST_PROCS+1; i++) {
		enqueue(&(ready_priority_queue[(gp_pcbs[i])->m_priority]), gp_pcb_nodes[i]);
	}

	// Setting everything in the blocked queue to be null
	blocked_priority_queue.head = NULL;
	blocked_priority_queue.tail = NULL;	
}

/**
 * Picks the process ID of the next to run process
 * Returns a PCB pointer of the next to run process or NULL if error occurs
 * If gp_current_process was NULL, then it gets set to pcbs[0]
 */
PCB *scheduler(void)
{
	int i = 0;

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
			p_pcb_old->m_state = RDY;
			p_pcb_old->mp_sp = (U32 *) __get_MSP();
		}
		gp_current_process->m_state = RUN;
		__set_MSP((U32) gp_current_process->mp_sp);
		__rte(); // pop exception stack frame from the stack for a new processes
	} 
	
	/* The following will only execute if the if block above is FALSE */
	
	if (gp_current_process != p_pcb_old) {
		if (state == RDY){ 		
			p_pcb_old->m_state = RDY; 
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
	process_switch(p_pcb_old);
	gp_current_process->m_state = RUN;
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
		PCB_NODE currPro;
		gp_current_process->m_state = BLOCKED;
		currPro.next = NULL;
		currPro.p_pcb = gp_current_process;
		enqueue(&blocked_priority_queue, &currPro);
	}
}

/**
 * Puts the first PCB node of the blocked queue to the ready queue
 * Marks the first PCB node of the blocked queue as ready
 */
void k_ready_first_blocked(void)
{
	int priority = 0;
	PCB_NODE* nowReady = dequeue(&blocked_priority_queue);
	(nowReady->p_pcb)->m_state = RDY;
	priority = nowReady->p_pcb->m_priority;
	enqueue(&ready_priority_queue[priority], nowReady);
	nowReady = NULL;
}
