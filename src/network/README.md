network library
===================================================================

Implements code to send bits and blips over the network.
The general design is that there is a factory function which
returns a new message_queue object that implements the 
message_queue interface.

Currently only boost asio is supported at the moment.

file summary
===================================================================

connection          
-------------------------------------------------------------------

Interface to a connection (UDP or TCP)

connection_manager          
-------------------------------------------------------------------
API to easily send data to endpoints whether UDP or TCP. Handles
binding the UDP and TCP listening ports and TCP connection pools.

Also does multiplexing across UDP and open TCP connections on receive.


message_queue       
-------------------------------------------------------------------

Main interface for creating a network queue.
Contains the message_queue interface.

tcp_queue          
-------------------------------------------------------------------

TCP queue implemented using boost asio library. 
Implements the message_queue interface.

udp_queue          
-------------------------------------------------------------------

UDP queue implemented using boost asio library. 
Implements the message_queue interface.

connection_manager 
-------------------------------------------------------------------

Manages boost_asio TCP/UDP connections. Opens a listen
socket and has a socket pool for outbound connections on
the same port. Had a send and receive function which
multiplexes and demultiplexes messages between all open
connections

stun_gun              
-------------------------------------------------------------------
Implementation of a simple STUN client to get ip and port 
information. Currently uses QT's UDP sockets but will eventually
change it to use boost asio udp.

endpoint          
-------------------------------------------------------------------

Code to handle endpoint parsing

util          
-------------------------------------------------------------------

Misc network utility code
