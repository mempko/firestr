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

vclock     
-------------------------------------------------------------------

Vector clock/version vector implementation. 

crstring     
-------------------------------------------------------------------

A concurrent string implementation which uses version vectors and a diff3 merge function
to help maintain string replicas. For example the code editor uses this to help assist
in achieving concurrent code editing.

