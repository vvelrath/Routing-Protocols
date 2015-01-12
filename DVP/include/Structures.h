/*
 * Structures.h
 *
 *  Created on: Nov 24, 2014
 *      Author: vivekanandh
 */

#ifndef STRUCTURES_H_
#define STRUCTURES_H_
#include <netdb.h>

//[PA3] Routing Table Start
//Declaration of routing table
typedef struct{
	int16_t dest_server_id;
	int16_t next_hop_server_id;
	uint16_t cost_of_link;
	int16_t counter;
	int16_t neighbour;
} Routing_entry;
//[PA3] Routing Table End

//[PA3] Update Packet Start
//Routing update structure
typedef struct{
	uint32_t ip_address;
	uint16_t port_no;
	int16_t zero_padding;
	int16_t server_id;
	uint16_t cost;
} Update_entry;


typedef struct routing_update{
	int16_t number_of_updates;
	uint16_t port_no;
	uint32_t ip_address;
	Update_entry entry[];
} routing_update;
//[PA3] Update Packet End

//Nodes information structure
typedef struct Node{
	int16_t server_id;
	char IP[INET_ADDRSTRLEN];
	char Port[10];
}Node;


//Neighbour information
typedef struct Link{
	int16_t neigbour_id;
	uint16_t cost;
	uint16_t orig_cost;
} Link;


#endif /* STRUCTURES_H_ */
