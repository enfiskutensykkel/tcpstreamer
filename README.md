tcpstreamer
===========
An utility program to set up TCP streams with the characteristics similar to 
those of streams created by real-time and interactive applications,
and to see how these streams behave in competition with both each other and
with so-called "greedy" streams. 

Please note that before trying to understand what this utility is for, 
you should have a fairly good understanding of how TCP works:
how it divides data into segments, how acknowledgements work and especially
how it handles retransmissions and the retransmission time-out calculation, 
and when and how exponential back-off applies.
You should also have an in-depth knowledge about the congestion avoidance mechanisms (i.e. Fast 
Retransmit and Fast Recovery). 

The purpose of this utility program is to test how the various congestion
avoidance techniques (and other reliability mechanisms for that matter) in TCP 
affects streams with the characteristics of traffic generated by interactive
applications, such as MMOGs or VoIP. These traffic patterns are very different
from traffic created by more "traditional" TCP applications, so-called "greedy"
streams, e.g. HTTP and FTP, which TCP congestion avoidance originally was
designed for.

For more information regarding the problem area, please refer [the litterature](http://www.duo.uio.no/sok/work.html?WORKID=99354&fid=55350).



Usage
-----
You can either start this program as a receiver instance, in which case it will
accept incoming TCP connections and act as a traffic sink, or you can start 
this program as a streamer instance, loading one of the available streamer
implementations.

To start a receiver instance, simply invoke the program with:

		./tcpstreamer

To start a streamer instance, invoke the program with:

		./tcpstreamer -s streamer hostname

Where `streamer` is the name of an available streamer implementation, and 
`hostname` is either the hostname or IPv4 address of the host running a 
receiver instance. The default port is 50000, if you want to change the port
the program listens to / streams to, use the `-p` option.
You can also use the following command for more program invokation options:

		./tcpstreamer -h [-s streamer]



### Compiling and building the project ###
I _think_ that any C99 compliant C compiler would be able to compile the code,
although I can't guarantee that I'm not relying on some GNU99 specific features.

I do, however, rely on a build script (Makefile) to parse the streamer 
source files and make some auto-generated code. So unless you replicate the
building process in some other way, I recommend that you use:
* GNU Make 
* GCC
* sh or bash

If you're running a Linux distro, this shouldn't be a problem. Simply type
`make all` into the terminal in the root project directory. Most distros either
include GCC and Make by default, or have it easily available (typical in a 
package called `build-essentials` or something similar).

You also need the following libraries to be installed and working on your
system:
* libpcap

I also use POSIX signals and POSIX threads (pthreads), so you need to have a 
system that supports these.
If you're using Windows, you might want to look into [Cygwin](http://www.cygwin.com/install.html).



Program design
--------------
The purpose of this program is to set up a [TCP](http://en.wikipedia.org/wiki/Transmission_Control_Protocol)
byte stream from a sender  instance to a receiver instance running on two 
different hosts in a network or test-bed. Today, a great variety of 
applications and use cases for TCP exists, from file transfer applications and
video streaming, to MMOGs and VoIP applications. In other words, TCP is used 
for streaming with the intention of maximizing throughput, to streaming with 
the intention of interactivity. The problem with TCP is that it was designed at
a time where reliability and congestion avoidance were important issues, and 
when there existed few  applications that had requirements for interacitivy and
real-time traffic.

Now, if you're thinking to yourself "*but [UDP](http://en.wikipedia.org/wiki/User_Datagram_Protocol#Reliability_and_congestion_control_solutions) is used where interactivity is more important than congestion avoidance and reliability (packet loss and  packet ordering)*",
then let me just stop you right there!
Because of mechanisms such as [NAT](http://en.wikipedia.org/wiki/Network_address_translation)
and the little support for [NAT Traversal](http://en.wikipedia.org/wiki/NAT_traversal)
in consumer routers, UDP simply isn't feasible to set up. Because of that 
exact reason, many applications that you'd think would use UDP does infact
use TCP. If you don't believe me, try running `netstat` the next time you're
on Skype, for example. TCP avoids problems introduced by NAT, simply because
it is full duplex; data can flow in both directions.

Back on topic: Since these very different scenarios exists, but they all are 
using the same protocol, it might be of interest to test how TCP performs
considering these different characteristics. That's why I've implemented this
utility program with the intention of it being easy to extend with additional
streaming characteristics. I have therefore separated the source code in two
parts, one is the **core** that provides core components (receiver, packet sniffer, CLI parser, etc)
and functionality for bootstrapping streamers, while the other part are the actual **streamer implementations**.
The main idea is that a developer can create his own streamer implementation 
without needing to touch the core source code of this utility; in other words
much like creating plug-ins for a program.

**NB!** Because of limitations with libpcap, this program only works with
IPv4 at this point.


### Creating a streamer ###

All streamer source code must be put in the `streamers` directory in order for
the build script to recognize it as a streamer implementation. All streamers
must have an entry point declared with the following signature:

```C
int streamer_entry_point(int connection, const state_t *run_state, const char **arguments)
```

Now, lets explain the arguments: The first argument, ``connection``, is the
connection socket descriptor, and it is ready to use when the entry point is
invoked. The next argument, the ``run_state`` pointer, is used for indicating
when your streamer should be aborted. It will be set to ``RUN`` when the entry
point is invoked, and can be set to ``STOP`` during execution of the streamer,
so beware! The third argument, ``arguments`` are the parameter values passed
on to the streamer from command line/program invokation. I will explain later
on how this works.

Your streamer can choose to return before ``run_state`` is set to ``STOP``,
and can return with whatever status code it likes; the tcpstreamer program
will do nothing with it except output what the code was. When the entry point
returns, it is assumed that the streamer has completed, and the program will
terminate shortly thereafter.

To define your function as your streamer entry point, you must use the 
following preprocessor macro definition:
```C
STREAMER(streamer_entry_point, NULL, NULL)
```
I will describe how this macro works later, but to briefly said it must be 
there so that the build script and the program can recognize your custom
function as your streamer entry point. As I said, I rely on some 
auto-generated code, and parsing the source code to find streamers is part
of that operation.


#### Example streamer ####
Here is an example streamer. The first argument to the entry point is a
socket descriptor, all ready to use. The next argument, as explained above,
indicates whether the streamer should continue or stop running. It is fully
acceptable for the streamer to stop by itself. This streamer would be 
invoked with the command ``./tcpstreamer -s example_streamer hostname``,
where ``hostname`` is the name of the host running a receiver instance.

```C
#include "streamer.h" // NB!
#include <unistd.h>

/* Stream the numbers 1, 2, 3, ... to a receiver */
static int example_streamer(int conn, const state_t *run, const char **args)
{
	ssize_t total_sent = 0;
	int counter = 0;

	while (*run == RUN) {
		total_sent += write(conn, (void*) &counter, sizeof(int));
		counter++;
	}

	return 0;
}

/* Expose streamer entry point */
STREAMER(example_streamer, NULL, NULL) 
```

Note that I declared the entry point function as static. This is recommended,
as C doesn't have what is known as *namespaces*, so if another function 
somewhere in the source code was named ``example_streamer``, declaring this 
one as static will prevent a possible symbol "collision".


### Taking arguments from command line ###
As mentioned previously, it is possible to take arguments given to the program
at invokation. To set up this, you must declare an *initialization* function 
for your streamer. This is done by providing a function as the second argument
to the ``STREAMER`` macro.

This initialization function must take the streamer entry point as an argument
(in case you want to use the same initialization function for multiple 
streamers) and can *register* streamer arguments. This is used by passing
getopt long option-style options (see the reference for ``getopt_long()``) to a
special register function, with the following signature:
```C
int register_argument(struct option parameter)
```
The register function will return the number of arguments previously
registered.

```C
#include "streamer.h"
#include "bootstrap.h" // NB!
#include <getopt.h>
#include <stdio.h>

static int entry_point(int c, const state_t *s, const char **args)
{
	if (args[0] != NULL)
		printf("string1: %s\n", args[0]);
	else
		puts("string1 was not given");
	
	if (args[1] != NULL)
		printf("string2: %s\n", args[1]);
	else
		puts("string2 was not given");

	return 0;
}

static void bootstrapper(streamer_t streamer)
{
	struct option opts[] = {
		{"string1", 1, 0, 0},
		{"string2", 2, 0, 0}
	};

	if (streamer == &entry_point) {
		register_argument(opts[0]);
		register_argument(opts[1]);
	}
}
```

Note here that the order of registration is also the order that the arguments
is passed on to the entry point. If an argument is not given on program
invokation, it will be set to ``NULL`` when invoking the entry point function.

#### Example file streamer ####
```C
#include "streamer.h"
#include "bootstrap.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <getopt.h>
#include <string.h>
#include <assert.h>

static int simple_streamer(int conn, const state_t *run, const char **args)
{
	int bufsz = 1460;
	FILE *fp = NULL;
	char *buf = NULL;
	ssize_t len;

	/* Parse arguments */
	if (args[0] == NULL) {
		fprintf(stderr, "Must supply filename!\n");
		return -1;
	}

	if ((fp = fopen(args[0], "r")) == NULL) {	
		fprintf(stderr, "Invalid file: '%s'\n", args[0]);
		return -1;
	}

	if (args[1] != NULL && ((bufsz = atoi(args[1])) < 0 || bufsz > 2048)) {
		fprintf(stderr, "Invalid buffer size: '%s'\n", args[1]);
		return -2;
	}

	/* Allocate buffer */
	if ((buf = malloc(sizeof(char) * bufsz)) == NULL) {
		perror("malloc");
		return -3;
	}
	memset(buf, 0, sizeof(char) * bufsz);

	/* Run streamer */
	while (*run == RUN && (fp == NULL || (!feof(fp) && !ferror(fp)))) {

		/* read from file */
		if (fp != NULL) {
			len = fread(buf, sizeof(char), bufsz, fp);
		}

		/* send to receiver */
		send(cond, buf, len, 0);
	}

	free(buf);
	if (fp != NULL)
		fclose(fp);

	return 0;
}

static void bootstrap(streamer_t entry_point)
{
	struct option streamer_args[] = {
		{"file", 1, 0, 0},
		{"bufsz", 1, 0, 0},
	};

	assert(entry_point == &simple_streamer);
	register_argument(streamer_args[0]);
	register_argument(streamer_args[1]);
}


STREAMER(simple_streamer, bootstrap, NULL)
```
Note that here the entry point can return before the state is ``STOP``.
