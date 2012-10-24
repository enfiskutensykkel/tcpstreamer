#ifndef __INSTANCE__
#define __INSTANCE__

/* Macro for turning a define into string 
 * Use this to convert the streamer defines into strings.
 */
#define STRINGIFY(str) #str
#define DEF_2_STR(str) STRINGIFY(str)



/* Function pointer to a streamer entry point.
 *
 * A streamer should take the socket descriptor as its first argument, 
 * a pointer to the run condition (false => streamer should quit) as the 
 * second and the argument values given from CLI as the third argument (see
 * how bootstrapping with register_argument() works for a description of this).
 *
 * A streamer should run either to completetion, or until the run condition is
 * set to false. This means that the streamer must continously check if 
 * condition is false.
 *
 * When done, the streamer can return a status code which the program will then
 * terminate with.
 */
typedef int (*streamer_t)(int conn, int const *cond, char const **args);



/* Function pointer to a streamer initialization function.
 *
 * A streamer can implement custom initialization function to set up
 * necessary variables and whatever I haven't been able to foresee. The
 * initialization function can also call the register_argument() to register
 * CLI arguments for the streamer (see the README for how to do this).
 *
 * The streamer entry point selected is given as an argument when calling
 * the callback in case the same initialization function is used for multiple
 * streamers.
 */
typedef void (*callback_t)(streamer_t entry_point);



/* Load streamer functions from shared object file.
 *
 * Load a shared object file with file name given by the name argument,
 * and load the symbols for the entry point and initialization function. 
 * The latter can be set to NULL, but entry_point will always be set 
 * (otherwise it is an invalid streamer).
 * 
 * Returns 0 on success, -1 if supplied file is invalid or -2 if symbol isn't
 * found.
 */
int load_streamer(void **handle, char const *name, streamer_t *entry_point, callback_t *init);



/* Unload streamer functions
 *
 * After loading and running the shared object file, unload the symbols.
 */
void unload_streamer(void *handle);



/* Streamer control.
 *
 * After loading a streamer entry point symbol, call it by passing it on to
 * this control function. This function spawns a new thread which calls the 
 * streamer entry point. The connection, condition and args arguments
 * are passed on directly to the streamer.
 *
 * A streamer will run either until completion (that is, the streamer entry 
 * point returns) or until it is cancelled. If duration is non-zero, the
 * streamer will run for maximum that amount of seconds. If duration is
 * zero, then the streamer will run until condition is set to false otherwise.
 * If the streamer doesn't return after condition changes to zero, this
 * control function will cancel the streamer thread, forcing the streamer
 * to quit.
 *
 * Returns the return value from the streamer.
 */
int streamer(streamer_t entry, unsigned dur, int conn, int *cond, char const **args);



/* Receiver control
 *
 * Start a receiver which accepts new connections and receive bytes from active
 * connections. It will stop when condition is set to zero.
 */
void receiver(int conn, int *cond);

#endif
