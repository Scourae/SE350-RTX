#ifndef _K_IPC_H_
#define _K_IPC_H_

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
#endif
