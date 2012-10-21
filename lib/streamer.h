#ifndef __STREAMER__
#define __STREAMER__

#include <sys/types.h>
#include <getopt.h>



/* Instance state declaration */
typedef enum { STOP = 0, RUN = 1 } state_t;

/* Streamer entry point signature */
typedef int (*streamer_t)(int, state_t const*, char const**);



/* Streamer declaration macro */
#define STREAMER(entry_point, initialize, uninitialize)\
	void __##entry_point (streamer_t* entry,\
			void(**init)(streamer_t),\
			void(**uninit)(streamer_t)\
			){\
		*entry= entry_point ;\
		*init= initialize ;\
		*uninit= uninitialize ;\
		}



/* Register an argument to the streamer entry point. */
int register_argument(struct option argument);

#endif
