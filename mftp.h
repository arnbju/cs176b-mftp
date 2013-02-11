/*
CS176B HW2 Swarming ftp client
by Arne Dahl Bjune
arne@dahlbjune.no
*/

#include "unistd.h"
#include "stdio.h"
#include "stdlib.h"
#include "getopt.h"
#include "sys/socket.h"
#include <sys/types.h>
#include "netinet/in.h"
#include "string.h"
#include <regex.h>


#include <errno.h> 
#include <netdb.h>
#include <arpa/inet.h>
#include "pthread.h"

struct globalArgs_t {
		char 	*filename;		// -f
		char 	*hostname;		// -s
		int 	 portnr;		// -p
		char 	*username;		// -n
		char 	*password;		// -P
		int 	 active;		// -a
		char 	*mode;  		// -m 	
		char 	*logfile;		// -l
		int 	logging;
		int 	tid;
		int 	nproc;
		
} globalArgs;	

static const char *optString = "hvf:s:p:n:P:am:l:";

static const struct option longOptions[] = {
	{"help", no_argument, NULL, 'h'},
	{"version", no_argument, NULL, 'v'},
	{"file", required_argument, NULL, 'f'},
	{"hostname", required_argument, NULL, 's'},
	{"port", required_argument, NULL, 'p'},
	{"username", required_argument, NULL, 'n'},
	{"password", required_argument, NULL, 'P'},
	{"active", required_argument, NULL, 'a'},
	{"mode", required_argument, NULL, 'm'},
	{"logfile", required_argument, NULL, 'l'},
	{NULL, no_argument, NULL, 0}
};
