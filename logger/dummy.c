#include "logger.h"

int main()
{
	logger_init(DEV_HANDLER);
	
	log_runtime(DEBUG, "hi!");
	
	logger_stop();
	
	return 0;
}