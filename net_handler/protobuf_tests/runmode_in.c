#include <stdio.h>
#include <stdlib.h>
#include "../pbc_gen/run_mode.pb-c.h"
#define MAX_MSG_SIZE 1024

static size_t read_buffer (unsigned max_length, uint8_t *out)
{
	size_t cur_len = 0;
	size_t nread;
	while ((nread = fread(out + cur_len, 1, max_length - cur_len, stdin)) != 0)
	{
		cur_len += nread;
		if (cur_len == max_length)
		{
			fprintf(stderr, "max message length exceeded\n");
			exit(1);
		}
	}
	return cur_len;
}

int main () 
{
	RunMode *run_mode;
	
	// Read packed message from standard-input.
	uint8_t buf[MAX_MSG_SIZE];
	size_t msg_len = read_buffer(MAX_MSG_SIZE, buf);
	
	// Unpack the message using protobuf-c.
	run_mode = run_mode__unpack(NULL, msg_len, buf);	
	if (run_mode == NULL)
	{
		fprintf(stderr, "error unpacking incoming message\n");
		exit(1);
	}
	
	// display the message's fields.
	printf("Received: mode = %u\n", run_mode->mode); //comes in as unsigned int

	// Free the unpacked message
	run_mode__free_unpacked(run_mode, NULL);
	return 0;
}