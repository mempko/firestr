messages library
===================================================================

This library stores a collection of message types that transcends 
components and application instances. All messages are serialized
to and from the mencode wire format.

file summary
===================================================================

sender  
-------------------------------------------------------------------
Utility to make it easier to send messages to specific contacts.
It uses the user_service to address information about a contact.

greeter 
-------------------------------------------------------------------
Messages used to communicate with the firegreet service 

new_app 
-------------------------------------------------------------------
Messages used to communicate applications between firestr instances.
