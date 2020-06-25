#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../pbc_gen/text.pb-c.h"

#define MAX_STRLEN 100

char* strs[4] = { "hello", "beautiful", "precious", "world" };

int main () 
{
	Text log_msg = TEXT__INIT; //iniitialize hooray
	void *buf;                     // Buffer to store serialized data
	unsigned len;                  // Length of serialized data
	
	//put some data
	log_msg.n_payload = 4;
	log_msg.payload = (char **) malloc (sizeof(char *) * log_msg.n_payload);
	for (int i = 0; i < log_msg.n_payload; i++) {
		log_msg.payload[i] = (char *) malloc(sizeof(char) * strlen(strs[i]));
		strcpy(log_msg.payload[i], (const char *) strs[i]);
	}
	
	len = text__get_packed_size(&log_msg);
	
	buf = malloc(len);
	text__pack(&log_msg, buf);
	
	fprintf(stderr, "Writing %d serialized bytes\n", len); // See the length of message
	fwrite(buf, len, 1, stdout); // Write to stdout to allow direct command line piping
	
	free(buf); // Free the allocated serialized buffer
	for (int i = 0; i < log_msg.n_payload; i++) {
		free(log_msg.payload[i]);
	}
	free(log_msg.payload);
	return 0;
}