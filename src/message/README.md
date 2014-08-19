message library
===================================================================

Messages need to be received and sent. The general model of the 
message library is inspired by the post office.

There are mailboxes and they have addresses. Mailboxes belong to
post offices and, as mailboxes, post offices have addresses.

The post offices are organized as an n-ary tree. Each has n children
and one parent.

If a post office cannot figure out how to send a message to its child
post offices or mailboxes, it will delegate it to the parent.

If a message reaches the root and cannot be sent, the root will
attempt to send the message over the network.

file summary
===================================================================

message     
-------------------------------------------------------------------

Defines the message class and implements serialization 
using the mencode format. A message has a type, from, to,
extra metadata, and data.

mailbox     
-------------------------------------------------------------------

Implements a mailbox which can receive messages in the
inbox and put messages in the outbox. A post office will
grab messages from the outbox and send them to the address
requested.

postoffice  
-------------------------------------------------------------------

Implements a post office as described above. Each post office
has a thread which polls from all the mailboxes and sends
messages from the mailbox outbox. This way, to send a message
a mailbox must be attached to a post office. The general idea
is that you can add messages to an outbox in a separate thread
and the messages are sent asynchronously. The root post office
can send messages to the outside world using the network library

master_post 
-------------------------------------------------------------------

Special post office which connects firestr to to the outside world.
It uses a connection manager to read from incoming connections 
and make outgoing connections. It is the entry and exit point for 
messages between firestr instances.
              
