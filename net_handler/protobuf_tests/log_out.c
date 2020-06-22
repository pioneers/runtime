#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../protobuf-c/text.pb-c.h"

#define MAX_STRLEN 100

char* strs[4] = { "hello", "beautiful", "precious", "world" };

int main () 
{
	Text log_msg = TEXT__INIT; //iniitialize hooray
	void *buf;                     // Buffer to store serialized data
	unsigned len;                  // Length of serialized data
	unsigned total_size = 0;		// total length of strings
	
	//put some data
	log_msg.msg = MSG__LOG;   //this is how you do enums
	log_msg.n_payloads = 4;
	for (int i = 0; i < log_msg.n_payloads; i++) {
		total_size += (int) strlen(strs[i]);
	}
	log_msg.payloads = (char **) malloc (total_size * sizeof(char *));
	for (int i = 0; i < log_msg.n_payloads; i++) {
		log_msg.payloads[i] = strs[i];
	}
	
	len = text__get_packed_size(&log_msg);
	
	buf = malloc(len);
	text__pack(&log_msg, buf);
	
	fprintf(stderr, "Writing %d serialized bytes\n", len); // See the length of message
	fwrite(buf, len, 1, stdout); // Write to stdout to allow direct command line piping
	
	free(buf); // Free the allocated serialized buffer
	free(log_msg.payloads);
	return 0;
}