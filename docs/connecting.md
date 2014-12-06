Connecting to People
================

There are two primary ways to connect to others. You can either

  1. Add them as a contact using an invite file.
  2. Another person introduces you to them.

1. Adding a Greeter
============

To connect to people across the internet you need to first add a **greeter**. 
Click on **Contacts** in the **Contacts** menu. This will open up the **Contacts** dialog.
Then go to the **Greeter** tab and click on the **green** add button. 

You can use the public greeter **mempko.com:8080** which is provided for your convenience.

The greeter is a signaling service that helps Fire★ find your peers across the internet and helps 
initiate the peer-to-peer connections.

If you are using static IPs, you do not need to use a greeter.

2. Adding Contacts
============

There are two ways you can connect to others.

  1. Using invite files.
  2. Someone sent you an introduction.

Using Invite Files
-----------

You first have to create an invite file that you give to people. Click on **Save Invite**
in the **Contacts** menu. Then give this file to the other person. You can reuse this invite
file with everyone.

Then get an invite file from the other person and add it by dragging it into Fire★.
Alternatively you can add them by clicking on **Add Contact** in the **Contact** menu.

Introducing People
-----------

Fire★ has a mechanism where you can introduce a person to another in your contact list. 
To introduce someone, click on **Contacts** in the **Contacts** menu. This will bring up 
a dialog. Click on **Introductions** tab and click the **green** introduction button at the bottom.

If you receive an introduction you can add that person by clicking on the **green** add button
to the right of the introduction.

More About Greeters
============

Greeters do not route any of your communications. They simply are a signaling service that
helps Fire★ locate your contacts across the internet when they change location and network address.

A Public Greeter
------------

A public greeter is a greeter that is run by someone or an organization for free. 
Mempko.com provides a public greeter for your convenience. 

  * mempko.com:8080

If you are using [ZeroTier One](https://www.zerotier.com/index.html), a public greeter
is provided on the following network.

  * Network: **8056c2e21cae4b10**
  * Greeter: **10.181.3.204:7070**

Using Your Own
------------

You can also run your own greeter by downloading the [code](http://www.github.com/mempko/firestr) 
and compiling it. Once compiled, running your own greeter is as simple as running the **firegreet** executable.


