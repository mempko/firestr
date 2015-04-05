
Project Overview 
===================================================================

The project is organized as libraries and executables. All of the 
executables start with `fire', where `firestr' is the main one.

Each subdirectory has it's own CMakeLists file and README.

Design 
===================================================================

The design of Fire★ is based on a microkernel architecture where
various parts run as services which communicate over message queues.

Dependency Graph 
===================================================================
        
            ----------------------------------
            |  QT Boost Botan Dtl Lua Snappy |
            ----------------------------------    
                                |
                        -----------------
            ------------|     util      |-----
            |           -----------------    |
            |             |        |         |
            |           -----------------    |
            |           |    security   |    |
            |           -----------------    |
            |             |        |         |
    -----------------     |        |         |
    |    network    |     |        |         |
    -----------------     |        |         |
                |         |        |         |
            -----------------      |         |
            |     message   |      |         |
            -----------------      |         |
                |                  |         |
            -----------------      |         |
            |     user      |      |         |
            -----------------      |         |
                 |                 |         |
                 |                 |         |
                 |          ------------------
                 |      /-- |     service    |
                 |      |   ------------------
                 |      |          |         |
            ------------------     |         |
            |    messages    |     |         |
            ------------------     |         |
                 |         |       |         |
                 |  ------------------       |  
                 |  |  conversation  |       | 
                 |  ------------------       |
                 |         |       |         |
                 |         |       |         |
                 |      -----------------    |
                 |      |     gui       |    |
                 |      -----------------    |
                 |              |            |
                 |              |            |
            /---------------\   |            |
            |     firestr   |----------------/ 
            \---------------/   


Component Summary 
===================================================================

util          
-------------------------------------------------------------------

Place to put things that everything else depends on

security      
-------------------------------------------------------------------

Library dealing with keeping the communication secure

network       
-------------------------------------------------------------------

Library dealing with sending bits and blips over the network

message       
-------------------------------------------------------------------

Library for sending messages between components and over network

messages      
-------------------------------------------------------------------

Code for various message types that can go over the wire

service       
-------------------------------------------------------------------

Library that simplifies the creation of threaded services.

user          
-------------------------------------------------------------------

Library that manages user and contact state and connectivity

conversation  
-------------------------------------------------------------------

Library that manages conversation state and connectivity between users

gui           
-------------------------------------------------------------------

Various widgets which can be combined into an application

firestr       
-------------------------------------------------------------------

Main application

firelocator     
-------------------------------------------------------------------

Greeter application which helps firestr communicate over NAT

packaged_apps 
-------------------------------------------------------------------

Example/useful apps

slb           
-------------------------------------------------------------------

The SLB Lua binding library written by Jose L. Hidalgo Valiño. 
Also contains Lua 5.2 which is available at [lua.org](http://www.lua.org).


