/* @brief: rtx.h User API prototype, this is only an example
 * @author: Yiqing Huang
 * @date: 2014/01/17
 */
#ifndef RTX_H_
#define RTX_H_

/* ----- Definitations ----- */
#define RTX_ERR -1
#define NULL 0
#define NUM_TEST_PROCS 6
/* Process Priority. The bigger the number is, the lower the priority is*/
#define HIGH    0
#define MEDIUM  1
#define LOW     2
#define LOWEST  3
#define NULL_PROC 4 /* the hidden priority for the null process only */
#define SYS_PROC 5 /* special priority for system processes KCD, CRT, and Wall Clock (and set priority process)*/

/* ----- Types ----- */
typedef unsigned int U32;

/* initialization table item */
typedef struct proc_init
{	
	int m_pid;	        /* process id */ 
	int m_priority;         /* initial priority */ 
	int m_stack_size;       /* size of stack in words */
	void (*mpf_start_pc) ();/* entry point of the process */    
} PROC_INIT;

/* ----- RTX User API ----- */
#define __SVC_0  __svc_indirect(0)

extern void k_rtx_init(void);
#define rtx_init() _rtx_init((U32)k_rtx_init)
extern void __SVC_0 _rtx_init(U32 p_func);

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

#endif /* !RTX_H_ */
