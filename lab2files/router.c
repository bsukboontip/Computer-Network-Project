#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include "ne.h"
#include "router.h"

//open_listenfd from lab 1
int udp_open_listenfd(int port) 
{
  int listenfd, optval = 1;
  struct sockaddr_in serveraddr;

  /*Create a socket descriptor*/
  if((listenfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    return -1;

  /*Eliminates "Address already in use" error from bind */
  if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int)) < 0)
    return -1;

  /*Listenfd will be an endpoint for all requests to port on any IP address for this host*/
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)port);
  if(bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
    return -1;

  //udp doesnt need to listen and accept

  return listenfd;
}

void logRoutes(int r_ID) {
	char filename[20];
	sprintf(filename, "router%d.log", r_ID);
	FILE * fp = fopen(filename, "w");
	PrintRoutes(fp, r_ID);
	fflush(fp);
	fclose(fp);
}

int main (int argc, char *argv[]) {
	if (argc != 5) {
		printf("Incorrect number of arguments\n");
		return EXIT_FAILURE;
	}

	//VARIABLE DECLARATIONS
	int ne_port, router_port, ne_listenfd, router_id; 
	char *hostname;
	struct hostent *hp;
	struct sockaddr_in recv_addr;
	struct sockaddr_in server_addr;
	struct pkt_INIT_RESPONSE init_response;
	struct pkt_INIT_REQUEST init_request;
	socklen_t len;
	//pthread_mutex_t lock;
	//pthread_t udp_thread_id;
	//pthread_t timer_thread_id;

	router_id = atoi(argv[1]);
	hostname = argv[2];
	ne_port = atoi(argv[3]);
	router_port = atoi(argv[4]);

	//Create UDP socket
	ne_listenfd = udp_open_listenfd(router_port);
	if(ne_listenfd < 0) {
		printf("Failed to open listenfd\n");
		return EXIT_FAILURE;
	}

	//Get hostname
	hp = gethostbyname(hostname);
	if(hp == NULL) {
		printf("Error getting hostname\n");
		return EXIT_FAILURE;
	}

	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(ne_port);
	memcpy((void *) &server_addr.sin_addr, hp->h_addr_list[0], hp->h_length);

	//Initial request
	bzero(&init_request, sizeof(init_request));
	init_request.router_id = htonl(router_id);

	int send = sendto(ne_listenfd, &init_request, sizeof(init_request), 0, (struct sockaddr *) &server_addr, sizeof(server_addr));
	if (send < 0) {
		printf("Failed to send\n");
	}

	//Receive response
	bzero(&recv_addr, sizeof(recv_addr));
	len = sizeof(recv_addr);
	int recv = recvfrom(ne_listenfd, &init_response, sizeof(init_response), 0, (struct sockaddr *) &recv_addr, &len);
	if (recv < 0) {
		printf("Failed to receive\n");
	}
	printf("begin init\n");
	ntoh_pkt_INIT_RESPONSE(&init_response);
	InitRoutingTbl(&init_response, router_id);
	printf("init successful\n");

	//Print initial neighbor info onto logfiles
	logRoutes(router_id);

	//Threading operations

	return EXIT_SUCCESS;
}