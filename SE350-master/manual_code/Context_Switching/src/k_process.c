/**
 * @file:   k_process.c  
 * @brief:  process management C file
 * @author: Yiqing Huang
 * @author: Thomas Reidemeister
 * @date:   2014/01/17
 * NOTE: The example code shows one way of implementing context switching.
 *       The code only has minimal sanity check. There is no stack overflow check.
 *       The implementation assumes six user processes and NO HARDWARE INTERRUPTS. 
 *       The purpose is to show how context switch could be done under stated assumptions. 
 *       These assumptions are not true in the required RTX Project!!!
 *       If you decide to use this piece of code, you need to understand the assumptions and
 *       the limitations. 
 */

#include <LPC17xx.h>
#include <system_LPC17xx.h>
#include "uart_polling.h"
#include "k_process.h"

#ifdef DEBUG_0
#include "printf.h"
#endif /* DEBUG_0 */

/* ----- Global Variables ----- */
PCB **gp_pcbs;                  /* array of pcb pointers */
//PCB g_pcbs[NUM_TEST_PROCS];			/* actual pcb array */
PCB_NODE **gp_pcb_nodes;  /* actual pcb node array */
PCB *gp_current_process = NULL; /* always point to the current RUN process */


/* process initialization table */
PROC_INIT g_proc_table[NUM_TEST_PROCS];
extern PROC_INIT g_test_procs[NUM_TEST_PROCS];

QUEUE ready_priority_queue[4];
QUEUE blocked_priority_queue;

/**
 * The Null process with priority 4
 */
void null_process() {
	while (1) {
		uart0_put_string("tnt");
		k_release_processor();
	}
}

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

PCB_NODE* peek(QUEUE *q) {
	return q->head;
}

int isEmpty(QUEUE *q) {
	return (q->head == NULL);
}

PCB_NODE* findPCB(int process_id){
	int i;
	for (i = 0; i < 4; i++){
		if(!isEmpty(&ready_priority_queue[i])){
			PCB_NODE *cur = ready_priority_queue[i].head;
			while(cur != NULL){
				if (cur->p_pcb->m_pid == process_id){
					return cur;
				}
				cur = cur->next;
			}
		}
	}
	return NULL;
}

int set_process_priority(int process_id, int priority){
	PCB_NODE * node = findPCB(process_id);
	if (process_id == 0 || priority < 0 || priority > 3){
		return -1;
	}
	if (node != NULL){
		node->p_pcb->m_priority = priority;
		enqueue(&ready_priority_queue[node->p_pcb->m_priority], node);
		return 1;
	}
	return -1;
}

int get_process_priority(int process_id){
	PCB_NODE *node = findPCB(process_id);
	if (node == NULL){
		return -1;
	}
	return node->p_pcb->m_priority;
}
	
/**
 * @biref: initialize all processes in the system
 * NOTE: We assume there are 6 processes in the system.
 */
void process_init() 
{
	int i;
	U32 *sp;
	
  /* fill out the initialization table */
	set_test_procs();
	
	for ( i = 0; i < NUM_TEST_PROCS; i++ ) {
		g_proc_table[i].m_pid = g_test_procs[i].m_pid;
		g_proc_table[i].m_priority = g_test_procs[i].m_priority;
		g_proc_table[i].m_stack_size = g_test_procs[i].m_stack_size;
		g_proc_table[i].mpf_start_pc = g_test_procs[i].mpf_start_pc;
	}
  
	// initilize exception stack frame (i.e. initial context) for each process 
	
	// Creating a PCB for the null process
	(gp_pcbs[0])->m_pid = 0;
	(gp_pcbs[0])->m_priority = 4;
	(gp_pcbs[0])->m_state = NEW;
	
	sp = alloc_stack(0x100);
		*(--sp)  = INITIAL_xPSR;      // user process initial xPSR  
		*(--sp)  = (U32)(&null_process); // PC contains the entry point of the process
	
	for ( i = 0; i < 6; i++ ) { // R0-R3, R12 are cleared with 0
		*(--sp) = 0x0;
	}
	(gp_pcbs[0])->mp_sp = sp;
	
	// Adding the null process to the global pcb array and pcb node array
	(gp_pcb_nodes[0])->next = NULL;
	(gp_pcb_nodes[0])->p_pcb = gp_pcbs[0];
	
	// Setting the value of the global PCB_NODE* for null process
	
	for ( i = 1; i < NUM_TEST_PROCS+1; i++ ) {
		int j;
		(gp_pcbs[i])->m_pid = (g_proc_table[i-1]).m_pid;
		(gp_pcbs[i])->m_state = NEW;
		(gp_pcbs[i])->m_priority = (g_proc_table[i-1]).m_priority;
		
		sp = alloc_stack((g_proc_table[i-1]).m_stack_size);
		*(--sp)  = INITIAL_xPSR;      // user process initial xPSR  
		*(--sp)  = (U32)((g_proc_table[i-1]).mpf_start_pc); // PC contains the entry point of the process
		for ( j = 0; j < 6; j++ ) { // R0-R3, R12 are cleared with 0
			*(--sp) = 0x0;
		}
		//g_proc_table[i-1].mp_sp = sp;
		(gp_pcbs[i])->mp_sp = sp;
		
		(gp_pcb_nodes[i])->next = NULL;
		(gp_pcb_nodes[i])->p_pcb = gp_pcbs[i];
	}
	
	// Nulling all the queues
	for (i = 0; i < 4; i++){
		ready_priority_queue[i].head = NULL;
		ready_priority_queue[i].tail = NULL;
	}
	
	// Adding processes to the appropriate ready queue
	for (i = 1; i < NUM_TEST_PROCS+1; i++) {
		enqueue(&(ready_priority_queue[(gp_pcbs[i])->m_priority]), gp_pcb_nodes[i]);
	}

	// Setting everything in the blocked queue to be null
	blocked_priority_queue.head = NULL;
	blocked_priority_queue.tail = NULL;	
}

/*@brief: scheduler, pick the pid of the next to run process
 *@return: PCB pointer of the next to run process
 *         NULL if error happens
 *POST: if gp_current_process was NULL, then it gets set to pcbs[0].
 *      No other effect on other global variables.
 */


PCB *scheduler(void)
{
	// Return null process there are no processes in the ready queue
	
	int i = 0;
	/*
	PCB_NODE* cur = NULL;
	PCB_NODE* prev = NULL;
	
	// Search through the blocked queue and move the unblocked processes to the ready queue
	cur = blocked_priority_queue.head;
	prev = blocked_priority_queue.head;
	
	while (cur != NULL){
		if (cur->p_pcb->m_state != BLOCKED){
			if (blocked_priority_queue.head == cur){
				prev = cur->next;
				blocked_priority_queue.head = prev;
				enqueue(&ready_priority_queue[cur->p_pcb->m_priority], cur);
				cur = prev;
			}
			else {
				prev->next = cur->next;
				enqueue(&ready_priority_queue[cur->p_pcb->m_priority], cur);
				cur = prev->next;
			}
		}
		else {
			if (cur != prev){
				prev = prev->next;
			}
			cur = cur->next;
		}
	}
	*/
	
	for (i = 0; i < 4; i++){
		if(!isEmpty(&ready_priority_queue[i])){
			return dequeue(&ready_priority_queue[i])->p_pcb;
		}
	}
	
	return gp_pcbs[0];
}

/*@brief: switch out old pcb (p_pcb_old), run the new pcb (gp_current_process)
 *@param: p_pcb_old, the old pcb that was in RUN
 *@return: RTX_OK upon success
 *         RTX_ERR upon failure
 *PRE:  p_pcb_old and gp_current_process are pointing to valid PCBs.
 *POST: if gp_current_process was NULL, then it gets set to pcbs[0].
 *      No other effect on other global variables.
 */
int process_switch(PCB *p_pcb_old) 
{
	PROC_STATE_E state;
	PCB * dummy = NULL;
	
	dummy = gp_current_process;

	state = gp_current_process->m_state;
	
	if(p_pcb_old->m_state == RUN){
		if(p_pcb_old->m_priority != 4){
			p_pcb_old->m_state = RDY;
			enqueue(&ready_priority_queue[p_pcb_old->m_priority], gp_pcb_nodes[p_pcb_old->m_pid]);
		}
	}
	
	if (state == NEW) {
		if (gp_current_process != p_pcb_old && p_pcb_old->m_state != NEW) {
			p_pcb_old->m_state = RDY;
			p_pcb_old->mp_sp = (U32 *) __get_MSP();
		}
		gp_current_process->m_state = RUN;
		__set_MSP((U32) gp_current_process->mp_sp);
		__rte();  // pop exception stack frame from the stack for a new processes
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
			//__rte();  // pop exception stack frame from the stack for a new processes
	}
	
	if(gp_current_process == NULL){
		gp_current_process = gp_pcbs[0];
	}
	return RTX_OK;
}

/**
 * @brief release_processor(). 
 * @return RTX_ERR on error and zero on success
 * POST: gp_current_process gets updated to next to run process
 */
int k_release_processor(void)
{
	PCB *p_pcb_old = NULL;
	
	p_pcb_old = gp_current_process;
	gp_current_process = scheduler();
	
	if ( gp_current_process == NULL  ) {
		gp_current_process = p_pcb_old; // revert back to the old process
		return RTX_ERR;
	}
  if ( p_pcb_old == NULL ) {
		p_pcb_old = gp_current_process;
	}
	process_switch(p_pcb_old);
	return RTX_OK;
}

/**
 *	Puts the current process into the blocked queue
 *  Mark the current process as blocked
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

/* 
 * put the first of the blocked queue to ready queue
 * mark it as ready
 */
void k_ready_first_blocked(void)
{
	PCB_NODE* nowReady = dequeue(&blocked_priority_queue);
	int priority = nowReady->p_pcb->m_priority;
	enqueue(&ready_priority_queue[priority], nowReady);
	nowReady = NULL;
}
