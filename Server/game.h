
#ifndef GAME_H
#define GAME_H

// the maximum number of connected clients
#define CLIENTS_MAX 4

// game modes
#define GAME_MODE_TAKING_CLIENTS 0
#define GAME_MODE_GAME           1

struct client {
	long long lastPingMS;
	unsigned long long iP;
	unsigned int packetID;
	int pingsSent;
	int port;
	int alive;
	char name[16];
	struct stats *stats;
};

struct game {
	int mode;
	int clientsN;
	struct client clients[CLIENTS_MAX];
};

#endif
