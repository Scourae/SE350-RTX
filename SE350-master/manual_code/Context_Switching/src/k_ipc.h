#ifndef _K_IPC_H_
#define _K_IPC_H_

#ifdef DEBUG_HOTKEYS
	void k_print_blocked_on_receive_queue(void);
#endif

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

typedef struct env_queue{
	ENVELOPE* head;
	ENVELOPE* tail;
} ENV_QUEUE;

ENVELOPE* dequeue_env_queue(ENV_QUEUE *q);

void set_message(void* envelope, void* message, int msg_bytes_size);

int k_send_message(int target_pid, void* message_envelope);
void* k_receive_message(int* sender_ID);
void* k_non_block_receive_message(int* sender_ID);
void k_print_blocked_on_receive_queue(void);
#endif
