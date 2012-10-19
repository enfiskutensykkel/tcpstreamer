#ifndef __STREAMER__
#define __STREAMER__

#include <sys/types.h>


typedef enum { RUN, STOP } state_t;

/* Streamer instance signature */
typedef int (*streamer_t)(int, state_t const*, char const**);


/* Streamer declaration macro */
#define __STREAMER(entry_point, ...)\
	void __##entry_point (streamer_t* entry,\
			void(**init)(streamer_t),\
			void(**uninit)(streamer_t)\
			){\
		void (*func_list[])(streamer_t) = { __VA_ARGS__ };\
		*entry= entry_point ;\
		*init=*uninit=(void(*)(streamer_t))0;\
		switch(sizeof(func_list)/sizeof(void(*)(streamer_t))){\
			case 2:*uninit=func_list[1];\
			case 1:*init=func_list[0];\
		}}

#define STREAMER(...) __STREAMER( __VA_ARGS__ )

#endif
