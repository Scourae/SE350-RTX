#include <LPC17xx.h>
#include "k_usr_proc.h"
#include "printf.h"
#include "string.h"
#include "rtx.h"
#include "k_rtx.h"

void stress_test_a(void){
	ENVELOPE *msg = (ENVELOPE *)request_memory_block();
	msg->message_type = MSG_COMMAND_REGISTRATION;
	msg->sender_pid = STRESS_TEST_A_PID;
	msg->destination_pid = KCD_PID;
	set_message(msg, "%Z" + '\0', 3*sizeof(char));
	send_message(KCD_PID, msg);	
	
	while(true) {
		msg = (ENVELOPE*) receive_message(NULL);
		char* command = (char*) msg->message;
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
		msg->message = &num;
		send_message(STRESS_TEST_B_PID, msg);	
		num += 1;
		release_processor();
	}
}

void stress_test_b(void){
}

void stress_test_c(void){
}
