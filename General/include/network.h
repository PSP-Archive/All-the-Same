
#ifndef NETWORK_H
#define NETWORK_H

// network buffer for data I/O
extern unsigned char networkBuffer[8*1024];

void networkInit(void);
void networkConnect(void);
void networkDisconnect(void);
void networkServerConnectionInit(char *networkServerName);
void networkRead(void);
void networkServerSend(unsigned char *buffer, int size);

#endif
