/** 
 * @file:   k_rtx.h
 * @brief:  kernel deinitiation and data structure header file
 * @auther: Yiqing Huang
 * @date:   2014/01/17
 */
 
#ifndef K_RTX_H_
#define K_RTX_H_

/*----- Definitations -----*/

#define RTX_ERR -1
#define RTX_OK  0

#define NULL 0

#define NUM_TEST_PROCS 6
#define NUM_PROCS 16

#define TIMER_PID 14

#ifdef DEBUG_0
#define USR_SZ_STACK 0x200         /* user proc stack size 512B   */
#else
#define USR_SZ_STACK 0x100         /* user proc stack size 218B  */
#endif /* DEBUG_0 */

/*----- Types -----*/
typedef unsigned char U8;
typedef unsigned int U32;

/* process states, note we only assume three states in this example */
typedef enum {NEW = 0, RDY, RUN, BLOCKED_ON_MEMORY, BLOCKED_ON_RECEIVE, INTRPT} PROC_STATE_E;  

typedef struct envelope {
	struct envelope* nextMsg;
	U8 sender_pid;
	U8 destination_pid;
	U8 message_type;
	U8 delay;
	void* message;
} ENVELOPE;

typedef struct env_queue{
	ENVELOPE* head;
	ENVELOPE* tail;
} ENV_QUEUE;

/*
  PCB data structure definition.
  You may want to add your own member variables
  in order to finish P1 and the entire project 
*/
typedef struct pcb
{ 
	U32 *mp_sp;		/* stack pointer of the process */
	U32 m_pid;		/* process id */
	PROC_STATE_E m_state;   /* state of the process */
	U32 m_priority;
	ENV_QUEUE env_q;
} PCB;

/* initialization table item */
typedef struct proc_init
{	
	int m_pid;	        /* process id */ 
	int m_priority;         /* initial priority, not used in this example. */ 
	int m_stack_size;       /* size of stack in words */
	void (*mpf_start_pc) ();/* entry point of the process */ 
	//U32 *mp_sp;		/* stack pointer of the process */	
} PROC_INIT;

typedef struct pcb_node
{
	struct pcb_node *next;
	PCB *p_pcb;
} PCB_NODE;

typedef struct queue
{
	PCB_NODE *tail;
	PCB_NODE *head;
} QUEUE;

void enqueue(QUEUE *q, PCB_NODE *n);
PCB_NODE* dequeue(QUEUE *q);
PCB_NODE* peek(QUEUE *q);
int isEmpty(QUEUE *q);

ENVELOPE* msg_dequeue(ENV_QUEUE* q, int* sender_ID);
void msg_enqueue(ENV_QUEUE* q, ENVELOPE* msg);
int msg_empty(ENV_QUEUE* q);
#endif // ! K_RTX_H_
