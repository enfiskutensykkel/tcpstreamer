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
the program listens to / streams to, use the `-p` option. You can set the
stream duration in seconds if you supply the `-t` option (`-t 0` means "run forever").
You can also use the following command for more program invokation options:

		./tcpstreamer -h [-s streamer]



### Compiling and building the project ###
I _think_ that any C99 compliant C compiler would be able to compile the code,
although I can't guarantee that I'm not relying on some GNU99 specific features.

I rely on my build script for dividing logically the streamers from the core 
program, so I recommend that you use the following when building unless you're
able to replicate the building process:
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

I also use POSIX signals, POSIX threads (pthreads) and POSIX dynamic linking
(dynamic libraries / shared objects), meaning that you need to use a system
that supports these.
I also use POSIX signals and POSIX threads (pthreads), so you need to have a 
system that supports these.
If you're using Windows, you might want to look into [Cygwin](http://www.cygwin.com/install.html),
although I'm afraid that not all features are reliable.



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

While core program source code can all be placed in different subdirectories
and can be scattered all across, streamers must be put in a special 
subdirectory in the source directory in order for the build script to 
recognize it as a streamer implementation. Streamers are given the same name
as their relative root directory.
Streamers must also contain a streamer *entry point* with the following signature:

```C
int streamer(int connection, const int* run_condition)
```

Now, lets explain the arguments: The first argument, ``connection``, is the
connection socket descriptor, and it is ready to use when the entry point is
invoked. The next argument, the ``run_condition`` pointer, is used for indicating
when your streamer should be aborted. It will be set to ``true`` when the entry
point is invoked, and can be set to ``false`` during execution of the streamer,
so beware! 

Your streamer can choose to return before ``run_condition`` is set to ``false``,
and can return with whatever status code it likes; the tcpstreamer program
will do nothing with it except output what the code was. When the entry point
returns, it is assumed that the streamer has completed, and the program will
terminate shortly thereafter.


#### Example streamer ####
Here is an example streamer, lets pretend that the source file is located
at ``src/streamers/example_streamer/source.c``. The first argument to the entry
point is a socket descriptor, all ready to use. The next argument, as explained
above, indicates whether the streamer should continue or stop running. It is 
fully acceptable for the streamer to stop by itself. This streamer would be 
invoked with the command ``./tcpstreamer -s example_streamer hostname``,
where ``hostname`` is the name of the host running a receiver instance.

```C
#include <unistd.h>

/* Stream the numbers 1, 2, 3, ... to a receiver */
int streamer(int conn, const int* run)
{
	ssize_t total_sent = 0;
	int counter = 0;

	while (*run) {
		total_sent += write(conn, (void*) &counter, sizeof(int));
		counter++;
	}

	return 0;
}
```


### Taking arguments from command line ###
It is also possible for a streamer to use parameters provided by the user.
To set this up, you need to declare a special streamer *bootstrapper* function
in addition to the streamer entry point, declared as the following:

```C
void streamer_init(void)
```

This bootstrapper function can be used to *register* command line arguments to
the streamer. This is done by calling the function ``register_argument``.
There are two types of parameters that the streamer can accept: **flags** and
**arguments**. Flags are on the form `--flag` and work simply as switches, 
while arguments are on the form `--argument=value`.

```C
int register_argument(const char* param_name, int* ref, int value)
```

Now, the first argument to the function, ``param_name``, is the name of the
parameter. This must be a `\0`-terminated string. The second argument, if
not `NULL`, indicates a flag. If the flag is set, then `ref` will, upon
streamer invokation, contain the value set in `value`. If `ref` is `NULL`,
this indicates an argument. In this case, the argument is mandatory if `value`
is non-zero.

Argument values are passed on to the streamer entry point in a vector, much
like `argv` is for a main function, however there is a difference: Only the
value of the argument is passed on, and its index is the same as the order
you registered arguments. If an argument is not given on program
invokation, it will be set to ``NULL`` when invoking the entry point function.



#### Example file streamer ####
```C
#include "bootstrap.h" // NB! We need the register_argument() signature
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <getopt.h>
#include <assert.h>

/* Stream a file given to the program as an argument using --file=filename */
int file_streamer(int conn, const int *run, const char **args)
{
	int bufsz = 1460;
	FILE *fp = NULL;
	char *buf = NULL;
	ssize_t len;

	/* Parse arguments */
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

	/* Run streamer */
	while (*run == RUN && !feof(fp) && !ferror(fp)) {

		/* read from file */
		len = fread(buf, sizeof(char), bufsz, fp);

		/* send to receiver */
		send(conn, buf, len, 0);
	}

	free(buf);
	fclose(fp);

	return 0;
}

/* Register arguments for the file streamer */
void streamer_init()
{
	register_argument("file", NULL, 1); // mandatory
	register_argument("buffer-size", NULL, 0); // optional 
}

```
