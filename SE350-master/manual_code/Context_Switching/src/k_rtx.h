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

#define STRESS_TEST_A_PID 7
#define STRESS_TEST_B_PID 8
#define STRESS_TEST_C_PID 9
#define SET_PRIORITY_PID 10
#define WALL_CLOCK_PID 11
#define KCD_PID 12
#define CRT_PID 13
#define TIMER_PID 14
#define UART_IPROC_PID 15

// Keyboard command bounds
#define KC_MAX_CHAR 10
#define KC_MAX_COMMANDS 16

#ifdef DEBUG_HOTKEYS
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
	MSG_COMMAND_REGISTRATION,
	MSG_CRT_DISPLAY,
	MSG_KCD_DISPATCH,
	MSG_WALL_CLOCK,
	MSG_COUNT_REPORT,
	MSG_WAKEUP10
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

#define __SVC_0  __svc_indirect(0)

extern int k_release_processor(void);
#define release_processor() _release_processor((U32)k_release_processor)
extern int __SVC_0 _release_processor(U32 p_func);

extern void *k_request_memory_block(void);
#define request_memory_block() _request_memory_block((U32)k_request_memory_block)
extern void *_request_memory_block(U32 p_func) __SVC_0;
/* __SVC_0 can also be put at the end of the function declaration */

extern int k_release_memory_block(void *);
#define release_memory_block(p_mem_blk) _release_memory_block((U32)k_release_memory_block, p_mem_blk)
extern int _release_memory_block(U32 p_func, void *p_mem_blk) __SVC_0;

extern int k_get_process_priority(int pid);
#define get_process_priority(pid) _get_process_priority((U32)k_get_process_priority, pid)
extern int _get_process_priority(U32 p_func, int pid) __SVC_0;
/* __SVC_0 can also be put at the end of the function declaration */

extern int k_set_process_priority(int pid, int prio);
#define set_process_priority(pid, prio) _set_process_priority((U32)k_set_process_priority, pid, prio)
extern int _set_process_priority(U32 p_func, int pid, int prio) __SVC_0;

extern int k_delayed_send(int process_id, void * env, int delay);
#define delayed_send(pid, env, delay) _delayed_send((U32)k_delayed_send, pid, env, delay)
extern int _delayed_send(U32 p_func, int target_pid, void* message_envelope, int delay) __SVC_0;

#endif // ! K_RTX_H_
