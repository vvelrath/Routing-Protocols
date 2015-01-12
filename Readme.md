Implementing Routing Protocols
=================================

### Description

Implemented a simplified version of the Distance Vector Protocol. The protocol will be run on top of servers (behaving as routers) using UDP. Each server runs on a machine at a predefined port number. The servers should be able to output their forwarding tables												
																																						
A server sends out routing packets only in the following two conditions: 
a) periodic update.																																			
b) the user uses a command asking for one.																														

### Topology Establishment

In this programming assignment, five servers were used – {stones, euston, highgate, embankment, underground}.cse.buffalo.edu. Each server will need to be supplied with a topology file at startup that it uses to build its initial routing table. The topology file is local and contains the link cost to the neighbors. For all other servers in the network, the initial cost would be infinity. Each server can only read the topology file for itself. The entries of a topology file are listed below:
● <num­servers>
● <num­neighbors>
● <server­ID> <server­IP> <server­port>
● <server­ID1> <server­ID2> <cost>

num­servers: total number of servers.
server­ID, server­ID1, server­ID2: A unique identifier for a server.
cost: Cost of a given link between a pair of servers. Assume that cost is either a non­zero positive integer value.

For more information on topology files, refer to the project description

### Program execution

Import the DVP folder into eclipse as a makefile project and compile it. Start a server using the following syntax

	Syntax: ./assignment3 ­t <path­to­topology­file> ­i <routing­update­interval>

path­to­topology­file: The topology file contains the initial topology
configuration for the server, e.g., timberlake_init.txt. Please adhere to the
format described in 3.1 for your topology files.

routing­update­interval: It specifies the time interval between routing updates
in seconds.

port and server­id: They are written in the topology file. The server should find
its port and server­id in the topology file without changing the entry format or
adding any new entries.