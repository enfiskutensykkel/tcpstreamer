#include "streamer.h"
#include "streamerctl.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include <assert.h>


/* Handle the auto-generated code */
extern struct {
	char const *name;
	void (*callback)(
			streamer_t*,
			void (**)(streamer_t),
			void (**)(streamer_t)
			);
} __streamers[];



/* Make passing arguments to thread easier */
struct thread_arg
{
	streamer_t entry_point;
	pthread_mutex_t state_mutex;
	pthread_cond_t stopped;
	int connection;
	cond_t const *condition;
	char const **arguments;
	int *status;
};



/* Hold the execution state of the streamer thread */
static enum { RUNNING, STOPPED } streamer_state;



/* Bootstrap all streamers and create streamer table */
int streamer_tbl_create(tblent_t **tbl)
{
	int i, n;

	/* Count the number of streamers */
	for (n = 0; __streamers[n].callback != NULL; ++n);

	/* Allocate memory for streamer table */
	*tbl = malloc(sizeof(tblent_t) * (n+1));
	if (*tbl == NULL)
		return -1;

	/* Bootstrap all streamers */
	for (i = 0; i < n; ++i) {
		(*tbl[i]).name = __streamers[i].name;
		__streamers[i].callback( &((*tbl[i]).streamer), &((*tbl[i]).init), &((*tbl[i]).uninit) );
	}

	/* Set the last table entry to be zero */
	memset((*tbl)+n, 0, sizeof(tblent_t));

	/* Return numbers of table entries */
	return n;
}



/* Destroy the streamer table */
void streamer_tbl_destroy(tblent_t *tbl)
{
	if (tbl != NULL) {
		free(tbl);
	}
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
	if (arg->status != NULL)
		*(arg->status) = status;

	/* Notify that we are done */
	pthread_mutex_lock(&arg->state_mutex);
	streamer_state = STOPPED;
	pthread_cond_signal(&arg->stopped);
	pthread_mutex_unlock(&arg->state_mutex);

	/* Exit thread */
	pthread_exit(NULL);
}



/* Start streamer thread */
int streamer(tblent_t *entry, unsigned dur, int conn, cond_t *cond, char const **args)
{
	pthread_t thread;
	pthread_attr_t attr;
	struct thread_arg *th_arg;
	struct timespec timeout = {0, 100 * 1000 * 1000};
	unsigned utime;

	/* Initialize thread arguments */
	if ((th_arg = malloc(sizeof(struct thread_arg))) == NULL)
		return -1;

	th_arg->entry_point = entry->streamer;
	th_arg->connection = conn;
	th_arg->condition = cond;
	th_arg->arguments = args;
	th_arg->status = NULL;

	assert(!pthread_mutex_init(&th_arg->state_mutex, NULL));
	assert(!pthread_cond_init(&th_arg->stopped, NULL));

	/* Set thread to be joinable */
	assert(!pthread_attr_init(&attr));
	assert(!pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE));

	/* Start thread */
	streamer_state = RUNNING;
	assert(!pthread_create(&thread, NULL, (void* (*)(void*)) &run_streamer, th_arg));


	/* Count down duration down */
	utime = dur * 1000;
	while (*cond == RUN) {
		
		if (pthread_mutex_trylock(&th_arg->state_mutex) == 0) {
			if (streamer_state != RUNNING)
				*cond = STOP;
			pthread_mutex_unlock(&th_arg->state_mutex);
		}

		if (dur != 0) {
			if (utime == 0)
				*cond = STOP;
			else {
				nanosleep(&timeout, NULL);
				utime -= 100;
			}
		}
	}


	/* Wait for streamer thread to complete */
	pthread_mutex_lock(&th_arg->state_mutex);
	if (streamer_state == RUNNING) {

		timeout.tv_sec = 0;
		timeout.tv_nsec = 50 << 20;

		if (pthread_cond_timedwait(&th_arg->stopped, &th_arg->state_mutex, &timeout) == ETIMEDOUT) {
			pthread_cancel(thread);
		}
	}
	pthread_mutex_unlock(&th_arg->state_mutex);


	/* Streamer thread rendezvous and resource freeing */
	pthread_join(thread, NULL);
	pthread_attr_destroy(&attr);
	pthread_cond_destroy(&th_arg->stopped);
	pthread_mutex_destroy(&th_arg->state_mutex);
	free(th_arg);

	return 0;
}
