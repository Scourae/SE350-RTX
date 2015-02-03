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
PCB **gp_pcbs;                  /* array of pcbs */
PCB *gp_current_process = NULL; /* always point to the current RUN process */


/* process initialization table */
PROC_INIT g_proc_table[NUM_TEST_PROCS];
extern PROC_INIT g_test_procs[NUM_TEST_PROCS];
PCB_NODE null_process_node;

QUEUE ready_priority_queue[4];
QUEUE blocked_priority_queue;

PCB_NODE newNode [NUM_TEST_PROCS];

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
	PCB nullProcPCBTemp;
  PCB_NODE nullPCBNode;
	
  /* fill out the initialization table */
	set_test_procs();
	
	for ( i = 0; i < NUM_TEST_PROCS; i++ ) {
		g_proc_table[i].m_pid = g_test_procs[i].m_pid;
		g_proc_table[i].m_priority = g_test_procs[i].m_priority;
		g_proc_table[i].m_stack_size = g_test_procs[i].m_stack_size;
		g_proc_table[i].mpf_start_pc = g_test_procs[i].mpf_start_pc;
	}
  
	/* initilize exception stack frame (i.e. initial context) for each process */
		// Creating a PCB for the null process
	
	nullProcPCBTemp.m_pid = 0;
	nullProcPCBTemp.m_priority = 4;
	nullProcPCBTemp.m_state = RDY;
	
	sp = alloc_stack(0x100);
		*(--sp)  = INITIAL_xPSR;      // user process initial xPSR  
		*(--sp)  = (U32)(&null_process); // PC contains the entry point of the process
	
	for ( i = 0; i < 6; i++ ) { // R0-R3, R12 are cleared with 0
		*(--sp) = 0x0;
	}
	nullProcPCBTemp.mp_sp = sp;
	
	// Creating a PCB_NODE for the null process
	nullPCBNode.next = NULL;
	nullPCBNode.p_pcb = &nullProcPCBTemp;
	
	null_process_node = nullPCBNode;
	
	// Setting the value of the global PCB_NODE* for null process
	
	for ( i = 0; i < NUM_TEST_PROCS; i++ ) {
		int j;
		(gp_pcbs[i])->m_pid = (g_proc_table[i]).m_pid;
		(gp_pcbs[i])->m_state = NEW;
		(gp_pcbs[i])->m_priority = (g_proc_table[i]).m_priority;
		
		sp = alloc_stack((g_proc_table[i]).m_stack_size);
		*(--sp)  = INITIAL_xPSR;      // user process initial xPSR  
		*(--sp)  = (U32)((g_proc_table[i]).mpf_start_pc); // PC contains the entry point of the process
		for ( j = 0; j < 6; j++ ) { // R0-R3, R12 are cleared with 0
			*(--sp) = 0x0;
		}
		(gp_pcbs[i])->mp_sp = sp;
	}
	
	// Adding the null process to the global pcb array
	(gp_pcbs[NUM_TEST_PROCS])->m_pid = null_process_node.p_pcb->m_pid;
	(gp_pcbs[NUM_TEST_PROCS])->m_state = RDY;
	(gp_pcbs[NUM_TEST_PROCS])->m_priority = null_process_node.p_pcb->m_priority;
	(gp_pcbs[NUM_TEST_PROCS])->mp_sp = null_process_node.p_pcb->mp_sp;
	
	// Placing the processes in the priority queue
	for (i = 0; i < 4; i++){
		ready_priority_queue[i].head = NULL;
		ready_priority_queue[i].tail = NULL;
	}
	
	for (i = 0; i < NUM_TEST_PROCS; i++) {
		newNode[i].next = NULL;
		//gp_pcbs[i]->m_state = RDY;
		newNode[i].p_pcb = gp_pcbs[i];
		enqueue(&(ready_priority_queue[(gp_pcbs[i])->m_priority]), &newNode[i]);
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
	PCB_NODE* cur = NULL;
	PCB_NODE* prev = NULL;
	PCB* test2 = NULL;
	
	test2 = gp_pcbs[NUM_TEST_PROCS];
	
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

	test2 = gp_pcbs[NUM_TEST_PROCS];
	
	for (i = 0; i < 4; i++){
		if(!isEmpty(&ready_priority_queue[i])){
			return dequeue(&ready_priority_queue[i])->p_pcb;
		}
	}
	
	test2 = gp_pcbs[NUM_TEST_PROCS];
	
	return gp_pcbs[NUM_TEST_PROCS];
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
	PCB * dummy;
	
	state = gp_current_process->m_state;

	dummy = gp_pcbs[NUM_TEST_PROCS];
	
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
	dummy = gp_pcbs[NUM_TEST_PROCS];
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
			__rte();  // pop exception stack frame from the stack for a new processes
	}
	
	dummy = gp_pcbs[NUM_TEST_PROCS];
	
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

	p_pcb_old = gp_pcbs[2];
	
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
