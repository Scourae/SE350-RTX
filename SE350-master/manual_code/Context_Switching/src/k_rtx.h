/** 
 * @file:   k_rtx.h
 * @brief:  kernel deinitiation and data structure header file
 * @auther: Yiqing Huang
 * @date:   2014/01/17
 */
 
#ifndef K_RTX_H_
#define K_RTX_H_

#include "k_ipc.h"

/*----- Definitations -----*/

#define RTX_ERR -1
#define RTX_OK  0

#define NULL 0

#define NUM_TEST_PROCS 6
#define NUM_PROCS 16

#define KCD_PID 12
#define TIMER_PID 14
#define UART_IPROC_PID 15

// Keyboard command bounds
#define KC_MAX_CHAR 10
#define KC_MAX_COMMANDS 16

#ifdef DEBUG_HOTKEY
#define DEBUG_HOTKEY_1 '!'
#define DEBUG_HOTKEY_2 '@'
#define DEBUG_HOTKEY_3 '#'
#endif

#ifdef DEBUG_0
#define USR_SZ_STACK 0x200         /* user proc stack size 512B   */
#else
#define USR_SZ_STACK 0x100         /* user proc stack size 218B  */
#endif /* DEBUG_0 */

/* Process Priority. The bigger the number is, the lower the priority is*/
#define HIGH    0
#define MEDIUM  1
#define LOW     2
#define LOWEST  3
#define NULL_PROC 4 /* the hidden priority for the null process only */
#define SYS_PROC 5 /* special priority for system processes KCD, CRT, and Wall Clock (and set priority process)*/

/*----- Types -----*/
typedef unsigned char U8;
typedef unsigned int U32;

/* process states, note we only assume three states in this example */
typedef enum {NEW = 0, RDY, RUN, BLOCKED_ON_MEMORY, BLOCKED_ON_RECEIVE, INTRPT} PROC_STATE_E;  

/* Message tyes */
typedef enum {
	MSG_DEFAULT = 0,
	MSG_CONSOLE_INPUT,
	MSG_COMMAND_REGISTRATION
} MSG_TYPE_E;

// Keyboard command list
typedef struct kc_list {
	char command [KC_MAX_CHAR];
	int pid;
} KC_LIST;

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
