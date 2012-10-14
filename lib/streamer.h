#ifndef __STREAMER__
#define __STREAMER__

/* A user implementing a streamer, this signature must be uesed for the 
 * streamer entry point:
 *    
 *     ssize_t streamer_entry_point(int socket_desc, ...);
 *
 * The streamer function must return the number of bytes successfully
 * transmitted until it has completed or is cancelled, or a value less than
 * zero on failure. It can take any number of arguments of any type (including
 * none at all), but it must take at least one argument, namely the socket
 * descriptor as its first argument.
 *
 *
 * When implementing a streamer, the following macro must be used:
 *
 *    STREAMER(streamer_entry_point, signal_handler_callback)
 *
 * This macro tells the project builder that the function 
 * streamer_entry_point() is a valid streamer entry point and can be called
 * when the program is started. In addition to the main entry point, there are
 * three more functions that can be declared as streamer entry point, namely a
 * signal handler, a initialization function and a uninitialization function.
 * These three functions must have the following signature:
 *
 *     void function(streamer_t running_instance);
 *
 * The running_instance argument is the invoked streamer instance, or in other
 * words, the specified streamer entry point. Use the following syntax to
 * declare that the streamer needs bootstrapping and cleaning up:
 * 
 *     STREAMER(entry_point, signal_handler, init, uninit)
 *
 * The initialization function is called before the entry point is called. Use
 * this to bootstrap and set up necessary parameters for the streamer. The
 * uninitialization function should be used in a similar fashion, to clean up
 * after the streamer is done. These two functions are optional and can
 * be omitted.
 *
 * Your custom signal handler is invoked if the program is terminated. It is 
 * important that this function doesn't block and has a minimal execution time
 * (like any signal handlers). Clean up and waiting for the streamer instance 
 * to run to completion should be done in the unitialization function, and NOT
 * the signal handler.
 *
 * Implementing a signal handler callback function is mandatory, because
 * a streamer can potentially stream data forever. A basic example of a 
 * streamer and a signal handler is as follows:
 *
 * #include "streamer.h"
 * #include <unistd.h>
 * #include <stdarg.h>
 *
 * static int run;
 *
 * ssize_t streamer(int socket_desc, ...)
 * {
 *     run = 1;
 *     ssize_t total_sent = 0;
 *
 *     while (run) {
 *         total_sent += write(socket_desc, "hello", 5);
 *     }
 *
 *     return total_sent;
 * }
 *
 * void sig_handler(streamer_t instance)
 * {
 *     if (instance == streamer)
 *         run = 0;
 * }
 *
 * STREAMER(streamer, sig_handler)
 */



/* Necessary includes */
#include <sys/types.h>
#include <stdarg.h>



/* Streamer instance signature */
typedef ssize_t (*streamer_t)(int sock_desc, ...);



/* Streamer declaration macro */
#define STREAMER(entry_point, ...)\
	void __##entry_point (streamer_t* streamer,\
			void(**callback)(streamer_t),\
			void(**init)(streamer_t),\
			void(**uninit)(streamer_t)\
			){\
		void (*func_list[])(streamer_t) = { __VA_ARGS__ };\
		*streamer= entry_point ;\
		*callback=*init=*uninit=(void(*)(streamer_t)) 0;\
		switch(sizeof(func_list)/sizeof(void(*)(streamer_t))){\
			case 3:*uninit = func_list[2];\
			case 2:*init = func_list[1];\
			case 1:*callback = func_list[0]; \
		}}

#endif
