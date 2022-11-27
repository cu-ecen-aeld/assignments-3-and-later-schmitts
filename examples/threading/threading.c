#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{
	struct thread_data* thread_func_args = (struct thread_data *) thread_param;

	// wait
	usleep(1000*thread_func_args->wait_to_obtain_ms);

	// obtain mutex
	pthread_mutex_lock(thread_func_args->mutex);

	// wait
	usleep(1000*thread_func_args->wait_to_release_ms);

	// release
	pthread_mutex_unlock(thread_func_args->mutex);

	thread_func_args->thread_complete_success = true;

    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */

	struct thread_data* thread_param = malloc(sizeof(struct thread_data));

	thread_param->mutex = mutex;
	thread_param->wait_to_obtain_ms = wait_to_obtain_ms;
	thread_param->wait_to_release_ms = wait_to_release_ms;

	int ret = pthread_create(thread, NULL, threadfunc, thread_param);

	if (ret) {
		return false;
	}

    return true;
}

