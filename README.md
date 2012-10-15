streamer
========
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

		./streamer

To start a streamer instance, invoke the program with:

		./streamer -s implementation hostname

Where implementation is the name of an available streamer implementation, and 
hostname is either the hostname or IPv4 address of the host running a receiver
instance. The default port is 50000, if you want to change the port the program
listens to / streams to, use the `-p` option.

Depending on the streamer implementation, you might need to pass arguments to
the streamer program. To see a list of available streamer implementations and
arguments they require, see

		./streamer --help [implementation]



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
* pthreads
* libpcap


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
parts, one is the **core** that provides core components (receiver, packet sniffers, etc)
and functionality for bootstrapping, while the other part are the actual **streamer implementations**.
The main idea is that a developer can create his own streamer implementation 
without needing to touch the core source code of this utility; in other words
much like creating plug-ins for a program.

### Creating a streamer ###

All streamer source code must be put in the `streamers` directory in order for
the build script to recognize it as a streamer implementation.

TODO explain the entry point and signal handler
TODO mention no namespaces
TODO say something about the macro
TODO say something about the autogenerated code

#### Example streamer implementation ####
```C
#include "streamer.h" // Need this in order to get the streamer_t typedef and STREAMER macro
#include <unistd.h>
#include <stdarg.h>

static int run = 1;

/* Streamer entry point */
ssize_t example_streamer(int socket_desc, ...);

/* Streamer signal handler */
void example_signal_handler(streamer_t running_implementation);

/* Declare streamer entry point and signal handler */
STREAMER(example_streamer, example_signal_handler) 



/* Stream the numbers 1, 2, 3, ... to a receiver */
ssize_t example_streamer(int socket_desc, ...)
{
	ssize_t total_sent = 0;
	int counter = 0;

	while (run) {
		total_sent += write(socket_desc, (void*) &counter, sizeof(int));
		counter++;
	}

	return total_sent;
}

/* Stop streamer instance */
void example_signal_handler(streamer_t running_implementation)
{
	if (running_implementation == &example_streamer) {
		run = 0;
	}
}
```

### Taking arguments from command line ###

TODO mention bootstrap
TODO say something about varargs and that you may notice the ... in the signature
TODO mention init and uninit


### Available API ###

#### Core components ###
