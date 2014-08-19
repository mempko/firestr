util library
===================================================================

This is a place to put misc code that everything needs.

file summary
===================================================================

dbc         
-------------------------------------------------------------------

Implements some Design By Contract constructs. 

bytes      
-------------------------------------------------------------------

Deals with bytes and converting from bytes to strings, etc

compress   
-------------------------------------------------------------------

Simple functions to compress and uncompress bytes using snappy

queue      
-------------------------------------------------------------------

Implements a thread safe queue.

string     
-------------------------------------------------------------------

Utilities dealing with string handling.

mencode    
-------------------------------------------------------------------

Implements a library to serialize data structures into 
mencode (max encode). This is inspired by bencode used
by BitTorrent. All messages are encoded in this format.

thread     
-------------------------------------------------------------------

Utilities dealing with threads.

uuid       
-------------------------------------------------------------------

Simple utility to generate uuids.

filesystem 
-------------------------------------------------------------------

Friendly file utilities.

disk_store 
-------------------------------------------------------------------

Simple database to store data on disk for an app
