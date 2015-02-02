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

QUEUE* ready_priority_queue[4];
QUEUE blocked_priority_queue;

void enqueue(QUEUE *q, PCB_NODE *n) {
	n->next = NULL;
	q->tail = n;
	q->tail = q->tail->next;
}

PCB_NODE* dequeue(QUEUE *q) {
	PCB_NODE *curHead = q->head;
	q->head = q->head->next;
	return curHead; //TODO: does this delete the node from the queue?
}

PCB_NODE* peek(QUEUE *q) {
	return q->head;
}

int isEmpty(QUEUE *q) {
	return (q->head == NULL);
}

PCB* findPCB(int process_id){
	int i;
	for (i = 0; i < 4; i++){
		if(!isEmpty(ready_priority_queue[i])){
			PCB_NODE *cur = ready_priority_queue[i]->head;
			while(cur != NULL){
				if (cur->p_pcb->m_pid == process_id){
					return cur->p_pcb;
				}
				cur = cur->next;
			}
		}
	}
	return NULL;
}

int set_process_priority(int process_id, int priority){
	PCB * node = findPCB(process_id);
	if (process_id == 0 || priority < 0 || priority > 3){
		return -1;
	}
	if (node != NULL){
		node->m_priority = priority;
		return 1;
	}
	return -1;
}

int get_process_priority(int process_id){
	PCB *node = findPCB(process_id);
	if (node == NULL){
		return -1;
	}
	return node->m_priority;
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
  
	/* initilize exception stack frame (i.e. initial context) for each process */
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
	
	// Setting the 
}

/*@brief: scheduler, pick the pid of the next to run process
 *@return: PCB pointer of the next to run process
 *         NULL if error happens
 *POST: if gp_current_process was NULL, then it gets set to pcbs[0].
 *      No other effect on other global variables.
 */

PCB *scheduler(void)
{
	if (gp_current_process == NULL) {
		gp_current_process = gp_pcbs[0]; 
		return gp_pcbs[0];
	}

	// TODO: choose the process with the highest priority that is not blocked
	if ( gp_current_process == gp_pcbs[0] ) {
		return gp_pcbs[1];
	} else if ( gp_current_process == gp_pcbs[1] ) {
		return gp_pcbs[2];
	} else if ( gp_current_process == gp_pcbs[2] ) {
		return gp_pcbs[3];
	} else if ( gp_current_process == gp_pcbs[3] ) {
		return gp_pcbs[4];
	} else if ( gp_current_process == gp_pcbs[4] ) {
		return gp_pcbs[5];
	} else if ( gp_current_process == gp_pcbs[5] ) {
		return gp_pcbs[0];
	} else {
		return NULL;
	}
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
	
	state = gp_current_process->m_state;

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
			// TODO: do we need this?
			gp_current_process = p_pcb_old; // revert back to the old proc on error
			return RTX_ERR;
		} 
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
	enqueue(ready_priority_queue[priority], nowReady);
	nowReady = NULL;
}

/**
 * The Null process with priority 4
 */
// Need to set priority of null process to 4 and its PID to 0
void null_process() {
	while (1) {
		k_release_processor();
	}
}
