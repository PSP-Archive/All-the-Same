
/*
 *
 * network.c - The Posix network subsystem
 *
 */

#define WIN32_LEAN_AND_MEAN		// exclude rarely used stuff from Windows headers

#ifdef WIN32
#include <windows.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#endif

#ifdef LINUX
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#endif

#ifdef PSP
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspsdk.h>
#include <stdlib.h>
#include <string.h>
#include <pspnet.h>
#include <pspnet_inet.h>
#include <pspnet_apctl.h>
#include <pspnet_resolver.h>
#include <psputility_netmodules.h>
#include <psputility_netparam.h>
#include <pspwlan.h>
#include <errno.h>
#endif

#ifdef WIN32
#include <winsock2.h>
#else
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#include "common.h"
#include "network.h"
#include "game.h"
#include "random.h"

// -----------------------------------------------------------------------
// variables
// -----------------------------------------------------------------------

// server port
unsigned int networkServerPort;

// my port
unsigned int networkClientPort;

// server connection status
int networkServerConnectionStatus;

// the packet numbers
int networkPacketNumberOut;
int networkPacketNumberIn;

// address data structures
struct sockaddr_in networkServerAddress;
struct sockaddr_in networkClientAddress;

// I/O socket
#ifdef WIN32
SOCKET networkSocketFD;
#else
int networkSocketFD;
#endif

// network buffer for data I/O
unsigned char networkBuffer[8*1024];

// server timeout (in seconds)
int networkServerTimeout;

// the last time we got a ping from the server
int networkServerPingTime;

// the network send timer
float networkSendTimer;

// -----------------------------------------------------------------------
// init
// -----------------------------------------------------------------------

void networkInit(void) {

  struct sockaddr_in addrMy;

#ifdef WIN32
  WSADATA wsaData;

  WSAStartup(MAKEWORD(2,0), &wsaData);
#endif

  networkSocketFD = -1;
  networkPacketNumberIn = 0;
  networkPacketNumberOut = 0;

  // init the client address structure
  networkClientAddress.sin_family = AF_INET;

  // reset the server port
  networkServerPort = 9999;

	/*
#ifdef PSP
	if (pspSdkLoadInetModules() < 0)
		return;
#endif
	*/

	// create a socket
	networkSocketFD = socket(AF_INET, SOCK_DGRAM, 0);

#ifdef WIN32
	{
		int i;

		// set the socket to be non-blocking
		i = 1;
		ioctlsocket(networkSocketFD, FIONBIO, &i);
	}
#endif

	// reset my port
	networkClientPort = 10000 + (randomNextInt() & 31);

	// bind my socket
	addrMy.sin_family = AF_INET;
	addrMy.sin_port = htons(networkClientPort);
	addrMy.sin_addr.s_addr = INADDR_ANY;        // automatically fill with my IP

	bind(networkSocketFD, (struct sockaddr *)&addrMy, sizeof(struct sockaddr));
}

// -----------------------------------------------------------------------
// send a packet to the server
// -----------------------------------------------------------------------

void networkServerSend(unsigned char *buffer, int size) {

  // embed the message size into the message
  buffer[0] = size >> 8;
  buffer[1] = size & 0xFF;

  sendto(networkSocketFD, buffer, size, 0, (struct sockaddr *)&networkServerAddress, sizeof(struct sockaddr));

	networkPacketNumberOut++;
}

// -----------------------------------------------------------------------
// connect to the network
// -----------------------------------------------------------------------

#ifdef PSP
int networkPSPStatus = 0;
#endif

void networkConnect(void) {

#ifdef PSP
	int err, state;

	if (networkPSPStatus == 3)
		return;

	err = pspSdkInetInit();
	if (err != 0)
		return;

	networkPSPStatus = 1;

	// connect to the first AP
	err = sceNetApctlConnect(0);
	if (err != 0)
		return;

	networkPSPStatus = 2;

	// wait until we have a connection
	while (1) {
		err = sceNetApctlGetState(&state);
		if (err != 0)
			break;

		// connected?
		if (state >= 4)
			break;

		// not trying?
		if (state < 0)
			break;

		// wait 50ms before polling again
		sceKernelDelayThread(50*1000);
	}

	networkPSPStatus = 3;
#endif
}

// -----------------------------------------------------------------------
// disconnect from the network
// -----------------------------------------------------------------------

void networkDisconnect(void) {

}

// -----------------------------------------------------------------------
// connect to the game server
// -----------------------------------------------------------------------

void networkServerConnectionInit(char *networkServerName) {

  struct hostent *serverInfo;

	if ((serverInfo = gethostbyname(networkServerName)) == NULL)
		return;

	// fill in the server information
	networkServerAddress.sin_family = AF_INET;
	networkServerAddress.sin_port = htons(networkServerPort);
	networkServerAddress.sin_addr.s_addr = *((unsigned long *)(serverInfo->h_addr));
}

// -----------------------------------------------------------------------
// read the input from network
// -----------------------------------------------------------------------

void networkRead(void) {

	struct sockaddr_in inputAddress;
	unsigned int len;
  int i, size;

  // get all the messages
  while (1) {
    len = sizeof(struct sockaddr);
#ifdef WIN32
    i = recvfrom(networkSocketFD, networkBuffer, 8*1024, 0, (struct sockaddr *)&inputAddress, &len);
#else
    i = recvfrom(networkSocketFD, networkBuffer, 8*1024, MSG_DONTWAIT, (struct sockaddr *)&inputAddress, &len);
#endif

    // nothing in the queue?
    if (i < 3)
      return;

		networkPacketNumberIn++;

    // parse the payload size
    size = (networkBuffer[0] << 8) | networkBuffer[1];

    // is the packet ok?
    if (size != i)
			return;

		// let the game handle the packet
		gameNetworkParsePacket(networkBuffer, size, networkPacketNumberIn);
	}
}
