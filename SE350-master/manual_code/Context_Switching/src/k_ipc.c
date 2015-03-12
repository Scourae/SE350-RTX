#include "k_ipc.h"
#include "k_memory.h"
#include "k_process.h"
#include "uart_polling.h"

extern PCB_NODE* blocked_on_receive_list;
extern int send_message_preemption_flag;
extern int uart_preemption_flag;

void add_to_blocked_list(PCB_NODE* target)
{
	PCB_NODE* pointer = blocked_on_receive_list;
	target->next = NULL;
	if (blocked_on_receive_list == NULL)
	{
		blocked_on_receive_list = target;
	}
	else
	{
		while(pointer->next != NULL)
			pointer = pointer->next;
		pointer->next = target;
	}
}

PCB_NODE* remove_from_blocked_list(int pid)
{
	PCB_NODE* target = blocked_on_receive_list;
	PCB_NODE* previous = blocked_on_receive_list;
	if (blocked_on_receive_list->p_pcb->m_pid == pid)
	{
		blocked_on_receive_list = blocked_on_receive_list->next;
		return target;
	}
	else
	{
		while (previous->next != NULL)
		{
			if (previous->next->p_pcb->m_pid == pid)
				break;
			previous = previous->next;
		}
		target = previous->next;
		if (target == NULL) return NULL;
		previous->next = previous->next->next;
		return target;
	}
}

 int msg_empty(ENV_QUEUE* q)
 {
 	return (q->head == NULL)?1:0;
 }

 void msg_enqueue(ENV_QUEUE* q, ENVELOPE* msg) {
 	msg->nextMsg = NULL;
 	if (q->head == NULL)
 	{
 		q->head = msg;
 		q->tail = msg;
 	}
 	else
 	{
 		q->tail->nextMsg = msg;
 		q->tail = q->tail->nextMsg;
 	}
 }

 int k_send_message(int target_pid, void* message_envelope)
 {
		ENVELOPE* msg = (ENVELOPE*) message_envelope;
		PCB* gp_current_process = gp_pcb_nodes[msg->sender_pid]->p_pcb;
		PCB* targetPCB = gp_pcb_nodes[target_pid]->p_pcb;
	  __disable_irq();
		msg->nextMsg = NULL;
		msg_enqueue(&(targetPCB->env_q), msg);
		if (targetPCB->m_state == BLOCKED_ON_RECEIVE)
		{
			remove_from_blocked_list(msg->destination_pid);
			k_ready_process(msg->destination_pid);
			if ((gp_current_process->m_priority < targetPCB->m_priority)&& send_message_preemption_flag){
				__enable_irq();
				k_release_processor();
				__disable_irq();
			}
		}
		__enable_irq();
		return 0;
 }

 
 
 void* k_receive_message(int* sender_ID)
 {
	 ENVELOPE* msg;
	 PCB* gp_current_process = k_get_current_process();
	 PCB_NODE* currPro = gp_pcb_nodes[gp_current_process->m_pid];
	 __disable_irq();
	while(msg_empty(&(gp_current_process->env_q)))
	{
		if (gp_current_process->m_state != BLOCKED_ON_RECEIVE)
		{
			gp_current_process->m_state = BLOCKED_ON_RECEIVE;
			currPro->next = NULL;
			add_to_blocked_list(currPro);
		}
		__enable_irq();
		k_release_processor();
		__disable_irq();
	}
	msg = dequeue_env_queue(&(gp_current_process->env_q));
	sender_ID = (int*) &msg->sender_pid;
	__enable_irq();
	return (void*) msg;
 }

 void set_message(void* envelope, void* message, int msg_size_bytes)
 {
	 ENVELOPE* msg = (ENVELOPE*) envelope;
	 U32* target = (U32*) envelope + HEADER_OFFSET;
	 U32* source = (U32*)message;
	 int i;
	 msg->message = target;
	 for (i = 0; i < msg_size_bytes; i++)
	 {
		 *target = *source;
		 source += 1;
		 target += 1;
	 }
 }
 
 void* k_non_block_receive_message(int destination_ID)
 {
		ENVELOPE* msg;
		PCB* gp_current_process = gp_pcb_nodes[destination_ID]->p_pcb;
		msg = dequeue_env_queue(&(gp_current_process->env_q));
		return (void*) msg;
 }

 // HotKey #3 helper function
void k_print_blocked_on_receive_queue_helper(int priority){
	PCB_NODE* cur = blocked_on_receive_list;
	char num = '0';
	while (cur != NULL)
	{
		if (cur->p_pcb->m_priority == priority)
			break;
		cur = cur->next;
	}
	if (cur == NULL) return;
	uart1_put_string("\n\rPriority ");
	uart1_put_char(num+priority);
	uart1_put_string(":\n\r");
	while(cur != NULL){
		if(cur->p_pcb->m_priority == priority){
				uart1_put_string("\t Process with PID ");
			  uart1_put_char(num+cur->p_pcb->m_pid);
				uart1_put_string("\n\r");
		}
		cur = cur->next;
	} 
}

// HotKey #3: printing to the RTX system debug terminal all the procs currently on the blocked on receive queue
void k_print_blocked_on_receive_queue()
{
	int i = 0;
	uart1_put_string("\n\r\n\r----- PROCESSES CURRENTLY IN BLOCKED ON RECEIVE QUEUE -----\n\r");

	for(i = 0; i < 4; i++){
		k_print_blocked_on_receive_queue_helper(i);
	}
} 
