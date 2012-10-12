#ifndef __STREAMER__
#define __STREAMER__

#include <sys/types.h>
#include <stdarg.h>


/* Streamer entry point signature
 *
 * A user implementing a streamer must use this signature for their streamer
 * entry point:
 *
 * ssize_t streamer_entry_poiint(int socket_desc, ...);
 *
 * The streamer function must return the number of bytes successfully
 * transmitted until it has completed or is cancelled, or a value less than
 * zero on failure. It can take any number of arguments of any type (including
 * none at all), but it must take at least one argument, namely the socket
 * descriptor as its first argument.
 *
 * To see how to bootstrap the streamer entry point, see entry.h
 */
typedef ssize_t (*streamer_t)(int sock_desc, ...);



/* Streamer cancellation callback signature
 *
 * When implementing a streamer, the user must also provide a mechanism to
 * stop it. Implement a function with this signature to stop a running streamer
 * instance.
 *
 * This function should not be used to return results or any values, it should
 * simply be used as a signal handler for the streamer. The streamer should
 * then return so that whatever function called the streamer entry point (see
 * above) should regain control and get a return value from the streamer.
 *
 * How this is implemented is up to the user, but it will be called in a
 * similar fashion to a signal handler and should therefore not block for an
 * extensive period of time. The running streamer instance is passed to the 
 * cancellation callback as the first argument.
 */
typedef void (*cancel_t)(streamer_t instance);

#endif
