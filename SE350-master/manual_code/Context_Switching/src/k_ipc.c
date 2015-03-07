#include "k_ipc.h"
#include "k_memory.h"
#include "k_process.h"

// ENVELOPE envelopes[MAX_ENVELOPE];

// int msg_empty(PCB* q)
// {
// 	return (q->first_msg == NULL)?1:0;
// }

// void msg_enqueue(PCB* q, ENVELOPE* msg) {
// 	msg->nextMsg = NULL;
// 	if (q->first_msg == NULL)
// 	{
// 		q->first_msg = msg;
// 		q->last_msg = msg;
// 	}
// 	else
// 	{
// 		q->last_msg->nextMsg = msg;
// 		q->last_msg = q->last_msg->nextMsg;
// 	}
// }

// ENVELOPE* msg_dequeue(PCB* q, int* sender_ID) {
// 	if (msg_empty(q) == 1) return NULL;
// 	ENVELOPE* target = q->first_msg;
// 	if ((sender_ID == NULL)||(target->sender_pid == *sender_ID))
// 	{
// 		if (q->first_msg == q->last_msg)
// 		{
// 			q->first_msg = NULL;
// 			q->last_msg = NULL;
// 		}
// 		else
// 			q->first_msg = q->first_msg->nextMsg;
// 		target->nextMsg = NULL;
// 		return target;
// 	}
// 	else
// 	{
// 		ENVELOPE* previous = q->first_msg;
// 		target = target->nextMsg;
// 		while (target != NULL)
// 		{
// 			if (target->sender_pid == *sender_ID)
// 				break;
// 			target = target->nextMsg;
// 			previous = previous->nextMsg;
// 		}
// 		if (target == NULL) return NULL;
// 		if (target == q->last_msg)
// 		{
// 			previous->nextMsg = NULL;
// 			q->last_msg = previous;
// 		}
// 		else
// 			previous->nextMsg = previous->nextMsg->nextMsg;
// 		target->nextMsg = NULL;
// 		return target;
// 	}
// }

// int k_send_message(int target_pid, void* message_envelope)
// {
// // 	ENVELOPE* msg = (ENVELOPE*) message_envelope;
// // 	msg->nextMsg = NULL;
// // 	PCB* targetPCB = g(msg->destination_pid];
// // 	msg_enqueue(targetPCB, msg);
// // 	if (targetPCB->m_state == BLOCKED_ON_RECEIVE)
// // 	{
// // 		//TODO remove from blocked on receive queue
// // 		
// // 	}
// 	return 0;
// }

// void* k_receive_message(int* sender_ID)
// {
// 	return NULL;
// }
