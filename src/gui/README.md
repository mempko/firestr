gui library
===================================================================

Implements the GUI and main functions of firestr. It connects most
of the pieces together and is probably the most HACKish part of the
project. 

file summary
===================================================================

main_win       
-------------------------------------------------------------------

Creates the main window and starts everything up.

api       
-------------------------------------------------------------------
Contains the interface to both the gui and backend api for apps.

app       
-------------------------------------------------------------------
Containst the various built in Apps and app related code.

data       
-------------------------------------------------------------------
Static data used by the gui.

lua       
-------------------------------------------------------------------
Lua app backend implementation.

qtw       
-------------------------------------------------------------------
QWidget based app frontend implementation.

list          
-------------------------------------------------------------------
implements a simple list gui object.
             
message       
-------------------------------------------------------------------
implements the container for 'messages' to live in, 
which are added to list. 

conversation  
-------------------------------------------------------------------
implements a conversation gui, including code to add contacts
to a conversation and stores messages for that conversation.

message_list   
-------------------------------------------------------------------
a factory class which creates a message based on type.
for example, if a app message comes it, it will initiate.
the application.

contact_list   
-------------------------------------------------------------------
UI for managing contacts and greeters.

contact_select 
-------------------------------------------------------------------
UI for select available contacts in a dropdown

mail_service  
-------------------------------------------------------------------

A thread which monitors a mailbox and emits a signal when a 
message arrives.

debugwin  
-------------------------------------------------------------------
Debug window which shows logs and connection graphs

icon
-------------------------------------------------------------------
The logo

unknown_message
-------------------------------------------------------------------
Displays an unknown message widget.

util
-------------------------------------------------------------------
Various utilities shared by various gui components.
