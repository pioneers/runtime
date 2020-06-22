#include <stdio.h>
#include <stdlib.h>
#include "../protobuf-c/run_mode.pb-c.h"

int main () 
{
	RunMode run_mode = RUN_MODE__INIT; //iniitialize hooray
	void *buf;                     // Buffer to store serialized data
	unsigned len;                  // Length of serialized data
	
	//put some data
	run_mode.msg = MSG__RUN_MODE;
	run_mode.mode = RUN_MODE__MODE__AUTO;
	
	len = run_mode__get_packed_size(&run_mode);
	
	buf = malloc(len);
	run_mode__pack(&run_mode, buf);
	
	fprintf(stderr, "Writing %d serialized bytes\n", len); // See the length of message
	fwrite(buf, len, 1, stdout); // Write to stdout to allow direct command line piping
	
	free(buf); // Free the allocated serialized buffer
	return 0;
}