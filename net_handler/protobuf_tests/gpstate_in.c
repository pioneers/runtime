#include <stdio.h>
#include <stdlib.h>
#include "../protobuf-c/gp_state.pb-c.h"
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
	GpState *gp_state;
	
	// Read packed message from standard-input.
	uint8_t buf[MAX_MSG_SIZE];
	size_t msg_len = read_buffer(MAX_MSG_SIZE, buf);
	
	// Unpack the message using protobuf-c.
	gp_state = gp_state__unpack(NULL, msg_len, buf);	
	if (gp_state == NULL)
	{
		fprintf(stderr, "error unpacking incoming message\n");
		exit(1);
	}
	
	// display the message's fields.
	printf("Received: buttons = %d\n\taxes:", gp_state->buttons);
	for (int i = 0; i < gp_state.n_axes; i++) {
		printf("\t%f", gp_state->axes[i]);
	}
	printf("\n");

	// Free the unpacked message
	gp_state__free_unpacked(gp_state, NULL);
	return 0;
}