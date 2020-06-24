#include <stdio.h>
#include <stdlib.h>
#include "../pbc_gen/gamepad.pb-c.h"

int main () 
{
	GpState gp_state = GP_STATE__INIT; //iniitialize hooray
	void *buf;                     // Buffer to store serialized data
	unsigned len;                  // Length of serialized data
	
	//put some data
	gp_state.buttons = 47894789;
	gp_state.n_axes = 4;
	gp_state.axes = (double *) malloc(gp_state.n_axes * sizeof(double));
	gp_state.axes[0] = -0.42;
	gp_state.axes[1] = -0.97;
	gp_state.axes[2] = 0.77;
	gp_state.axes[3] = 0.04;
	
	
	len = gp_state__get_packed_size(&gp_state);
	
	buf = malloc(len);
	gp_state__pack(&gp_state,buf);
	
	fprintf(stderr, "Writing %d serialized bytes\n", len); // See the length of message
	fwrite(buf, len, 1, stdout); // Write to stdout to allow direct command line piping
	
	free(buf); // Free the allocated serialized buffer
	return 0;
}