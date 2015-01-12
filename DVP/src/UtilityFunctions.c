/**
 * File Name: UtilityFunctions.c
 * @vvelrath_assignment1
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

#include "../include/UtilityFunctions.h"

//This function creates the sockaddr_in structure using the getaddrinfo() system call
struct addrinfo * getAddrInfoStructure(char* server, char* port){

	//Code adapted from http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#getaddrinfo starts here

	memset(&hints, 0, sizeof(hints)); // make sure the struct is empty
	memset(&servinfo, 0, sizeof(servinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if ((getaddrinfo(server, port, &hints, &servinfo)) != 0) {
	    fprintf(stderr, "Error invoking getaddrinfo\n");
	}

	//Code adapted from http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#getaddrinfo ends here

	return servinfo;
}

struct addrinfo * getAddrInfoStructure_DG(char server[], char port[]){

	//Code adapted from http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#getaddrinfo starts here
	memset(&hints, 0, sizeof(hints)); // make sure the struct is empty
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	char* dest_server = malloc(sizeof(char*));
	char* dest_port = malloc(sizeof(char*));

	strcpy(dest_server, server);
	strcpy(dest_port, port);

	if ((getaddrinfo("127.0.0.1", port, &hints, &servinfo)) != 0) {
	    fprintf(stderr, "Error invoking getaddrinfo\n");
	}
	//Code adapted from http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#getaddrinfo ends here

	return servinfo;
}
