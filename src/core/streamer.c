#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include "instance.h"


/* Make passing arguments to thread easier */
struct thread_arg
{
	streamer_t entry_point;
	pthread_mutex_t state_mutex;
	pthread_cond_t stopped;
	int connection;
	int const *condition;
	char const **arguments;
	int *status;
};



/* Hold the execution state of the streamer thread */
static enum { RUNNING, STOPPED } streamer_state;




/* Load dynamic library file / shared object file and symbols */
int load_streamer(char const *name, streamer_t *entry, callback_t *init)
{

	char *filename = NULL;
	void *handle;

	/* Construct file name */
	if ((filename = malloc(strlen(DEF_2_STR(STREAMER_DIR)) + strlen(name) + 2)) == NULL)
		return -3;
	strcpy(filename, DEF_2_STR(STREAMER_DIR));
	strcat(filename, "/");
	strcat(filename, name);

	/* Load dynamic library file */
	handle = dlopen(filename, RTLD_LAZY | RTLD_LOCAL | RTLD_NODELETE);
	if (handle == NULL) {
		free(filename);
		return -1;
	}
	free(filename);

	/* Load symbol for entry point */
	*entry = (streamer_t) dlsym(handle, "streamer");
	if (*entry == NULL)
		return -2;

	/* Load symbols for initialization function */
	dlerror();
	*init = (callback_t) dlsym(handle, "streamer_create");
	if (dlerror() != NULL)
		*init = NULL;

	/* Close dynamic library file */
	dlclose(handle);
	return 0;
}



/* Run streamer thread */
static void* run_streamer(struct thread_arg *arg)
{
	int status;
	
	/* Set the thread to be cancelable */
	assert(!pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &status));
	assert(!pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &status));

	/* Call streamer entry point */
	status = arg->entry_point(arg->connection, arg->condition, arg->arguments);
	*(arg->status) = status;

	/* Notify that we are done */
	pthread_mutex_lock(&arg->state_mutex);
	streamer_state = STOPPED;
	pthread_cond_signal(&arg->stopped);
	pthread_mutex_unlock(&arg->state_mutex);

	/* Exit thread */
	pthread_exit(&arg->status);
}



/* Start streamer thread */
int streamer(streamer_t entry, unsigned dur, int conn, int *cond, char const **args)
{
	pthread_t thread;
	pthread_attr_t attr;
	struct thread_arg *th_arg;
	struct timespec timeout = {0, 100 * 1000 * 1000};
	unsigned utime = dur * 1000;
	int status = -1;

	/* Initialize thread arguments */
	if ((th_arg = malloc(sizeof(struct thread_arg))) == NULL)
		return -1;

	th_arg->entry_point = entry;
	th_arg->connection = conn;
	th_arg->condition = cond;
	th_arg->arguments = args;
	th_arg->status = &status;

	assert(!pthread_mutex_init(&th_arg->state_mutex, NULL));
	assert(!pthread_cond_init(&th_arg->stopped, NULL));

	/* Set thread to be joinable */
	assert(!pthread_attr_init(&attr));
	assert(!pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE));

	/* Start thread */
	streamer_state = RUNNING;
	assert(!pthread_create(&thread, NULL, (void* (*)(void*)) &run_streamer, th_arg));


	/* Count down duration down */
	while (*cond) {
		
		if (pthread_mutex_trylock(&th_arg->state_mutex) == 0) {
			if (streamer_state != RUNNING)
				*cond = 0;
			pthread_mutex_unlock(&th_arg->state_mutex);
		}

		if (dur != 0) {
			if (utime == 0)
				*cond = 0;
			else {
				nanosleep(&timeout, NULL);
				utime -= 100;
			}
		}
	}


	/* Wait for streamer thread to complete */
	pthread_mutex_lock(&th_arg->state_mutex);
	if (streamer_state == RUNNING) {

		assert(clock_gettime(CLOCK_REALTIME, &timeout) == 0);
		timeout.tv_nsec += 500 << 20; // ~.5 seconds

		if (pthread_cond_timedwait(&th_arg->stopped, &th_arg->state_mutex, &timeout) == ETIMEDOUT) {
			pthread_cancel(thread);
		}
	}
	pthread_mutex_unlock(&th_arg->state_mutex);


	/* Streamer thread rendezvous and resource freeing */
	pthread_join(thread, (void**) &th_arg->status);
	pthread_attr_destroy(&attr);
	pthread_cond_destroy(&th_arg->stopped);
	pthread_mutex_destroy(&th_arg->state_mutex);
	free(th_arg);

	return status;
}
