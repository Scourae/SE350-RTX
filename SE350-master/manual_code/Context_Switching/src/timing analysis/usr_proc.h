/**
 * @file:   usr_proc.h
 * @brief:  Two user processes header file
 * @author: Yiqing Huang
 * @date:   2014/01/17
 */
 
#ifndef USR_PROC_H_
#define USR_PROC_H

void set_test_procs(void);
void send_delayed_message(void);
void receive_delayed_message(void);
void receive_delayed_message_preemption(void);
void request_all_memory_block(void);

void send_message_to_blocked(void);
void receive_message_to_blocked(void);
#endif /* USR_PROC_H_ */
