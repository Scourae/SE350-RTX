#include <LPC17xx.h>
#include "k_usr_proc.h"
#include "printf.h"
#include "string.h"
#include "k_rtx.h"

void stress_test_a(void){
	int num;
	ENVELOPE *msg = (ENVELOPE *)request_memory_block();
	char* command; 
	msg->message_type = MSG_COMMAND_REGISTRATION;
	msg->sender_pid = STRESS_TEST_A_PID;
	msg->destination_pid = KCD_PID;
	set_message(msg, "%Z" + '\0', 3*sizeof(char));
	send_message(KCD_PID, msg);	
	
	while(1) {
		msg = (ENVELOPE*) receive_message(NULL);
		command = (char*) msg->message;
		if ((command[0] == '%')&&(command[1] == 'Z'))
		{
			release_memory_block(msg);
			break;
		}
		else
		{
			release_memory_block(msg);
		}
	}
	num = 0;
	while(1) {
		msg = (ENVELOPE *)request_memory_block();
		msg->message_type = MSG_COUNT_REPORT;
		msg->sender_pid = STRESS_TEST_A_PID;
		msg->destination_pid = STRESS_TEST_B_PID;
		msg->message = (void*) num;
		send_message(STRESS_TEST_B_PID, msg);	
		num += 1;
		release_processor();
	}
}

void stress_test_b(void){
	while (1){
		ENVELOPE* msg= receive_message(NULL);
		msg->sender_pid = STRESS_TEST_B_PID;
		msg->destination_pid = STRESS_TEST_C_PID;
		send_message(STRESS_TEST_C_PID, msg);
	}
}

void stress_test_c(void){
	ENV_QUEUE messageQueue;
	ENVELOPE * p;
	ENVELOPE * q;
	
	while(1) {
		if(msg_empty(&messageQueue)) {
			p = receive_message(NULL);
		} else {
			p = dequeue_env_queue(&messageQueue);
		}
		
		if( p->message_type == MSG_COUNT_REPORT ) {
			int value = (int)(p->message);
			non_block_release_memory_block(p);
			if(value % 31 == 0) {
				ENVELOPE * msg;
				char* message;
				message = "Process C";
				message[9] = '\r';
				message[10] = '\n';
				message[11] = '\0';
				msg = request_memory_block();
				msg->message_type = MSG_CRT_DISPLAY;
				msg->sender_pid = STRESS_TEST_C_PID;
				msg->destination_pid = CRT_PID;
				set_message(msg, message, 12*sizeof(char));
				send_message(CRT_PID, msg);	
				/*hibernate for 10s*/
				q=(ENVELOPE *) request_memory_block();
				q->message_type=MSG_WAKEUP10;
				q->sender_pid = STRESS_TEST_C_PID;
				q->destination_pid = STRESS_TEST_C_PID;
				delayed_send(STRESS_TEST_C_PID, q, 10000);
				while (1) {
					p= receive_message(NULL);
					if(p->message_type == MSG_WAKEUP10) {
						break;
					} else {
						msg_enqueue(&messageQueue, p);
					}					
				}
			}
		}
		release_memory_block(p);
	}
}
