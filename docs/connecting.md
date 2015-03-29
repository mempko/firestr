Connecting to People
================

There are two steps to connect to others. 

  1. Add a locator to help you connect with others.
  2. Share your identity with people you want to connect with.

1. Adding a Locator
============

To connect to people across the internet you need to first add a **locator**. 
Click on **Manage Contacts** in the **Contacts** menu. This will open up the **Contacts** dialog.
Then go to the **Locator** tab and click on the **green** add button. 

You can use the free public locator **mempko.com:8080** which is provided for your convenience.

The locator is a signaling service that helps Fire★ find your peers across the internet and helps 
initiate the peer-to-peer connections. None of your communication with someone is routed through 
a locator.

If you are using static IPs, you do not need to use a locator.

2. Adding Contacts
============

There are two ways you can connect to others.

  1. Using identities.
  2. Someone sent you an introduction.

Using Identities
-----------

Everyone has an identity and you can connect with others by giving them your identity and
getting theirs.

You can email your identity to someone using the **Contacts** menu. Click on **Email Identity** 
which will open your mail client with a ready to send email.

To add someone as your contact, click on the **Contacts** menu and select **Add Contact**. 
Paste their identity and click on the **green** add button.

Once both of you have added each other, Fire★ should automatically connect you within a
couple seconds. It will use the Locator to find the person you just added.

Introducing People
-----------

Fire★ has a mechanism where you can introduce a person to another in your contact list. 
To introduce someone, click on **Manage Contacts** in the **Contacts** menu. This will bring up 
a dialog. Click on **Introductions** tab and click the **green** introduction button at the bottom.

If you receive an introduction you can add that person by clicking on the **green** add button
to the right of the introduction.

Both people have to be online to be able to introduce them to each other.

More About Locators
============

Locators do not route any of your communications. They simply are a signaling service that
helps Fire★ locate your contacts across the internet when they change location and network address.

A Public Locator
------------

A public locator is a locator that is run by someone or an organization for free. 
Mempko.com provides a public locator for your convenience. 

  * mempko.com:8080

If you are using [ZeroTier One](https://www.zerotier.com/index.html), a public locator
is provided on the following network.

  * Network: **8056c2e21cae4b10**
  * Locator: **10.181.3.204:7070**

Using Your Own
------------

You can also run your own locator by downloading the [code](http://www.github.com/mempko/firestr) 
and compiling it. Once compiled, running your own locator is as simple as running the **firegreet** executable.


