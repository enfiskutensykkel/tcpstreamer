#ifndef __STREAMER__
#define __STREAMER__

#include <sys/types.h>


typedef enum { STOP = 0, RUN = 1 } state_t;

/* Streamer instance signature */
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

#endif
