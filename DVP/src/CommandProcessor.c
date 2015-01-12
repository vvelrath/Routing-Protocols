/*
 * CommandProcessor.c
 *
 *  Created on: Nov 26, 2014
 *      Author: vivekanandh
 */
//Header files
#include <../include/Structures.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//Importing global variables
extern int my_id;
extern int num_servers;
extern int num_neighbours;
extern Routing_entry* routing_table;
extern struct Link* Neighbours;
extern int** distVectorForNetwork;
extern int num_packets;
extern char user_input_command[200];
extern routing_update* packet;
extern int packet_size;

extern fd_set master_sockets;
extern int listener;
extern int crash;

//Function declarations
void recalculateRoutingTable(int neighbour_id, uint16_t cost, int update, int disable);

//Academic Integrity Statement
void academicIntegrity(){
	cse4589_print_and_log("%s:SUCCESS\n",user_input_command);
	cse4589_print_and_log("I have read and understood the course academic integrity policy located at "
			   "http://www.cse.buffalo.edu/faculty/dimitrio/courses/cse4589_f14/index.html#integrity");
}

//Updating the link cost of the neighbour
void updateCostofLink(){
	//Local variables
	char *serverID1_str;
	char *serverID2_str;
	char *cost_str;
	int serverID1;
	int serverID2;
	uint16_t cost;

	//Getting the Server ID's and the new cost of the link
	serverID1_str = strtok(NULL," ");
	serverID2_str = strtok(NULL," ");
	cost_str = strtok(NULL," ");

	//Null validations
	if(serverID1_str == NULL){
		cse4589_print_and_log("%s:%s\n",user_input_command,"Please enter <SERVER ID 1> argument of the UPDATE command");
		printf("Usage: UPDATE <SERVER ID 1> <SERVER ID 2> <COST>\n");
		return;
	}
	if(serverID2_str == NULL){
		cse4589_print_and_log("%s:%s\n",user_input_command,"Please enter <SERVER ID 2> argument of the UPDATE command");
		printf("Usage: UPDATE <SERVER ID 1> <SERVER ID 2> <COST>\n");
		return;
	}
	if(cost_str == NULL){
		cse4589_print_and_log("%s:%s\n",user_input_command,"Please enter <COST> argument of the UPDATE command");
		printf("Usage: UPDATE <SERVER ID 1> <SERVER ID 2> <COST>\n");
		return;
	}

	serverID1 = atoi(serverID1_str);
	serverID2 = atoi(serverID2_str);
	cost = atoi(cost_str);

	//Input Validations
	if(serverID1 != my_id){
		cse4589_print_and_log("%s:%s\n",user_input_command,"The <SERVER ID 1> should be your own ID");
		printf("Usage: UPDATE <SERVER ID 1> <SERVER ID 2> <COST>\n");
		return;
	}

	if(routing_table[serverID2 % num_servers].neighbour != 1){
		cse4589_print_and_log("%s:%s\n",user_input_command,"The <SERVER ID 2> should be one of your neighbour's ID");
		printf("Usage: UPDATE <SERVER ID 1> <SERVER ID 2> <COST>\n");
		return;
	}

	cse4589_print_and_log("%s:SUCCESS\n",user_input_command);

	//Recalculate routing table
	recalculateRoutingTable(serverID2, cost, 1, 0);
}


//Step Command
void step(){
	cse4589_print_and_log("%s:SUCCESS\n",user_input_command);
	populateUpdatePacket();
	sendRoutesToNeighbors();
}


//Display the number of packets since last received
void packets(){
	cse4589_print_and_log("%s:SUCCESS\n",user_input_command);
	cse4589_print_and_log("%d\n",num_packets);
	num_packets = 0;
}


//Disable Link
void disableLink(){
	//Local variables
	char *serverID_str;
	int serverID;

	//Getting the Server ID to disable
	serverID_str = strtok(NULL," ");

	if(serverID_str == NULL){
		cse4589_print_and_log("%s:%s\n",user_input_command,"Please enter the <SERVER ID> argument of the DISABLE command");
		printf("Usage: DISABLE <SERVER ID>\n");
		return;
	}

	serverID = atoi(serverID_str);

	//Validations
	if(serverID == my_id || routing_table[serverID % num_servers].neighbour != 1){
		cse4589_print_and_log("%s:%s\n",user_input_command,"The <SERVER ID> should be one of my neighbour's ID");
		printf("Usage: DISABLE <SERVER ID>\n");
		return;
	}

	cse4589_print_and_log("%s:SUCCESS\n",user_input_command);

	//Recalculate routing table
	recalculateRoutingTable(serverID, UINT16_MAX, 0, 1);
}


//Recalculate my routing table costs as the cost of a link has changed
void recalculateRoutingTable(int neighbour_id, uint16_t cost, int update, int disable){

	//Local variables
	int i = 0;

	//Updating the neighbours cost
	for(i = 0; i < num_neighbours; i++){
		if(Neighbours[i].neigbour_id == neighbour_id){
			Neighbours[i].cost = cost;
			if(update == 1) Neighbours[i].orig_cost = cost;
		}
	}

	routing_table[neighbour_id % num_servers].next_hop_server_id = neighbour_id;
	routing_table[neighbour_id % num_servers].cost_of_link = cost;

	//Updating the next hop and cost in routing table
	if(cost == UINT16_MAX && disable == 1){
		routing_table[neighbour_id % num_servers].next_hop_server_id = -1;
		routing_table[neighbour_id % num_servers].neighbour = 0;

		for(i = 0; i < num_servers; i++){
			distVectorForNetwork[neighbour_id % num_servers][i] = UINT16_MAX;
		}
	}

	//Updating the new cost in the network distance vector
	distVectorForNetwork[my_id % num_servers][neighbour_id % num_servers] = cost;

	//Run the distance vector algorithm
	runDVAlgorithm();
}


//Display my routing table
void displayRoutingTable(){
	//Local variables
	int i = 0;
	int swap = 1;

	Routing_entry* routing_table_temp = (Routing_entry*)malloc(num_servers * sizeof(Routing_entry));;

	for(i = 0; i < num_servers; i++){
		routing_table_temp[i] = routing_table[i];
	}

	//Bubble sort the array if there was a swap in the previous iteration
	while(swap == 1){

		//Setting the swap variable back to 0
		swap = 0;

		for(i = 0; i < num_servers - 1; i++){
			if(routing_table_temp[i].dest_server_id > routing_table_temp[i + 1].dest_server_id){
				swap = 1;
				Routing_entry temp = routing_table_temp[i];
				routing_table_temp[i] = routing_table_temp[i + 1];
				routing_table_temp[i + 1] = temp;
			}
		}
	}//end while

	cse4589_print_and_log("%s:SUCCESS\n",user_input_command);

	for(i = 0; i < num_servers; i++){
		cse4589_print_and_log("%-15d%-15d%-15d\n",routing_table_temp[i].dest_server_id,
												  routing_table_temp[i].next_hop_server_id,
												  routing_table_temp[i].cost_of_link);
	}

	free(routing_table_temp);
}


//Crashing the node
void crashServer(){
	//Local variables
	int i = 0;

	//Setting the crash variable
	crash = 1;

	cse4589_print_and_log("%s:SUCCESS\n",user_input_command);

	for(i = 0; i < num_servers; i++){
		if(routing_table[i].neighbour == 0) continue;
		recalculateRoutingTable(routing_table[i].dest_server_id, UINT16_MAX, 0, 1);
	}

	//Stop listening on listener(to stop listening on other hosts)
	FD_CLR(listener, &master_sockets);
}


//Dump a packet with the detials from routing table
void dumpPacket(){
	cse4589_print_and_log("%s:SUCCESS\n",user_input_command);
	populateUpdatePacket();
	cse4589_dump_packet((void *)packet,packet_size);
}
