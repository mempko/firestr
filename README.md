[Fire★](http://www.firestr.com) (Fire Star) 0.6
===================================================================

[![Documentation Status](https://readthedocs.org/projects/fire/badge/?version=latest)](https://readthedocs.org/projects/fire/?badge=latest)

Fire★ is a simple distributed, decentralized communication 
and computation platform.

You don't send a message to someone, you send software, which 
can have rich content. All programs are wired up together automatically
providing distributed communication, either through text, images, videos, 
or games.

The source code to all applications is available immediately to 
instantly clone and modify. 

WARNING
===================================================================

The software is in development. The security of the software has
not been audited so use AT YOUR OWN RISK.

If you are a security expert, please contribute.

Organization
===================================================================

This is the reference implementation of the protocol. It uses QT for
the GUI and a custom message back-end. Currently the applications
are written in Lua, but other bindings will be provided in the future. 

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

