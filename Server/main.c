
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <signal.h>

#ifdef LINUX
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#endif

#ifdef WIN32
#include <winsock2.h>
#endif

#include "defines.h"
#include "main.h"
#include "game.h"


// the server port
#define SERVER_PORT 9999

// the size of the input buffer (64KBs should be enough for UDP packets)
#define INPUT_BUFFER_SIZE (64*1024)

// debug to stderr?
#define DEBUG_MESSAGES

// the one and only game structure
struct game game;

// quit?
int quit = NO;

// the client timeout in seconds
int clientTimeoutSeconds;


void clientReset(struct game *game, int clientID) {

	struct client *client = &game->clients[clientID];

	client->iP = 0;
	client->packetID = 0;
	client->lastPingMS = 0;
	client->pingsSent = 0;
	client->alive = NO;
	client->name[0] = 0;
}


void gameReset(struct game *game) {

	int i;

	// init the game
	game->mode = GAME_MODE_TAKING_CLIENTS;
	game->clientsN = 0;

	// zero the clients
	for (i = 0; i < CLIENTS_MAX; i++)
		clientReset(game, i);
}


void clientQuit(struct game *game, int clientID) {

	clientReset(game, clientID);

	// decrease the count
	game->clientsN--;

	if (game->clientsN <= 0) {
		// end of game? all quit?

#if defined(DEBUG_MESSAGES)
		fprintf(stderr, "CLIENTQUIT: All clients have quit... Resetting the server...\n");
#endif

		// reset the game (no clients, etc.)
		gameReset(game);
	}
}


void gameDistribute(struct game *game, struct sockaddr_in *addrClient, unsigned char *outputBuffer, socket_t sockFD, int size) {

	int i;

	// write the size
	outputBuffer[0] = size >> 8;
	outputBuffer[1] = size & 0xFF;

	// pass the message to the other clients
	for (i = 0; i < CLIENTS_MAX; i++) {
		// skip clients that have quitted the game
		if (game->clients[i].alive == NO)
			continue;

		addrClient->sin_port = game->clients[i].port;
		addrClient->sin_addr.s_addr = game->clients[i].iP;

		if (sendto(sockFD, outputBuffer, size, 0, (struct sockaddr *)addrClient, sizeof(struct sockaddr)) == -1)
			fprintf(stderr, "GAMEDISTRIBUTE: [1] Error sending a message.\n");
	}
}


void gameDistributeDeath(struct game *game, struct sockaddr_in *addrClient, unsigned char *outputBuffer, socket_t sockFD, int clientID) {

	outputBuffer[2] = 'D';
	outputBuffer[3] = clientID;

	// distribute the data
	gameDistribute(game, addrClient, outputBuffer, sockFD, 4);
}


void gameDistributeStatus(struct game *game, struct sockaddr_in *addrClient, unsigned char *outputBuffer, socket_t sockFD) {

	int i, j, k;

	// header
	j = 2;
	outputBuffer[j++] = 'N';

	// the number of entries
	outputBuffer[j++] = CLIENTS_MAX;

	// names
	for (i = 0; i < CLIENTS_MAX; i++) {
		k = 0;
		while (game->clients[i].name[k] != 0)
			outputBuffer[j++] = game->clients[i].name[k++];
		outputBuffer[j++] = 0;
	}

	// distribute the data
	gameDistribute(game, addrClient, outputBuffer, sockFD, j);
}


void sigHandler(int sig) {

	// make the program exit
	quit = YES;
}

#ifdef WIN32
void gettimeofday(struct timeval *tv, void *time_zone) {
	struct _timeb cur;
	_ftime(&cur);
	tv->tv_sec = cur.time;
	tv->tv_usec = cur.millitm * 1000;
}
#endif

void noIncomingTraffic(struct game *game, struct sockaddr_in *addrClient, unsigned char *outputBuffer, socket_t sockFD) {

#ifdef LINUX
	struct timespec req, rem;
#endif
	struct timeval tv;
	long long ms, dt;
	int i;

#ifdef LINUX
	// sleep for 5ms
	req.tv_sec = 0;
	req.tv_nsec = 5000000;

	nanosleep(&req, &rem);
#endif

#ifdef WIN32
	Sleep(5);
#endif

	// get the current time
	gettimeofday(&tv, NULL);

	// -> ms
	ms = ((long long)tv.tv_sec * 1000) + ((long long)tv.tv_usec / 1000);

	// do we need to ping the clients?
	for (i = 0; i < CLIENTS_MAX; i++) {
		// skip clients that have quitted the game
		if (game->clients[i].alive == NO)
			continue;

		// did we just get something from the client?
		if (game->clients[i].lastPingMS < 0)
			game->clients[i].lastPingMS = ms;

		dt = ms - game->clients[i].lastPingMS;

		if (dt > 1000) {
			// last ping was more than a second ago
			game->clients[i].lastPingMS = ms;

			// timeout the client?
			if (game->clients[i].pingsSent >= clientTimeoutSeconds) {
				// yes!
#if defined(DEBUG_MESSAGES)
				fprintf(stderr, "NOINCOMINGTRAFFIC: %s (%d) timed out.\n", game->clients[i].name, i);
#endif
				// kill the client
				clientQuit(game, i);

				// send all clients the client status list
				gameDistributeStatus(game, addrClient, outputBuffer, sockFD);

				// send all clients the death of client i
				gameDistributeDeath(game, addrClient, outputBuffer, sockFD, i);
			}
			else {
				// no, send a ping
				game->clients[i].pingsSent++;

				// set the size
				outputBuffer[0] = 3 >> 8;
				outputBuffer[1] = 3 & 0xFF;
				outputBuffer[2] = 'p';

				// fix the structure
				addrClient->sin_port = game->clients[i].port;
				addrClient->sin_addr.s_addr = game->clients[i].iP;

				// send
				if (sendto(sockFD, outputBuffer, 3, 0, (struct sockaddr *)addrClient, sizeof(struct sockaddr)) == -1)
					fprintf(stderr, "NOINCOMINGTRAFFIC: [1] Error sending a message.\n");
			}
		}
	}
}


int main(int argc, char *argv[]) {

	struct sockaddr_in addrServer; // my address information
	struct sockaddr_in addrClient; // connector's address information
	unsigned char inputBuffer[INPUT_BUFFER_SIZE];
	unsigned char outputBuffer[INPUT_BUFFER_SIZE];
	unsigned long int clientIP;
	int packetSize;
	int addrSize;
	int i, j, m, clientValid, clientID;
	socket_t sockFD;

#ifdef WIN32
	WSADATA wsaData;
#endif

#ifdef WIN32
	WSAStartup(MAKEWORD(2,0),&wsaData);
#endif

	// exit?
	i = NO;

	// parse the client timeout
	if (argc > 1)
		clientTimeoutSeconds = atoi(argv[1]);

	if (argc != 2 || clientTimeoutSeconds <= 0)
		i = YES;

	if (i == YES) {
		fprintf(stderr, "USAGE: %s <TIMEOUT SECONDS>\n", argv[0]);
		return 1;
	}

	// set the signal handlers
	signal(SIGINT, sigHandler);
	signal(SIGTERM, sigHandler);

	// reset the game (no clients, etc.)
	gameReset(&game);

	// create a socket
	sockFD = socket(AF_INET, SOCK_DGRAM, 0);

	// set the socket to be non-blocking
	i = 1;

#ifdef LINUX
	ioctl(sockFD, FIONBIO, &i);
#endif

#ifdef WIN32
	ioctlsocket(sockFD, FIONBIO, &i);
#endif

	addrServer.sin_family = AF_INET;          // host byte order
	addrServer.sin_port = htons(SERVER_PORT); // short, network byte order
	addrServer.sin_addr.s_addr = INADDR_ANY;  // automatically fill with my IP

	if (bind(sockFD, (struct sockaddr *)&addrServer, sizeof(struct sockaddr)) == -1) {
		fprintf(stderr, "MAIN: Error binding the socket.\n");
		exit(1);
	}

	// print my info
	{
		struct hostent *serverInfo;
		char hostname[256];

		// get my hostname
		gethostname(hostname, 256);

		// get the corresponding IP
		serverInfo = gethostbyname(hostname);

		if (serverInfo != NULL) {
			addrServer.sin_addr.s_addr = *((unsigned long *)(serverInfo->h_addr_list[0]));
			fprintf(stderr, "MAIN: Server running at %s:%d...\n", inet_ntoa(addrServer.sin_addr), ntohs(addrServer.sin_port));
		}
		else
			fprintf(stderr, "MAIN: Server running at %s...\n", hostname);
	}

	// the main loop
	while (1) {
		// exit?
		if (quit == YES)
			break;

		// incoming traffic?
		addrSize = sizeof(struct sockaddr);
		packetSize = recvfrom(sockFD, inputBuffer, INPUT_BUFFER_SIZE, 0, (struct sockaddr *)&addrClient, &addrSize);

		if (packetSize <= 0) {
			// no incoming traffic -> ping?
			noIncomingTraffic(&game, &addrClient, outputBuffer, sockFD);
			continue;
		}

		// TODO: move the following code into its own function

		clientIP = addrClient.sin_addr.s_addr;
		clientValid = NO;

		// client validity check
		for (i = 0; i < CLIENTS_MAX; i++) {
			if (game.clients[i].alive == YES && game.clients[i].iP == clientIP && game.clients[i].port == addrClient.sin_port) {
				clientValid = YES;
				clientID = i;
				break;
			}
		}

		/*
#if defined(DEBUG_MESSAGES)
		fprintf(stderr, "MAIN: Packet from %s:%d.\n", inet_ntoa(addrClient.sin_addr), ntohs(addrClient.sin_port));
#endif
		*/

		// is the packet size ok?
		if (packetSize < 3) {
			fprintf(stderr, "MAIN: The packet (%d bytes) is too small!\n", packetSize);
			continue;
		}

		// is the embedded size ok?
		i = (inputBuffer[0] << 8) | inputBuffer[1];
		if (i != packetSize) {
			fprintf(stderr, "MAIN: The actual packet size (%d bytes) and the embedded packet size (%d bytes) don't match!\n", packetSize, i);
			continue;
		}

		///////////////////////////////////////////////////////////////////////////////////////////////////
		// COMMON MESSAGES
		///////////////////////////////////////////////////////////////////////////////////////////////////

		if (clientValid == YES) {
			// reset the ping counter
			game.clients[clientID].pingsSent = 0;
			game.clients[clientID].lastPingMS = -1;

			if (inputBuffer[2] == 'p') {
				// ping
				continue;
			}
		}

		if (game.mode == GAME_MODE_TAKING_CLIENTS) {
			///////////////////////////////////////////////////////////////////////////////////////////////////
			// GAME_MODE_TAKING_CLIENTS
			///////////////////////////////////////////////////////////////////////////////////////////////////

			// a new client?
			if (inputBuffer[2] == 'J') {
				// yes! return the client ID number, if possible
				for (i = 0; i < CLIENTS_MAX; i++) {
					if (game.clients[i].iP == clientIP && game.clients[i].port == addrClient.sin_port)
						break;
				}

				if (i == CLIENTS_MAX) {
					// we have a new client
					for (i = 0; i < CLIENTS_MAX; i++) {
						if (game.clients[i].iP == 0) {
							game.clients[i].iP = clientIP;
							game.clients[i].port = addrClient.sin_port;
							game.clients[i].alive = YES;
							game.clients[i].pingsSent = 0;
							game.clients[i].lastPingMS = -1;

							// copy the name
							m = 3;

							j = 0;
							while (inputBuffer[m] != 0)
								game.clients[i].name[j++] = inputBuffer[m++];
							game.clients[i].name[j] = 0;

							game.clientsN++;

#if defined(DEBUG_MESSAGES)
							fprintf(stderr, "MAIN: Got a new client \"%s\" %s:%d (%d).\n", game.clients[i].name, inet_ntoa(addrClient.sin_addr), ntohs(game.clients[i].port), i);
#endif
							break;
						}
					}
				}

				if (i != CLIENTS_MAX) {
					outputBuffer[0] = 0; // the size of the message byte 1.
					outputBuffer[1] = 4; //                              2.
					outputBuffer[2] = 'W';
					outputBuffer[3] = i;

					// send the client a welcome!
					if (sendto(sockFD, outputBuffer, 4, 0, (struct sockaddr *)&addrClient, sizeof(struct sockaddr)) == -1)
						fprintf(stderr, "MAIN: [1] Error sending a message.\n");

					// send all clients the client status list
					gameDistributeStatus(&game, &addrClient, outputBuffer, sockFD);
				}

				// are we out of client slots?
				if (i == CLIENTS_MAX) {
					// yes, send an apology
					outputBuffer[0] = 0; // the size of the message byte 1.
					outputBuffer[1] = 3; //                              2.
					outputBuffer[2] = 'S';

					if (sendto(sockFD, outputBuffer, 3, 0, (struct sockaddr *)&addrClient, sizeof(struct sockaddr)) == -1)
						fprintf(stderr, "MAIN: [2] Error sending a message.\n");
				}
			}
			// begin the game?
			else if (inputBuffer[2] == 'B') {
				if (clientValid == YES) {
					// inform the clients
					outputBuffer[2] = 'B';

					// send the message to all clients
					gameDistribute(&game, &addrClient, outputBuffer, sockFD, 3);

					// change the game mode
					game.mode = GAME_MODE_GAME;

#if defined(DEBUG_MESSAGES)
					fprintf(stderr, "MAIN: A game has begun!\n");
#endif
				}
			}
			else {
#if defined(DEBUG_MESSAGES)
				fprintf(stderr, "MAIN: Got an unknown message from %s.\n", inet_ntoa(addrClient.sin_addr));
#endif
			}
		}

		// invalid clients don't get any further
		if (clientValid == NO)
			continue;

		if (game.mode == GAME_MODE_GAME) {
			///////////////////////////////////////////////////////////////////////////////////////////////////
			// GAME_MODE_GAME
			///////////////////////////////////////////////////////////////////////////////////////////////////

			// TODO: analyze the message

			i = 2;

			switch (inputBuffer[i++]) {
			case 'K':
				// KILL
				j = inputBuffer[i];

#if defined(DEBUG_MESSAGES)
				fprintf(stderr, "MAIN: %s (%d) killed %s (%d).\n", game.clients[j].name, j, game.clients[clientID].name, clientID);
#endif

				/*
				// send everybody the updated statistics
				gameDistributeGameStatistics(&game, &addrClient, outputBuffer, sockFD);

				// catalog the kill
				if (j != clientID)
					statsAddKill(&(game.clients[j]), &(game.clients[clientID]));

				// send the statistics
				gameSendStatistics(&game, &addrClient, outputBuffer, sockFD, j);

				if (j != clientID)
					gameSendStatistics(&game, &addrClient, outputBuffer, sockFD, clientID);
				*/

				break;

			case 'D':
				// DEATH - A CLIENT DROPPED OUT

#if defined(DEBUG_MESSAGES)
				fprintf(stderr, "MAIN: %s (%d) has quit the game.\n", game.clients[clientID].name, clientID);
#endif
				// kill the client
				clientQuit(&game, clientID);

				if (game.clientsN > 0) {
					// send this information to all the other clients
					outputBuffer[2] = 'X';
					outputBuffer[3] = clientID;

					// distribute the data
					gameDistribute(&game, &addrClient, outputBuffer, sockFD, 4);
				}

				break;

			case 'Q':
				// QUIT SESSION

				// reset the game (no clients, etc.)
				gameReset(&game);

#if defined(DEBUG_MESSAGES)
				fprintf(stderr, "MAIN: Quit session requested... Resetting...\n");
#endif

				break;

			case 'U':
				// UPDATE

				// pass the message to the other clients
				for (i = 0; i < CLIENTS_MAX; i++) {
					// skip dead clients
					if (game.clients[i].alive == NO)
						continue;
					// skip the sender itself
					if (i == clientID)
						continue;

					addrClient.sin_port = game.clients[i].port;
					addrClient.sin_addr.s_addr = game.clients[i].iP;

					if (sendto(sockFD, inputBuffer, packetSize, 0, (struct sockaddr *)&addrClient, sizeof(struct sockaddr)) == -1) {
						fprintf(stderr, "MAIN: [4] Error sending a message.\n");
					}
				}

				break;
			}

			/*
			// if the first event in the message is '!', then the server should analyze the message
			if (inputBuffer[2] == '!') {
				// yes! analyze


			}

			// pass the message to the other clients
			for (i = 0; i < game.clientsN; i++) {
				// skip the sender itself
				if (i == clientID)
					continue;

				addrClient.sin_port = game.clients[i].port;
				addrClient.sin_addr.s_addr = game.clients[i].iP;

				if (sendto(sockFD, inputBuffer, packetSize, 0, (struct sockaddr *)&addrClient, sizeof(struct sockaddr)) == -1) {
					fprintf(stderr, "MAIN: [4] Error sending a message.\n");
				}
			}
			*/
		}

		// when we reach here, the packet we got from a valid client has been processed
	}

#ifdef LINUX
	close(sockFD);
#endif

#ifdef WIN32
	closesocket(sockFD);
#endif

	return 0;
} 
