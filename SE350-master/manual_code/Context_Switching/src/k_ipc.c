#include "k_rtx.h"
#include "k_memory.h"
#include "k_process.h"

#define MAX_ENVELOPE 32
PCB_NODE* blocked_on_receive_list = NULL;

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

 int msg_empty(PCB* q)
 {
 	return (q->first_msg == NULL)?1:0;
 }

 void msg_enqueue(PCB* q, ENVELOPE* msg) {
 	msg->nextMsg = NULL;
 	if (q->first_msg == NULL)
 	{
 		q->first_msg = msg;
 		q->last_msg = msg;
 	}
 	else
 	{
 		q->last_msg->nextMsg = msg;
 		q->last_msg = q->last_msg->nextMsg;
 	}
 }

 ENVELOPE* msg_dequeue(PCB* q, int* sender_ID) {
 	ENVELOPE* target = q->first_msg;
	ENVELOPE* previous = q->first_msg;
	if (msg_empty(q) == 1) return NULL;
 	if ((sender_ID == NULL)||(target->sender_pid == *sender_ID))
 	{
 		if (q->first_msg == q->last_msg)
 		{
 			q->first_msg = NULL;
 			q->last_msg = NULL;
 		}
 		else
 			q->first_msg = q->first_msg->nextMsg;
 		target->nextMsg = NULL;
 		return target;
 	}
 	else
 	{
 		target = target->nextMsg;
 		while (target != NULL)
 		{
 			if (target->sender_pid == *sender_ID)
 				break;
 			target = target->nextMsg;
 			previous = previous->nextMsg;
 		}
 		if (target == NULL) return NULL;
 		if (target == q->last_msg)
 		{
 			previous->nextMsg = NULL;
 			q->last_msg = previous;
 		}
 		else
 			previous->nextMsg = previous->nextMsg->nextMsg;
 		target->nextMsg = NULL;
 		return target;
 	}
 }

 int k_send_message(int target_pid, void* message_envelope)
 {
  	PCB* gp_current_process = k_get_current_process();
		ENVELOPE* msg = (ENVELOPE*) message_envelope;
		PCB* targetPCB = gp_pcb_nodes[msg->destination_pid]->p_pcb;
  	msg->nextMsg = NULL;
  	msg_enqueue(targetPCB, msg);
  	if (targetPCB->m_state == BLOCKED_ON_RECEIVE)
  	{
  		remove_from_blocked_list(msg->destination_pid);
			k_ready_process(msg->destination_pid);
			if (gp_current_process->m_priority < targetPCB->m_priority)
				k_release_processor();
		}
 	return 0;
 }

 void* k_receive_message(int* sender_ID)
 {
	ENVELOPE* msg;
	PCB* gp_current_process = k_get_current_process();
	PCB_NODE* currPro = gp_pcb_nodes[gp_current_process->m_pid];
	while(msg_empty(gp_current_process))
	{
		if (gp_current_process->m_state != BLOCKED_ON_RECEIVE)
		{
			gp_current_process->m_state = BLOCKED_ON_RECEIVE;
			currPro->next = NULL;
			add_to_blocked_list(currPro);
		}
		k_release_processor();
	}
	msg = msg_dequeue(gp_current_process, sender_ID);
	return (void*) msg;
 }
