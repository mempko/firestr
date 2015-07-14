Library Dependencies
===================================================================
* gcc 4.7
* Qt 5
* Boost
* Botan 1.10
* libsnappy
* libopus
* libxss
* uuid
* libgmp

Building Firestr
===================================================================

Firestr uses CMake for it's meta-build configuration.

    $ mkdir build
    $ cd build
    $ cmake ..
    $ make -j

Running Firestr
===================================================================

There are two ways to have two or more instances of Firestr connect
to each other. 

If you are on a local network and only want to communicate within 
the local network, you can simply run firestr.

If you want to communicate over the internet, you have to 
first add a public locator server to your locator list
under the contact menu. 

mempko.com provides a free public server at http://mempko.com:8080

To connect with someone, give them your identity and get theirs.

After you both add each other as contacts, firestr should automatically 
connect you with the other person.  If you have added a locator, it will 
talk to the locator and use it as part of the connection process.

The locator service is simply responsible for providing information
to the parties on their public ip/port after which firestr will 
initiate a p2p connection. 

Have fun.

License GPLv3
===================================================================

Copyright (C) 2012  Maxim Noah Khailo
 
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
 
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
 
You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.


