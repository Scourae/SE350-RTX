#ifndef _K_IPC_H_
#define _K_IPC_H_

#define MAX_ENVELOPE 32
#define SENDER_ID_OFFSET 										sizeof(ENVELOPE*)
#define DESTINATION_ID_OFFSET 							SENDER_ID_OFFSET + sizeof(U32)
#define MESSAGE_TYPE_OFFSET 								DESTINATION_ID_OFFSET + sizeof(U32)
#define DELAY_OFFSET 												MESSAGE_TYPE_OFFSET + sizeof(U32)
#define HEADER_OFFSET 											DELAY_OFFSET + sizeof(U32)

typedef unsigned int U32;

typedef struct envelope {
	struct envelope* nextMsg;
	U32 sender_pid;
	U32 destination_pid;
	U32 message_type;
	U32 delay;
	void* message;
} ENVELOPE;

void set_message(void* envelope, void* message, int msg_bytes_size);
void k_print_blocked_on_receive_queue(void);
#endif
