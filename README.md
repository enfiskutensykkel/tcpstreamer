streamer
========

An utility program to set up TCP streams with the characteristics of 
real-time/interactive traffic, and see how these streams behave in competition
with both each other and with so-called "greedy" streams, the latter which is
the characteristics of more "traditional" applications such as HTTP and FTP.

Please note that before trying to understand what this utility is for, 
you should have a fairly good understanding of how TCP works:
how it divides data into segments, how acknowledgements work and especially
how it handles retransmissions and the retransmission time-out calculation, 
and when and how exponential back-off applies.
You should also have an in-depth knowledge about the congestion avoidance mechanisms (i.e. Fast 
Retransmit and Fast Recovery). 

For more information regarding the problem area, please refer [the litterature](http://www.duo.uio.no/sok/work.html?WORKID=99354&fid=55350).*
