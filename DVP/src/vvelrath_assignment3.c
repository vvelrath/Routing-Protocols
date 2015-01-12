/**
 * @vvelrath_assignment3
 * @author  Vivekanandh Vel Rathinam <vvelrath@buffalo.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * This contains the main function. Add further description here....
 */
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "../include/Structures.h"
#include "../include/global.h"
#include "../include/logger.h"
#include "../include/UtilityFunctions.h"


//Declaring variables
int num_servers;
int num_neighbours;
int i = 0;

int server_id;
char server_ip[INET_ADDRSTRLEN];
char server_port[10];

int my_id;
int neighbour_id;
int cost;

int listener;
int a = 1;
const void *optval = &a;

fd_set master_sockets, watch_sockets;
int max_socket;
struct timeval timeout;

char *command;
char user_input[200];
char user_input_command[200];

int rv;

int packet_size;

int** distVectorForNetwork;
int crash = 0;
/*-------------------END OF VARIABLE DECLARATIONS------------------------------------*/

Routing_entry* routing_table = NULL;
struct routing_update* packet = NULL;
struct Node* NodeInNetwork = NULL;
struct Link* Neighbours = NULL;

//Function declarations
void initializeStructures();
void readTopologyFile();
void routeInitializer();
void initializeFDS();
void createDistanceVectorForNetwork();
char* toUpperCase(char* string);
/**
 * main function
 *
 * @param  argc Number of arguments
 * @param  argv The argument list
 * @return 0 EXIT_SUCCESS
 */

int main(int argc, char **argv)
{
	/*Init. Logger*/
	cse4589_init_log();

	/*Clear LOGFILE and DUMPFILE*/
	fclose(fopen(LOGFILE, "w"));
	fclose(fopen(DUMPFILE, "wb"));

	/*Start Here*/
	//Reads from topology file
	readTopologyFile(argv);

	//Initializes the routing table
	routeInitializer();

	//Creating a listening socket
	listener = createListener();

	//Initializing the Master FDS with the listening and the standard input socket numbers
	initializeFDS();

	//Setting the timer each time in the for loop
	timeout.tv_sec = atoi(argv[4]);
	timeout.tv_usec = 0;

	//Infinite FOR loop to continuously satisfy the network
	for(;;){

		//Copying the master list to the watch list
		memcpy(&watch_sockets,&master_sockets,sizeof(&master_sockets));

		if((rv = select(max_socket+1,&watch_sockets,NULL,NULL,&timeout)) == -1){
			perror("Error in Select");
			exit(1);
		}

		//Looking for user commands from the input prompt
		if(FD_ISSET(0, &watch_sockets)){

				//Getting the user input
				fgets(user_input, sizeof(user_input), stdin);
				user_input[strlen(user_input) - 1] = '\0';

				//Getting the primary command from user input
				if(strlen(user_input) == 0)
					command = "";
				else
					command = strtok(user_input," ");

				if(crash == 1){
					//Input Prompt for getting next input
					printf("\n>> ");
					fflush(stdout);
					continue;
				}

				//Changing to upper case
				strcpy(user_input_command, command);
				command = toUpperCase(command);


				if(strcmp(command,"ACADEMIC_INTEGRITY") == 0){
			 		academicIntegrity();
			 	}else if(strcmp(command,"UPDATE") == 0){
					updateCostofLink();
				}else if(strcmp(command,"STEP") == 0){
					step();
				}else if(strcmp(command,"PACKETS") == 0){
					packets();
				}else if(strcmp(command,"DISPLAY") == 0){
					displayRoutingTable();
				}else if(strcmp(command,"DISABLE") == 0){
					disableLink();
				}else if(strcmp(command,"CRASH") == 0){
					crashServer();
				}else if(strcmp(command,"DUMP") == 0){
					dumpPacket();
				}else{
					cse4589_print_and_log("%s:%s\n",user_input_command,"The input command is not valid");
				}

				//Input Prompt for getting next input
				printf("\n>> ");
				fflush(stdout);
		}
		//Timeout in select
		else if(rv == 0){

			//Setting the timer each time after a timeout
			timeout.tv_sec = atoi(argv[4]);
			timeout.tv_usec = 0;

			//Decrementing the counters for all my neighbours
			decrementCountersForNeighbors();

			//Populating the update packet from my routing table
			populateUpdatePacket();

			//Sending the update packet to all my neighbours
			sendRoutesToNeighbors();

		}
		//We have some packet from a neighbour
		else{
			receivePacketFromNeighbour();

			//Input Prompt after satisfying all the requests in the watch list
			printf("\n>> ");
			fflush(stdout);
		}
	}

	return 0;
}


//Initializing the buffer to the size of the window
void initializeStructures(){
	routing_table = (Routing_entry*)realloc (routing_table, num_servers * sizeof(Routing_entry));
	NodeInNetwork = (struct Node*)realloc (NodeInNetwork, num_servers * sizeof(struct Node));
	Neighbours = (struct Link*)realloc (Neighbours, num_neighbours * sizeof(struct Link));
	packet = (struct routing_update*)realloc (packet, sizeof(struct routing_update) + num_servers*sizeof(Update_entry));
	packet_size = sizeof(struct routing_update) + num_servers*sizeof(Update_entry);
}


//Creating and initializing the distance vector for network
void createDistanceVectorForNetwork(){
	//Local variables
	int i = 0;
	int j = 0;

	/*Code adapted from http://www.geeksforgeeks.org/dynamically-allocate-2d-array-c/ starts here*/
	distVectorForNetwork = (int **)malloc(num_servers * sizeof(int *));

	for (i = 0; i < num_servers; i++){
		distVectorForNetwork[i] = (int *)malloc(num_servers * sizeof(int));
	}
	/*Code adapted from http://www.geeksforgeeks.org/dynamically-allocate-2d-array-c/ ends here*/

	//Initialzing the distance vector with infinity as the cost
	for(i = 0; i < num_servers; i++){
		for(j = 0; j < num_servers; j++){
			distVectorForNetwork[i][j] = UINT16_MAX;
		}
	}

	//Adding self loop values to me and my neighbours
	distVectorForNetwork[my_id % num_servers][my_id % num_servers] = 0;

	for(i = 0; i < num_neighbours; i++){
		distVectorForNetwork[my_id % num_servers][Neighbours[i].neigbour_id % num_servers] = Neighbours[i].cost;
		distVectorForNetwork[Neighbours[i].neigbour_id % num_servers][Neighbours[i].neigbour_id % num_servers] = 0;
	}
}


//Parses the topology file for network information
void readTopologyFile(char **argv){
	//Reading from topology file
	FILE *tfp = fopen(argv[2],"r");

	fscanf(tfp, "%d", &num_servers);
	fscanf(tfp, "%d", &num_neighbours);

	//Initialize the routing structures
	initializeStructures();

	//Reading the topology files
	for(i = 0; i < num_servers; i++){
		fscanf(tfp, "%d %s %s", &server_id, server_ip, server_port);
		NodeInNetwork[server_id % num_servers].server_id = server_id;
		strcpy(NodeInNetwork[server_id % num_servers].IP, server_ip);
		strcpy(NodeInNetwork[server_id % num_servers].Port, server_port);
	}

	for(i = 0; i < num_neighbours; i++){
		fscanf(tfp, "%d %d %d", &my_id, &neighbour_id, &cost);
		Neighbours[i].neigbour_id = neighbour_id;
		Neighbours[i].cost = cost;
		Neighbours[i].orig_cost = cost;
	}

	fclose(tfp);
}


//Initializes the routing table from the topology info
void routeInitializer(){
	for(i = 0; i < num_servers; i++){
		int16_t server_id = NodeInNetwork[i].server_id;
		routing_table[server_id % num_servers].dest_server_id = NodeInNetwork[i].server_id;
		routing_table[server_id % num_servers].next_hop_server_id = -1;
		routing_table[server_id % num_servers].cost_of_link = UINT16_MAX;
		routing_table[server_id % num_servers].counter = INT16_MAX;
		routing_table[server_id % num_servers].neighbour = 0;
	}

	for(i = 0; i < num_neighbours; i++){
		int16_t neighbour_id = Neighbours[i].neigbour_id;
		routing_table[neighbour_id % num_servers].cost_of_link = Neighbours[i].cost;
		routing_table[neighbour_id % num_servers].neighbour = 1;
		routing_table[neighbour_id % num_servers].next_hop_server_id = neighbour_id;
	}

	//Adding self loop to my server
	routing_table[my_id % num_servers].cost_of_link = 0;
	routing_table[my_id % num_servers].next_hop_server_id = my_id;

	createDistanceVectorForNetwork();
}


//This function helps us to create a socket and listen on a specific port
int createListener(){
	//Local listening socket number
	int listen_socket = 0;
	struct addrinfo *addr_info, *p;

	char* port_str = NodeInNetwork[my_id % num_servers].Port;

	//Creating a listener socket
	if ((listen_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		perror("Error creating a Socket");
		exit(1);
	}

	//Gets rid of the Address Already in USE message
	if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1){
		perror("Error setting options on the socket");
		exit(1);
	}

	//Auto filling the Sockaddr data structure
	addr_info = getAddrInfoStructure(NULL,port_str);

	for(p = addr_info; p != NULL; p = p->ai_next) {
		if(bind(listen_socket,p->ai_addr,p->ai_addrlen) < 0)
			continue;
		else
			break;
	}

	return listen_socket;
}


//This function initializes the master and file descriptor sets
void initializeFDS(){
	//Initializing the master and the watch lists
	FD_ZERO(&master_sockets);
	FD_ZERO(&watch_sockets);

	//Adding the Standard Input File descriptor to the master connectors
	FD_SET(0, &master_sockets);

	//Adding the listening socket to the master connectors
	FD_SET(listener, &master_sockets);

	//Setting the maximum socket number with the last created listening socket number
	max_socket = listener;

	//To make sure that the data is flushed to standard output even without a new line
	setbuf(stdout, NULL);
	printf(">> ");
}

/*Code adapted from http://www.java2s.com/Tutorial/Cpp/0040__Data-Types/Convertinglowercaseletterstouppercaselettersusingapointer.htm starts here*/
//Function to convert a string to upper case
char* toUpperCase(char* string){
	char *start = string;
	while(*string != '\0'){
		*string = toupper(*string);
		string++;
	}
	return start;
}
/*Code adapted from http://www.java2s.com/Tutorial/Cpp/0040__Data-Types/Convertinglowercaseletterstouppercaselettersusingapointer.htm ends here*/
