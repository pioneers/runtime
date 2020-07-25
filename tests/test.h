#ifndef TEST_H
#define TEST_H

#include "client/net_handler_client.h"
#include "client/shm_client.h"
#include "client/executor_client.h"
//#include "client/dev_handler_client.h"

//routes stdout of test process to a temporary file to collect output from all the clients
void start_test (char *test_name);

//this is called when the test has shut down all runtime processes and is ready to compare output
void end_test ();

//returns true if exepcted output matches output exactly
void match_all (char *expected_output);

//returns true if expected output is somewhere in the output
void match_part (char *expected_output);

#endif
