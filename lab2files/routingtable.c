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
		routingTable[i].path_len = 1;
		routingTable[i].path[0] = dest_id;
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
	// ntoh_pkt_RT_UPDATE(RecvdUpdatePacket);
	

	return 0;
}


////////////////////////////////////////////////////////////////
void ConvertTabletoPkt(struct pkt_RT_UPDATE *UpdatePacketToSend, int myID){
	/* ----- YOUR CODE HERE ----- */

	// ntoh_pkt_RT_UPDATE(UpdatePacketToSend);
	int i = 0;
	for (i = 0; i <= NumRoutes; i++) {
		UpdatePacketToSend->route[i] = routingTable[i];
	}
	UpdatePacketToSend->sender_id = myID;
	UpdatePacketToSend->no_routes = NumRoutes;

	// hton_pkt_RT_UPDATE(UpdatePacketToSend);

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
	return;
}