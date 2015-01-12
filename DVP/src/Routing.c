/*
 * Routing.c
 *
 *  Created on: Nov 23, 2014
 *      Author: vivekanandh
 */
#include <stdint.h>
#include <netdb.h>
#include <stdio.h>
#include <../include/UtilityFunctions.h>
#include <../include/Structures.h>
#include <netinet/in.h>
#include <arpa/inet.h>

//Global variables
uint16_t latest_neighbour = 0;
int num_packets = 0;

//Importing global structures
extern Node* NodeInNetwork;
extern Routing_entry* routing_table;
extern struct Link* Neighbours;

//Importing global variables
extern routing_update* packet;
extern int num_servers;
extern int num_neighbours;
extern int my_id;
extern int listener;
extern int packet_size;
extern int** distVectorForNetwork;

socklen_t addr_len = sizeof(struct sockaddr_in);
struct sockaddr_in dest_addr;
struct sockaddr_in nebr_addr;

//Function declarations
void printInfoForNeighbour(struct routing_update* nebr_packet);
void displayPacketDetails(struct routing_update* nebr_packet);
void updateNetworkDistanceVector(struct routing_update* nebr_packet);
void getServerIDofReceivedPacket(struct routing_update* nebr_packet);
void runDVAlgorithm();
void reInitializeRoutingTable();

/*Code adapted from http://www.xinotes.net/notes/note/1493/ starts here*/
uint32_t convertIPToUint32(char* IP_STR){
	struct sockaddr_in sa;
	inet_pton(AF_INET, IP_STR, &(sa.sin_addr));
	return (uint32_t)sa.sin_addr.s_addr;
}
/*Code adapted from http://www.xinotes.net/notes/note/1493/ ends here*/

//Populating the update packet
void populateUpdatePacket(){

	int i = 0;

	packet->number_of_updates = htons(num_servers);
	packet->ip_address = convertIPToUint32(NodeInNetwork[my_id % num_servers].IP);
	packet->port_no = htons(atoi(NodeInNetwork[my_id % num_servers].Port));

	for(i = 0; i < num_servers; i++){

		int16_t dest_server_id = routing_table[i].dest_server_id;
		packet->entry[i].server_id = htons(dest_server_id);
		packet->entry[i].ip_address = convertIPToUint32(NodeInNetwork[dest_server_id % num_servers].IP);
		packet->entry[i].zero_padding = 0;
		packet->entry[i].port_no = htons(atoi(NodeInNetwork[dest_server_id % num_servers].Port));
		packet->entry[i].cost = htons(routing_table[i].cost_of_link);
	}
}


//Sending the routing packet to my neighbours
void sendRoutesToNeighbors(){

	int i = 0;
	int sockfd = 0;

	cse4589_dump_packet((void *)packet,packet_size);

	for(i = 0; i < num_neighbours; i++){

		int16_t neighbour_id = Neighbours[i].neigbour_id;

		if(routing_table[neighbour_id % num_servers].neighbour == 0)
			continue;

/*
		//Auto filling the Sockaddr data structure
		struct addrinfo *addr_info = getAddrInfoStructure_DG(NodeInNetwork[neighbour_id % num_servers].IP,
														  NodeInNetwork[neighbour_id % num_servers].Port);

		//Code adapted from http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#datagram starts here
		// loop through all the results and make a socket
		for(p = addr_info; p != NULL; p = p->ai_next) {
			if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
				perror("talker: socket");
				close(sockfd);
				continue;
			}
			break;
		}

		if (p == NULL) {
			fprintf(stderr, "talker: failed to bind socket\n");
			return;
		}
		//Code adapted from http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#datagram ends here
*/

		/*Code adapted from http://stackoverflow.com/questions/23966080/sending-struct-over-udp-c starts here*/

		//Populating the neighbour's address structure
		memset(&dest_addr, 0, sizeof(struct sockaddr_in));
		inet_pton(AF_INET, NodeInNetwork[neighbour_id % num_servers].IP, &(dest_addr.sin_addr));
		dest_addr.sin_port = htons(atoi(NodeInNetwork[neighbour_id % num_servers].Port));
		dest_addr.sin_family = AF_INET;

		//Sending the routing packet to neighbour
		sendto(listener,(void *)packet, packet_size, 0, (struct sockaddr*)&dest_addr, sizeof(struct sockaddr_in));

		/*Code adapted from http://stackoverflow.com/questions/23966080/sending-struct-over-udp-c ends here*/
	}
}


//Receive the packet from a neighbour
void receivePacketFromNeighbour(){

	//Local variables
	int i = 0;

	//Neighbour packet declaration
	struct routing_update* nebr_packet = (struct routing_update*)malloc (packet_size);
	memset(&nebr_addr, 0, sizeof(struct sockaddr_in));

	/*Code adapted from http://stackoverflow.com/questions/6967104/passing-structures-using-udp starts here*/
	//Receiving the packet from neighbour
	if ((recvfrom(listener, (void *)nebr_packet, packet_size, 0,(struct sockaddr *)&nebr_addr, &addr_len)) <= 0) {
		perror("Error in recvfrom");
		return;
	}
	/*Code adapted from http://stackoverflow.com/questions/6967104/passing-structures-using-udp ends here*/

	getServerIDofReceivedPacket(nebr_packet);

	//If this packet is not from neighbour, ignore this packet
	if(routing_table[latest_neighbour % num_servers].neighbour == 0)
		return;

	//Increase the number of packets received
	num_packets++;

	//Displaying the neighbor's server ID
	printInfoForNeighbour(nebr_packet);

	//Displaying the neighbour's packet in order
	displayPacketDetails(nebr_packet);

	//Setting the counter back to 3
	routing_table[latest_neighbour % num_servers].counter = 3;

	//Setting the cost of my link to neighbour to my previous cost, if I receive after three timeouts
	for(i = 0; i < num_neighbours; i++){
		if(Neighbours[i].neigbour_id == latest_neighbour && Neighbours[i].cost == UINT16_MAX){
			Neighbours[i].cost = Neighbours[i].orig_cost;
		}
	}

	//Saving the neighbour's packet details in the Network distance vector
	updateNetworkDistanceVector(nebr_packet);

	//Run the DV algorithm to update my routing table
	runDVAlgorithm();

	free(nebr_packet);
}

//Displaying the neighbor's server ID
void printInfoForNeighbour(struct routing_update* nebr_packet){
	cse4589_print_and_log("RECEIVED A MESSAGE FROM SERVER %d\n",latest_neighbour);
}

//Getting the server ID of the recieved packet
void getServerIDofReceivedPacket(struct routing_update* nebr_packet){
	//Local variables
	int i = 0;

	struct in_addr addr = {nebr_packet->ip_address};
	char nebr_port[15];
	sprintf(nebr_port, "%d", ntohs(nebr_packet->port_no));
	char *nebr_ip = inet_ntoa(addr);

    for(i = 0; i < num_servers; i++){
		if(strcmp(NodeInNetwork[i].IP,nebr_ip) == 0 && strcmp(NodeInNetwork[i].Port,nebr_port) == 0){
			break;
		}
	}

    latest_neighbour = ntohs(nebr_packet->entry[i].server_id);
}


//Displaying the packet details in order -- Bubble sort
void displayPacketDetails(struct routing_update* nebr_packet){
	//Local variables
	int i = 0;
	int swap = 1;

	//Bubble sort the array if there was a swap in the previous iteration
	while(swap == 1){

		//Setting the swap variable back to 0
		swap = 0;

		for(i = 0; i < num_servers - 1; i++){
			if(nebr_packet->entry[i].server_id > nebr_packet->entry[i + 1].server_id){
				swap = 1;
				Update_entry temp = nebr_packet->entry[i];
				nebr_packet->entry[i] = nebr_packet->entry[i + 1];
				nebr_packet->entry[i + 1] = temp;
			}
		}
	}//end while

	//Now display the packet details
	for(i = 0; i < num_servers; i++){
		cse4589_print_and_log("%-15d%-15d\n",ntohs(nebr_packet->entry[i].server_id),ntohs(nebr_packet->entry[i].cost));
	}
}

//Updating the network DV with the new packet received from neighbour
void updateNetworkDistanceVector(struct routing_update* nebr_packet){

	//Local variables
	int i = 0;
	int j = 0;

	//Find the neighbour's id
	int neighbour_id = latest_neighbour;

	for(i = 0; i < num_servers; i++){
		int16_t server_id = ntohs(nebr_packet->entry[i].server_id);
		uint16_t cost = ntohs(nebr_packet->entry[i].cost);
		distVectorForNetwork[neighbour_id % num_servers][server_id % num_servers] = cost;
	}
}

//Distance vector algorithm
void runDVAlgorithm(){
	//Local variables
	int i = 0;
	int j = 0;

	//Reinitialize routing table
	reInitializeRoutingTable();

	//Updating my distance vector with link cost to neighbours
	for(i = 0; i < num_servers; i++){
		distVectorForNetwork[my_id % num_servers][i] = UINT16_MAX;
	}

	for(i = 0; i < num_neighbours; i++){
		distVectorForNetwork[my_id % num_servers][Neighbours[i].neigbour_id % num_servers] = Neighbours[i].cost;
	}

	distVectorForNetwork[my_id % num_servers][my_id % num_servers] = 0;

	//Iterating through all the neighbors DV
	for(i = 0; i < num_servers; i++){
		for(j = 0; j < num_servers; j++){
			if(my_id % num_servers != j)
				if(	distVectorForNetwork[my_id % num_servers][i] >
					(distVectorForNetwork[my_id % num_servers][j] + distVectorForNetwork[j][i])){
					distVectorForNetwork[my_id % num_servers][i] = distVectorForNetwork[my_id % num_servers][j] +
																   distVectorForNetwork[j][i];
					routing_table[i].next_hop_server_id = NodeInNetwork[j].server_id;
				}
		}//end for
	}//end for

	//Updating the routing table with computed values after DV algorithm
	for(i = 0; i < num_servers; i++){
		routing_table[i].cost_of_link = distVectorForNetwork[my_id % num_servers][i];
	}

/*
	printf("After DV Algorithm\n");
	for(i = 0; i < num_servers; i++){
		for(j = 0; j < num_servers; j++){
			printf("%d\t",distVectorForNetwork[i][j]);
		}
		printf("\n");
	}
*/

}


//Reinitialize routing table
void reInitializeRoutingTable(){
	//Local variables
	int i = 0;

	for(i = 0; i < num_servers; i++){
		int16_t server_id = routing_table[server_id % num_servers].dest_server_id;
		routing_table[server_id % num_servers].next_hop_server_id = -1;
		routing_table[server_id % num_servers].cost_of_link = UINT16_MAX;
	}

	for(i = 0; i < num_neighbours; i++){
		int16_t neighbour_id = Neighbours[i].neigbour_id;

		if(routing_table[neighbour_id % num_servers].neighbour == 0) continue;

		routing_table[neighbour_id % num_servers].next_hop_server_id = neighbour_id;
		routing_table[neighbour_id % num_servers].cost_of_link = Neighbours[i].cost;
	}

	//Adding self loop to my server
	routing_table[my_id % num_servers].cost_of_link = 0;
	routing_table[my_id % num_servers].next_hop_server_id = my_id;
}


//Decrementing the counters for all my neighbors
void decrementCountersForNeighbors(){
	//Local variables
	int i = 0;

	for(i = 0; i < num_servers; i++){

		//If you are not my neighbour, just continue for next iteration
		if(routing_table[i].neighbour == 0)
			continue;

		//You are a neighbour, but havent sent your first packet yet
		if(routing_table[i].counter == INT16_MAX) continue;

		if(routing_table[i].counter == 0){
			//Recalculate routing table with a cost of infinity
			recalculateRoutingTable(routing_table[i].dest_server_id, UINT16_MAX, 0, 0);
			routing_table[i].counter--;
		}

		if(routing_table[i].counter > 0){
			routing_table[i].counter--;
		}//end if

	}//end for
}
