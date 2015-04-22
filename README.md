[Fire★](http://www.firestr.com) (Fire Star) 0.8
===================================================================

[![Documentation Status](https://readthedocs.org/projects/fire/badge/?version=latest)](https://readthedocs.org/projects/fire/?badge=latest)

**The Grass Computing Platform**

Fire★ is a platform for creating and sharing P2P software.
This is not cloud software, but grass software.
You can touch it and shape it.

Fire★ provides a built in application editor where you can program in 
real time with others. Apps are written in the Lua programming language
using an API designed for writing P2P applications. 

Sharing an application with another is as simple as using it in a conversation. 
They can install it on their system with one click and use it in their own conversations.
Any program you get you can open in the application editor to modify and share.

The hard part of setting up a P2P connection (NAT transversal, UDP hole punching, etc)
is done for you so you can concentrate on building applications that work together
without a central server. 

All communication is encrypted between peers to provide a safe environment and 
no communication is routed through a server.


WARNING
===================================================================

The software is in development. The security of the software has
not been audited so use AT YOUR OWN RISK.

If you are a security expert, please contribute.

Organization
===================================================================

Fire★ is written using C++11 and requires a fairly modern compiler.
It should with the latest gcc, clang, and visual studio. 

The main directory is src. Each subdirectory is a library and
the main application is in src/firestr

The project uses CMake as the meta-build configuration.

Build Dependencies
===================================================================

* gcc 4.8
* CMake
* Qt 5 
* Boost 1.54
* Botan 1.10
* libopus
* libsnappy
* uuid
* libssl
* libgmp

Building Using Vagrant
===================================================================

Install vagrant on your machine. Open a terminal and run:

    $ vagrant up
    $ vagrant ssh
    $ startxfc4&

Open terminal in VM and run:

    $ cd /vagrant
    $ mkdir build
    $ cd build
    $ cmake ..
    $ make -j2

You can then run two test instances

    $ cd src/firestr/
    $ ./firestr --home test1 --port 6060&
    $ ./firestr --home test2 --port 7070&

License GPLv3
===================================================================

Copyright (C) 2013  Maxim Noah Khailo
 
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

