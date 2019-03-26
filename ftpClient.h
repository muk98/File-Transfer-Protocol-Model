#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <errno.h> 
#include <netdb.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <string.h>
#include<time.h>
#include <fcntl.h>
#include <ifaddrs.h> 
#include <dirent.h>


#define SA struct sockaddr_in 
#define MAXLEN 100
#define LISTENQ 100
FILE *fp;

