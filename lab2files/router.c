#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include "ne.h"
#include "router.h"

struct update_tracker {
	unsigned int sender_id;
	unsigned int cost;
	unsigned int last_update;
};

//Global Variables
struct update_tracker update_list[MAX_ROUTERS];
pthread_mutex_t lock;
int ne_listenfd, last_update_time, last_converge, current_time, last_changed, start_time, converge_flag;
unsigned int num_neighbors = 0;
unsigned int router_id;

struct sockaddr_in server_addr;

//Function Declarations
int udp_open_listenfd(int port);
void logRoutes(int r_ID);
void *udp_thread(void * arg);
void *timer_thread(void * arg);


//open_listenfd from lab 1
int udp_open_listenfd(int port) 
{
  int listenfd, optval = 1;
  struct sockaddr_in serveraddr;

  //Create a socket descriptor
  if((listenfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    return -1;

  //Eliminates "Address already in use" error from bind 
  if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int)) < 0)
    return -1;

  //Listenfd will be an endpoint for all requests to port on any IP address for this host
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
	FILE * fp = NULL;
	sprintf(filename, "router%d.log", r_ID);
	fp = fopen(filename, "w");
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
	int ne_port, router_port, i; 
	char *hostname;
	struct hostent *hp;
	struct sockaddr_in recv_addr;
	struct pkt_INIT_RESPONSE init_response;
	struct pkt_INIT_REQUEST init_request;
	socklen_t len;

	pthread_t udp_thread_id;
	pthread_t timer_thread_id;

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
	int send_size = sizeof(server_addr);
	int pkt_size = sizeof(init_request);
	int send = sendto(ne_listenfd, &init_request, pkt_size, 0, (struct sockaddr *) &server_addr, send_size);
	if (send < 0) {
		printf("Failed to send\n");
	}

	//Receive response
	bzero(&recv_addr, sizeof(recv_addr));
	len = sizeof(recv_addr);
	pkt_size = sizeof(init_response);
	int recv = recvfrom(ne_listenfd, &init_response, pkt_size, 0, (struct sockaddr *) &recv_addr, &len);
	if (recv < 0) {
		printf("Failed to receive\n");
	}
	// printf("begin init\n");
	ntoh_pkt_INIT_RESPONSE(&init_response);
	
	InitRoutingTbl(&init_response, router_id);
	logRoutes(router_id);
	num_neighbors = init_response.no_nbr;

	last_update_time = time(NULL);
	last_converge = time(NULL);
	start_time = time(NULL);
	converge_flag = 0;

	for (i = 0; i < num_neighbors; i++) {
		update_list[i].cost = init_response.nbrcost[i].cost;
		update_list[i].sender_id = init_response.nbrcost[i].nbr;
		update_list[i].last_update = time(NULL);
	}

	pthread_mutex_init(&lock, NULL);
	pthread_create(&udp_thread_id, NULL, udp_thread, NULL);
	pthread_create(&timer_thread_id, NULL, timer_thread, NULL);

	pthread_join(udp_thread_id, NULL);
	pthread_join(timer_thread_id, NULL);

	//Print initial neighbor info onto logfiles

	//Threading operations
	// fclose(fp);
	return EXIT_SUCCESS;
}

void *udp_thread(void * arg) {
	struct pkt_RT_UPDATE update_response;
	struct sockaddr_in recv_addr;
	socklen_t len;
	int i = 0;
	int flag = 0;
	unsigned int cost = 0;
	bzero(&recv_addr, sizeof(recv_addr));
	len = sizeof(recv_addr);
	// printf("IN UDP_THREAD\n");

	char filename[20];
	FILE * fp = NULL;
	sprintf(filename, "router%d.log", router_id);

	while(1) {
		
		int pkt_size = sizeof(update_response);
		int recv = recvfrom(ne_listenfd, &update_response, pkt_size, 0, (struct sockaddr *) &recv_addr, &len);
		if (recv < 0) {
			printf("Failed to receive\n");
			return NULL;
		}
		ntoh_pkt_RT_UPDATE(&update_response);

		pthread_mutex_lock(&lock);

		// Get cost
		for (i = 0; i < num_neighbors; i++) {
			if (update_response.sender_id == update_list[i].sender_id) {
				update_list[i].last_update = time(NULL);
				cost = update_list[i].cost;
				break;
			}
		}

		flag = UpdateRoutes(&update_response, cost, router_id);
		// printf("flag: %d\n", flag);
		if (flag) {
			// printf("Routes Updated\n");
			fp = fopen(filename, "w");
			PrintRoutes(fp, update_response.dest_id);
			fflush(fp);
			fclose(fp);
			last_converge = time(NULL);
			converge_flag = 1;
		}
		pthread_mutex_unlock(&lock);
	}
	
	return NULL;
}

void *timer_thread(void * args) {
	struct pkt_RT_UPDATE rt_update;
	int i, send;
	char filename[20];
	FILE * fp = NULL;
	sprintf(filename, "router%d.log", router_id);

	int DeadNbr[MAX_ROUTERS];
	i = 0;
	while (i < MAX_ROUTERS) {
		DeadNbr[i] = 0;
		i++;
	}

	while (1) {
		//Check if last update expired. If so, send update packet to all neighbors
		pthread_mutex_lock(&lock);
		// printf("IN TIMER_THREAD\n");
		i = 0;
		current_time = time(NULL);
		// printf("Curr_time: %d, last_update_time %d\n", current_time, last_update_time);
		if ((current_time - last_update_time) >= UPDATE_INTERVAL) {
			while (i < num_neighbors) {
				bzero(&rt_update, sizeof(rt_update));
				ConvertTabletoPkt(&rt_update, router_id);
				rt_update.dest_id = update_list[i].sender_id;
				hton_pkt_RT_UPDATE(&rt_update);
				int pkt_size = sizeof(rt_update);
				int send_size = sizeof(server_addr);
				send = sendto(ne_listenfd, &rt_update, pkt_size, 0, (struct sockaddr *) &server_addr, send_size);
				if (send < 0) {
					printf("Failed to send\n");
				}
				// printf("pkt sent\n");
				last_update_time = time(NULL);
				i++;
			}
		}
		pthread_mutex_unlock(&lock);
		//Check for dead neighbors
		pthread_mutex_lock(&lock);
		i = 0;
		while(i < num_neighbors) {
			// printf("IN TIMER_THREAD\n");
			current_time = time(NULL);
			if((current_time - update_list[i].last_update) > FAILURE_DETECTION) {
				UninstallRoutesOnNbrDeath(update_list[i].sender_id);
				if(DeadNbr[i] == 0) {
					fp = fopen(filename, "w");
					PrintRoutes(fp, router_id);
					fflush(fp);
					fclose(fp);
					DeadNbr[i] = 1;
					// printf("%d, last_converge change in timer\n", last_converge);
					last_converge = time(NULL);
					converge_flag = 1;
				}
			}
			else {
				DeadNbr[i] = 0;
			}
			i++;
		}
		pthread_mutex_unlock(&lock);
		//Check for convergence
		pthread_mutex_lock(&lock);
		current_time = time(NULL);
		// printf("%d:Check Converged, current_time: %d, last_converge: %d\n", current_time - last_converge, current_time, last_converge);
		if(((current_time - last_converge) > CONVERGE_TIMEOUT) && (converge_flag)) {
			// PrintRoutes(fp, router_id);
			fp = fopen(filename, "w");
			fprintf(fp, "%d:Converged\n", (int) current_time - start_time);
			fflush(fp);
			fclose(fp);
			printf("%d:Converged\n", (int) current_time - start_time);
			converge_flag = 0;
		}
		pthread_mutex_unlock(&lock);
	}
	return NULL;
}