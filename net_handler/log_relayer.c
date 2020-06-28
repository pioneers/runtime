#include "log_relayer.h"

pthread_t log_relay_tid;
int log_buf_ix = 0; //indexer into args->logs

//insert the contents of buf (n_bytes long), which may contain multiple lines of logs, into the array of lines in args
static void fill_args_buffer (log_relay_args_t *args, char *buf, ssize_t n_bytes, int max_newest_log, int *start_loc)
{
	//go until hard cap of max_newest_log or until we reach the end of the buffer
	for (int i = start_loc; (i < n_bytes) || (args->newest_log_num == max_newest_log); i++) {
		if (buf[i] != '\n') {
			args->logs[args->newest_log_num % (MAX_NUM_LOGS * 2)][log_buf_ix++] = buf[i];
		} else {
			args->newest_log_num++; //go to the next available slot
			log_buf_ix = 0; //reset the indexer into args->logs
		}
	}
	*start_loc = 0;
}

static void *log_relayer (void *args)
{
	log_relay_args_t *log_relay_args = (log_relay_args_t *)args;
	fd_set fifo_read;
	ssize_t n_read;
	char buf[MAX_LOG_LEN];
	int start_loc, prev_newest_log_num;
	int maxfdp1 = log_relay_args->fifo_fd + 1;
	
	FD_ZERO(fifo_read);
	while (1) {
		FD_SET(log_relay_args->fifo_fd + 1);
		
		//use select to block until something becomes available 
		//(setting fifo to read only and then blocking on read() wouldn't allow us to test for EWOULBLOCK)
		if (select(maxfdp1, &fifo_set, NULL, NULL, NULL) < 0) {
			//ignore EINTR
			if (errno == EINTR) {
				continue;
			}
			error("select: @log_relay");
		}

		//acquire mutex
		pthread_mutex_lock(log_relay_args->new_logs_mutex);
		
		//fill args buffer with anything left over from previous loop
		prev_newest_log_num = log_relay_args->newest_log_num;
		fill_args_buffer(log_relay_args, buf, n_read, prev_newest_log_num + MAX_NUM_LOGS, &start_loc)
		
		//read off the FIFO until EWOULDBLOCK into local array or MAX_NUM_LOGS lines; insert into proper location in log_relay_args
		while (log_relay_args->newest_log_num < prev_newest_log_num + MAX_NUM_LOGS) {
			n_read = read(log_relay_args->fifo_fd, buf, MAX_LOG_LEN);
			if (n_read == -1 && (errno == EWOULDBLOCK || errno == EAGAIN)) { //nothing more to read
				break;
			} else if (n_read == -1)
			fill_args_buffer(log_relay_args, buf, n_read, prev_newest_log_num + MAX_NUM_LOGS, start_loc);
		}
		//unlock the mutex, broadcast the condition variable
		pthread_mutex_unlock(log_relay_args->new_logs_mutex);
		pthread_cond_broadcast(log_relay_args->new_logs_cond);
	}
}

//on the other side--
//wait on condition variable
//unlock mutex
//read from current log to newest_log 
//acquire tcp mutex
//send data
//repeat

void start_log_relayer (log_relay_args_t *args)
{	
	//fill in the args structure
	if ((args->fifo_fd = open(FIFO_NAME, O_RDWR | O_NONBLOCK)) == -1) {
		perror("open: net handler could not open log FIFO");
	}
	if (pthread_cond_init(args->new_logs_cond, NULL) != 0) {
		perror("pthread_cond_init: could not initialize log relayer cond var");
	}
	if (pthread_mutex_init(args->new_logs_mutex, NULL) != 0) {
		perror("pthread_mutex_init: could not initialize log relayer mutex");
	}
	args->newest_log_num = 0;
	args->logs = (char **) malloc(sizeof(char *) * MAX_NUM_LOGS * 2);
	for (int i = 0; i < MAX_NUM_LOGS * 2; i++) {
		args->logs[i] = (char *) malloc(sizeof(char) * MAX_LOG_LEN);
	}
	
	//create thread
	if (pthread_create(&log_relay_tid, NULL, log_relayer, args) != 0) {
		error("pthread_create: @log_relay");
		return;
	}
	
}

void stop_log_relayer ()
{
	if (pthread_cancel(log_relay_tid) != 0) {
		error("pthread_cancel: @log_relay");
	}
	if (pthread_join(log_relay_tid, NULL) != 0) {
		error("pthread_join: @log_relay");
	}
	
	//deinitialize all the things in args
	//free all the things in args
	
	log_runtime(INFO, "stopped log relayer thread");
}