#include "ne.h"
#include "router.h"


/* ----- GLOBAL VARIABLES ----- */
struct route_entry routingTable[MAX_ROUTERS];
int NumRoutes;


////////////////////////////////////////////////////////////////
void InitRoutingTbl (struct pkt_INIT_RESPONSE *InitResponse, int myID){
	/* ----- YOUR CODE HERE ----- */
	unsigned int i = 0;
	unsigned int dest_id = 0;
	unsigned int next_hop = 0;
	unsigned int cost = 0;
	NumRoutes = InitResponse->no_nbr + 1;

	for (i = 0; i < (InitResponse->no_nbr); i++) {
		dest_id = InitResponse->nbrcost[i].nbr;
		next_hop = InitResponse->nbrcost[i].nbr;
		cost = InitResponse->nbrcost[i].cost;
		routingTable[i].dest_id = dest_id;
		routingTable[i].next_hop = next_hop;
		routingTable[i].cost = cost;
		routingTable[i].path_len = 2;
		routingTable[i].path[0] = myID;
		routingTable[i].path[1] = dest_id;
	}

	dest_id = myID;
	next_hop = myID;
	cost = 0;
	routingTable[i].dest_id = dest_id;
	routingTable[i].next_hop = next_hop;
	routingTable[i].cost = cost;
	routingTable[i].path_len = 1;
	routingTable[i].path[0] = dest_id;

	return;
}


////////////////////////////////////////////////////////////////
int UpdateRoutes(struct pkt_RT_UPDATE *RecvdUpdatePacket, int costToNbr, int myID){
	/* ----- YOUR CODE HERE ----- */
	unsigned int route_num = RecvdUpdatePacket->no_routes;
	int i = 0;
	int j = 0;
	int k = 0;
	int check_flag = 0;
	int out_flag = 0;
	int loop_flag = 0;
	int split_horizon_path_flag = 0;
	int temp_cost;
	// int temp_next_hop;
	int forced_update_path_flag = 0;
	unsigned int new_cost = 0;
	unsigned int p_len = 0;
	unsigned int path_id = 0;

	// printf("Before: %d\n", NumRoutes);
	// printf("route_num: %d, NumRoutes: %d\n", route_num, NumRoutes);
	// printf("sender_id: %d\n", RecvdUpdatePacket->sender_id);

	for (i = 0; i < route_num; i++) {
		// Detemines if router is new
		if (RecvdUpdatePacket->route[i].path_len == MAX_PATH_LEN) {
     		 RecvdUpdatePacket->route[i].cost = INFINITY;
		}
		loop_flag = 0;
		for (j = 0; j < NumRoutes; j++) {
			// printf("i: %d, j: %d, routingTable[j].dest_id: %d, RecvdUpdatePacket->route[i].dest_id: %d NumRoutes: %d\n", i, j, routingTable[j].dest_id, RecvdUpdatePacket->route[i].dest_id, NumRoutes);
			if (routingTable[j].dest_id == RecvdUpdatePacket->route[i].dest_id) {
				if (RecvdUpdatePacket->route[i].cost >= INFINITY) {
					new_cost = INFINITY;
				}
				else {
					new_cost = RecvdUpdatePacket->route[i].cost + costToNbr;
				}
				loop_flag = 1;
				p_len = RecvdUpdatePacket->route[i].path_len;
				split_horizon_path_flag = 0;
				// printf("BEGIN PATH\n");
				for (k = 0; k < p_len; k++) {
					path_id = RecvdUpdatePacket->route[i].path[k];
					// printf("path_id: %d, my_ID: %d\n", path_id, myID);
					if (path_id == myID) {
						split_horizon_path_flag = 1;
						break;
					}
				}

				// p_len = routingTable[j].path_len;
				// forced_update_path_flag = 0;
				// for (k = 0; k < p_len; k++) {
				// 	if (routingTable[j].path[k] == RecvdUpdatePacket->sender_id) {
				// 		forced_update_path_flag = 1;
				// 	}
				// }
				
				forced_update_path_flag = 0;
				for (k = 0; k < RecvdUpdatePacket->route[i].path_len; k++) {
					if (routingTable[j].path[k] == RecvdUpdatePacket->sender_id && new_cost == INFINITY) {
						forced_update_path_flag = 1;
					}
				}

				if (((routingTable[j].next_hop == RecvdUpdatePacket->sender_id) && (split_horizon_path_flag == 0)) || ((new_cost < routingTable[j].cost) && (split_horizon_path_flag == 0)) || forced_update_path_flag) {
				// if (((forced_update_path_flag == 1) && (split_horizon_path_flag == 0)) || ((new_cost < routingTable[j].cost) && (split_horizon_path_flag == 0))) {
					// printf("Before hop: %d, cost: %d\n", routingTable[j].next_hop, routingTable[j].cost);
					
					// forced_update_path_flag = 0;

					for (k = 0; k < RecvdUpdatePacket->route[i].path_len; k++) {
						if (RecvdUpdatePacket->route[i].path[k] == myID) {
							// forced_update_path_flag = 1;
							new_cost = INFINITY;
						}
					}

					// temp_next_hop = routingTable[j].next_hop;
					temp_cost = routingTable[j].cost;
					routingTable[j].next_hop = RecvdUpdatePacket->sender_id;
					routingTable[j].cost = new_cost;

					
					p_len = RecvdUpdatePacket->route[i].path_len;

					// What if k + 1 exceeds MAX_ROUTERS?
					if (new_cost < INFINITY) {
						for (k = 1; k <= p_len; k++) {
							routingTable[j].path[k] = RecvdUpdatePacket->route[i].path[k - 1];
						}
						routingTable[j].path[0] = myID;
						if (routingTable[j].path_len + 1 < MAX_PATH_LEN) {
							routingTable[j].path_len = RecvdUpdatePacket->route[i].path_len + 1;
						}
					}
					
					
					// printf("Path_len: %d, After hop: %d, cost: %d\n", routingTable[j].path_len, routingTable[j].next_hop, routingTable[j].cost);
					
					if (temp_cost != routingTable[j].cost) {
						out_flag = 1;
						// printf("routingTable[j].dest_id: %d, RecvdUpdatePacket->route[i].dest_id: %d\n", routingTable[j].dest_id, RecvdUpdatePacket->route[i].dest_id);
						printf("For %d: Updated from cost %d to %d, path_len: %d. Info from %d\n", routingTable[j].dest_id, temp_cost, routingTable[j].cost, routingTable[j].path_len, RecvdUpdatePacket->sender_id);
						for (k = 0; k < routingTable[j].path_len; k++) {
							printf("%d->", routingTable[j].path[k]);
						}
						printf("\n");
					
					}
					// out_flag = 1;
				}
				break;
			}
		}
		// check if router already exists
		check_flag = 0;
		if (loop_flag == 0) {
			// printf("NumRoutes: %d\n", NumRoutes);
			for (k = 0; k < NumRoutes; k++) {
				// printf("routingTable[k].dest_id: %d, RecvdUpdatePacket->route[i].dest_id: %d\n", routingTable[k].dest_id, RecvdUpdatePacket->route[i].dest_id);
				if (routingTable[k].dest_id == RecvdUpdatePacket->route[i].dest_id) {
					// printf("CHECK\n");
					check_flag = 1;
					break;
				}
			}
		}
		
		//Router is new
		if (loop_flag == 0 && NumRoutes < MAX_ROUTERS && check_flag == 0 && routingTable[NumRoutes].cost < INFINITY) {
			p_len = RecvdUpdatePacket->route[i].path_len;
			routingTable[NumRoutes].dest_id = RecvdUpdatePacket->route[i].dest_id;
			routingTable[NumRoutes].next_hop = RecvdUpdatePacket->sender_id;
			routingTable[NumRoutes].cost = RecvdUpdatePacket->route[i].cost + costToNbr;
			
			for (k = 1; k <= p_len; k++) {
				routingTable[NumRoutes].path[k] = RecvdUpdatePacket->route[i].path[k - 1];
			}
			routingTable[NumRoutes].path[0] = myID;
			if (routingTable[NumRoutes].path_len + 1 < MAX_PATH_LEN) {
				routingTable[NumRoutes].path_len = RecvdUpdatePacket->route[i].path_len + 1;
			}
			NumRoutes++;
			// printf("ADDED: NumRoutes: %d, dest_id: %d, RecvdUpdatePacket->route[i].dest_id: %d,  i: %d\n", NumRoutes, routingTable[NumRoutes - 1].dest_id, RecvdUpdatePacket->route[i].dest_id, i);
			out_flag = 1;
		}
	}

	return out_flag;
}


////////////////////////////////////////////////////////////////
void ConvertTabletoPkt(struct pkt_RT_UPDATE *UpdatePacketToSend, int myID){
	/* ----- YOUR CODE HERE ----- */

	int i = 0;
	for (i = 0; i < NumRoutes; i++) {
		UpdatePacketToSend->route[i] = routingTable[i];
	}
	UpdatePacketToSend->sender_id = myID;
	UpdatePacketToSend->no_routes = NumRoutes;

	return;
}


////////////////////////////////////////////////////////////////
//It is highly recommended that you do not change this function!
void PrintRoutes (FILE* Logfile, int myID){
	/* ----- PRINT ALL ROUTES TO LOG FILE ----- */
	int i;
	int j;
	for(i = 0; i < NumRoutes; i++){
		fprintf(Logfile, "<R%d -> R%d> Path: R%d", myID, routingTable[i].dest_id, myID);

		/* ----- PRINT PATH VECTOR ----- */
		for(j = 1; j < routingTable[i].path_len; j++){
			fprintf(Logfile, " -> R%d", routingTable[i].path[j]);	
		}
		fprintf(Logfile, ", Cost: %d\n", routingTable[i].cost);
	}
	fprintf(Logfile, "\n");
	fflush(Logfile);
}


////////////////////////////////////////////////////////////////
void UninstallRoutesOnNbrDeath(int DeadNbr){
	/* ----- YOUR CODE HERE ----- */
	int i = 0;
	for (i = 0; i < NumRoutes; i++) {
		if(routingTable[i].next_hop == DeadNbr) {
			routingTable[i].cost = INFINITY;
			// printf("DEAD\n");
		}
	}
	return;
}