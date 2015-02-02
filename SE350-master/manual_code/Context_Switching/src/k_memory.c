/**
 * @file:   k_memory.c
 * @brief:  kernel memory managment routines
 * @author: Yiqing Huang
 * @date:   2014/01/17
 */

#include "k_memory.h"
#include "k_process.h"

#ifdef DEBUG_0
#include "printf.h"
#endif /* ! DEBUG_0 */

/* ----- Global Variables ----- */
U32 *gp_stack; /* The last allocated stack low address. 8 bytes aligned */
               /* The first stack starts at the RAM high address */
	       /* stack grows down. Fully decremental stack */
#define MEMORY_BLOCK_SIZE 128
struct FreeHeap {
	struct FreeHeap* next;
	U8* memory_block;
};
U8* endHeap;
struct FreeHeap* heapStart = NULL;
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
          |        PCB 2              |
          |---------------------------|
          |        PCB 1              |
          |---------------------------|
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
	U32 flush = 0x00000000;
	U8* target;
	struct FreeHeap start;
	struct FreeHeap* tempPointer;
  
	p_end = (U8 *)&Image$$RW_IRAM1$$ZI$$Limit;
	/* 4 bytes padding */
	p_end += 4;

	/* allocate memory for pcb pointers   */
	gp_pcbs = (PCB **)p_end;
	p_end += NUM_TEST_PROCS * sizeof(PCB *);
  
	for ( i = 0; i < NUM_TEST_PROCS; i++ ) {
		gp_pcbs[i] = (PCB *)p_end;
		p_end += sizeof(PCB); 
	}
#ifdef DEBUG_0  
	printf("gp_pcbs[0] = 0x%x \n", gp_pcbs[0]);
	printf("gp_pcbs[1] = 0x%x \n", gp_pcbs[1]);
#endif
	
	/* prepare for alloc_stack() to allocate memory for stacks */
	
	gp_stack = (U32 *)RAM_END_ADDR;
	if ((U32)gp_stack & 0x04) { /* 8 bytes alignment */
		--gp_stack; 
	}
	
	// Calculate end of stack pointers
	endHeap = (U8*) gp_stack - USR_SZ_STACK*NUM_TEST_PROCS;
	// Heap flush
	target = p_end;
	while (target < endHeap)
	{
		*target = flush;
		target+= sizeof(flush);
	}
	
	/* allocate memory for heap ADDED BY MIKE*/
	/* Heap starts at the end of the process control blocks, it ends at stack pointers */
	start.memory_block = p_end;
	start.next = NULL;
	heapStart = &start;
	tempPointer = heapStart;
	while (tempPointer->memory_block + 2*MEMORY_BLOCK_SIZE < endHeap)
	{
		struct FreeHeap nextPointer;
		nextPointer.next = NULL;
		nextPointer.memory_block = tempPointer->memory_block + MEMORY_BLOCK_SIZE;
		tempPointer->next = &nextPointer;
		tempPointer = tempPointer->next;
	}
}

/**
 * @brief: allocate stack for a process, align to 8 bytes boundary
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

// 1 if theres heap to allocate, 0 otherwise
int hasHeap() 
{
	if (heapStart == NULL)
		return 0;
	if (heapStart->next->memory_block > endHeap)
		return 0;
	return 1;
}

void *k_request_memory_block(void) {
	void* rVoid;
#ifdef DEBUG_0 
	printf("k_request_memory_block: entering...\n");
#endif /* ! DEBUG_0 */
	if (hasHeap() == 0)
	{
		// No more memory to give
		while(hasHeap == 0)
		{
			k_block_current_processs();
			k_release_processor();
		}
	}
	rVoid = heapStart->memory_block;
	heapStart = heapStart->next;
	return (void*) rVoid;
}

int k_release_memory_block(void *p_mem_blk) {
	struct FreeHeap freed;
#ifdef DEBUG_0 
	printf("k_release_memory_block: releasing block @ 0x%x\n", p_mem_blk);
#endif /* ! DEBUG_0 */
	if (p_mem_blk == NULL) return RTX_ERR;
	if (((U8*) p_mem_blk < p_end)||((U8*)p_mem_blk + MEMORY_BLOCK_SIZE > endHeap)) return RTX_ERR;
	freed.next = NULL;
	freed.memory_block = p_mem_blk;
	if (heapStart == NULL)
	{
		heapStart = &freed;
	}
	else
	{
		if (heapStart->memory_block > p_mem_blk)
		{
			freed.next = heapStart;
			heapStart = &freed;
		}
		else
		{
			struct FreeHeap* tempPointer = heapStart;
			while (tempPointer->next != NULL)
			{
				if (tempPointer->next->memory_block > p_mem_blk)
				{
					freed.next = tempPointer->next;
					tempPointer->next = &freed;
					break;
				}
				else
				{
					tempPointer = tempPointer->next;
				}
			}
			if (tempPointer->next == NULL)
			{
				tempPointer->next = &freed;
			}
	  }
	}
	k_ready_first_blocked();
	return RTX_OK;
}
