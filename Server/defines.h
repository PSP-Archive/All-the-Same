
#ifndef DEFINES_H
#define DEFINES_H

#ifndef M_PI
#define M_PI 3.14159265
#endif

#ifdef FAILED
#undef FAILED
#endif
#ifdef SUCCEEDED
#undef SUCCEEDED
#endif

#define FAILED    0
#define SUCCEEDED 1

#define NO  0
#define YES 1

#define OFF 0
#define ON  1

#ifdef LINUX
typedef int socket_t;
#endif

#ifdef WIN32
typedef SOCKET socket_t;
#endif


#endif
