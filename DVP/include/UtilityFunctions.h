/*
 * UtilityFunctions.h
 *
 *  Created on: Sep 25, 2014
 *      Author: vivekanandh
 */

#ifndef UTILITYFUNCTIONS_H_
#define UTILITYFUNCTIONS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>

struct addrinfo hints;
struct addrinfo *servinfo, *p;

struct addrinfo * getAddrInfoStructure(char *server, char *port);
struct addrinfo * getAddrInfoStructure_DG(char server[], char port[]);

#endif /* UTILITYFUNCTIONS_H_ */
