
#ifndef GAME_H
#define GAME_H

// the run time
extern float runTime;

// the network server connection statuses
#define NETWORK_SERVER_NO_CONNECTION 0
#define NETWORK_SERVER_CONNECTION    1
#define NETWORK_SERVER_GAME          2

// the server connection status
extern int gameNetworkServerConnectionStatus;

// my ID in the game
extern int gameNetworkMyID;

void gameInit(void);
void gameIteration(float deltaTime);
void gameDraw(void);

void gameNetworkParsePacket(unsigned char *buffer, int size, int packetNumber);
int  gameNetworkCreateUpdatePacket(unsigned char *buffer);
void gameNetworkServerSendUpdate(float deltaTime, float updateInterval);
void gameNetworkServerSendStart(void);
void gameNetworkServerSendJoin(void);

#endif
