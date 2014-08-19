conversation library
===================================================================

Library provides the model for the conversation state and service which
manipulates it.

A conversation is an instance of communication between a set of people.

This needs to be fleshed out. I am thinking of doing a reference 
counted setup for the life of a conversation. Right now if you leave
a conversation, you cannot come back.

file summary
===================================================================

conversation         
-------------------------------------------------------------------

Provides the model for the conversation state.

conversation_service 
-------------------------------------------------------------------

Service which handles creation and synchronization
of conversations between users.
              
