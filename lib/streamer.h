#ifndef __STREAMER__
#define __STREAMER__



/* Necessary includes */
#include <sys/types.h>



/* Streamer instance signature */
typedef ssize_t (*streamer_t)(int sock_desc, char const **arguments);



/* Streamer declaration macro */
#define STREAMER(entry_point, ...)\
	void __##entry_point (streamer_t* streamer,\
			void(**callback)(streamer_t),\
			void(**init)(streamer_t),\
			void(**uninit)(streamer_t)\
			){\
		void (*func_list[])(streamer_t) = { __VA_ARGS__ };\
		*streamer= entry_point ;\
		*init=*uninit=(void(*)(streamer_t))0;\
		switch(sizeof(func_list)/sizeof(void(*)(streamer_t))){\
			case 3:*uninit = func_list[2];\
			case 2:*init = func_list[1];\
			case 1:*callback=func_list[0];\
		}}

#endif
