#include "k_memory.h"
#include "k_process.h"

int msg_empty(PCB* q)
{
	return q->first_msg == NULL?1:0;
}

void msg_enqueue(PCB* q, envelope* msg) {
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

envelope* msg_dequeue(PCB* q, int* sender_ID) {
	if (msg_empty(q) == 1) return NULL;
	envelope* target = q->first_msg;
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
		envelope* previous = q->first_msg;
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
	evnelope* msg = (envelope*) message_envelope;
	msg->nextMsg = NULL;
	PCB* targetPCB = get_PCB_block(msg->destination_pid);
	msg_enqueue(targetPCB, msg);
	if (targetPCB->m_state == BLOCKED_ON_RECEIVE)
	{
		//TODO remove from blocked on receive queue
		
	}
}

void* k_receive_message(int* sender_ID)
{
	
}