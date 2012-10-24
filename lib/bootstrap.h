#ifndef __BOOTSTRAP__
#define __BOOTSTRAP__

/* Register argument.
 *
 * Register an command line argument which takes a parameter value.
 * Arguments are given on program invokation as either --param=value
 * or --flag
 * 
 * The first argument, name, refers to the argument name. The second and third
 * arguments are used as follows: If reference points to an address (not NULL),
 * then the argument is a flag, meaning that it accepts no value argument.
 * In this case, reference will be set to value, if the flag is set.
 *
 * If reference is NULL, then the argument is a parameter, meaning that a value
 * must also be given to the argument. In this case, value indicates whether
 * the parameter is mandatory or not.
 * 
 * Note that the order in which parameters are registered are also the order
 * they are passed on to the streamer. If a parameter is not set at command
 * line, the argument passed on to the streamer at that index will be set to
 * NULL.
 *
 * See the README file for more information.
 */
int register_argument(const char* name, int* reference, int value);

#endif
