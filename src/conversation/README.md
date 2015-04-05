conversation library
===================================================================

Library provides the model for the conversation state and service which
manipulates it.

A conversation is an instance of communication between a set of people.
The lifetime of the conversation exists as long as one person is in it.

If you leave a conversation, you need someone who is still in it to add
you back.

file summary
===================================================================

conversation         
-------------------------------------------------------------------

Provides the model for the conversation state.

conversation_service 
-------------------------------------------------------------------

Service which handles creation and synchronization
of conversations between users.
              
