/**
 * @file:   k_memory.c
 * @brief:  kernel memory managment routines
 * @author: Angad Kashyap, Vishal Bollu, Mike Wang and Tong Tong
 * @author: Yiqing Huang
 * @date:   2014/01/17
 */

#include "k_memory.h"
#include "k_process.h"

#ifdef DEBUG_0
	#include "printf.h"
#endif /* ! DEBUG_0 */

/* ----- Global Variables ----- */
U32 *gp_stack; 	/* The last allocated stack low address. 8 bytes aligned */
								/* The first stack starts at the RAM high address */
								/* stack grows down. Fully decremental stack */
U8* beginHeap;
U8* beginMemMap;
U8 *p_end;

/**
 * @brief: Initialize RAM as follows:

0x10008000+---------------------------+ High Address
          |    Proc 1 STACK           |
          |---------------------------|
          |    Proc 2 STACK           |
          |---------------------------|<--- gp_stack
          |                           |
          |        HEAP               |
          |                           |
					|---------------------------|<--- p_end
					|             .	            |
					|             . 	          |
					|             . 	          |
          |---------------------------|
          |        PCB_NODE 2         |
          |---------------------------|
          |        PCB_NODE 1         |
					|---------------------------|
          |             .	            |
					|             . 	          |
					|             . 	          |
          |---------------------------|
          |        PCB 2              |
          |---------------------------|
          |        PCB 1              |
          |---------------------------|
					|        PCB_NODE pointers  |
          |---------------------------|<--- gp_pcb_nodes
          |        PCB pointers       |
          |---------------------------|<--- gp_pcbs
          |        Padding            |
          |---------------------------|  
          |Image$$RW_IRAM1$$ZI$$Limit |
          |...........................|          
          |       RTX  Image          |
          |                           |
0x10000000+---------------------------+ Low Address

*/

void memory_init(void)
{
	int i;
  
	p_end = (U8 *)&Image$$RW_IRAM1$$ZI$$Limit;
	/* 4 bytes padding */
	p_end += 4;

	/* allocate memory for pcb pointers   */
	gp_pcbs = (PCB **)p_end;
	p_end += (NUM_PROCS) * sizeof(PCB *);
  
	for ( i = 0; i < NUM_PROCS; i++ ) {
		gp_pcbs[i] = (PCB *)p_end;
		p_end += sizeof(PCB); 
	}
	
	/* allocate memory for pcb node pointers   */
	gp_pcb_nodes = (PCB_NODE **)p_end;
	p_end += (NUM_PROCS) * sizeof(PCB_NODE *);
  
	for ( i = 0; i < NUM_PROCS; i++ ) {
		gp_pcb_nodes[i] = (PCB_NODE *)p_end;
		p_end += sizeof(PCB_NODE); 
	}
	
	/* prepare for alloc_stack() to allocate memory for stacks */
	gp_stack = (U32 *)RAM_END_ADDR;
	if ((U32)gp_stack & 0x04) { /* 8 bytes alignment */
		--gp_stack; 
	}
	
	// Calculate beginning of heap pointers
	beginMemMap = (U8*) p_end;
	beginHeap = (U8*) beginMemMap + NUM_OF_MEMBLOCKS;
	
	for (i = 0; i < NUM_OF_MEMBLOCKS; i++)
	{
		*(beginMemMap+i) = 0;
	}
}

/**
 * Allocates stack for a process, align to 8 bytes boundary
 * @param: size, stack size in bytes
 * @return: The top of the stack (i.e. high address)
 * POST:  gp_stack is updated.
 */

U32 *alloc_stack(U32 size_b) 
{
	U32 *sp;
	sp = gp_stack; /* gp_stack is always 8 bytes aligned */
	
	/* update gp_stack */
	gp_stack = (U32 *)((U8 *)sp - size_b);
	
	/* 8 bytes alignement adjustment to exception stack frame */
	if ((U32)gp_stack & 0x04) {
		--gp_stack; 
	}
	return sp;
}

/**
 * Checks whether the memory is empty
 * Returns 1 if memory is empty or 0 otherwise
 */
int mem_empty() {
	int i;
	for (i = 0;i < NUM_OF_MEMBLOCKS; i++)
	{
		if (*(beginMemMap + i) == 0)
		{
			return 0;
		}
	}
	return 1;
}

/**
 * Requests a memory block
 */
void *k_request_memory_block(void) {
	U8* rVoid = beginHeap;
	int i;
	__disable_irq();
	while(mem_empty() == 1)
	{
		k_block_current_processs();
		__enable_irq();
		k_release_processor();
		__disable_irq();
	}
	
	for (i = 0; i < NUM_OF_MEMBLOCKS; i++)
	{
		if (*(beginMemMap + i) == 0)
		{
			*(beginMemMap + i) = 1;
			break;
		}
	}
	
	/*#ifdef DEBUG_0 
		printf("k_request_memory_block: @ 0x%x\n\r", rVoid+i*MEMORY_BLOCK_SIZE);
	#endif */
	__enable_irq();
	return (void*) (rVoid+i*MEMORY_BLOCK_SIZE);
}

int k_non_block_release_memory_block(void *p_mem_blk)
{
	int index;
	int ready_priority;
	__disable_irq();
	if (p_mem_blk == NULL){
		return RTX_ERR;
	}
	
	if (((U8*) p_mem_blk < beginHeap)||((U8*)p_mem_blk + MEMORY_BLOCK_SIZE > (beginHeap+NUM_OF_MEMBLOCKS*MEMORY_BLOCK_SIZE))){ 
		return RTX_ERR;
	}
	
	if (((U8*)p_mem_blk - beginHeap)%MEMORY_BLOCK_SIZE != 0){
		return RTX_ERR;
	}
	
	index = ((U8*)p_mem_blk - beginHeap)/MEMORY_BLOCK_SIZE;
	*(beginMemMap + index) = 0;
	ready_priority = k_ready_first_blocked();
	__enable_irq();
	return ready_priority;
}

/**
 * Releases a memory block
 */
int k_release_memory_block(void *p_mem_blk) {
	int ready_priority;
	ready_priority = k_non_block_release_memory_block(p_mem_blk);
	if (ready_priority != k_get_current_process()->m_priority)
		k_release_processor();
	return RTX_OK;
}
