/*
CS176B HW2 Swarming ftp client
by Arne Dahl Bjune
arne@dahlbjune.no
*/

#include "mftp.h"
#include "unistd.h"
#include "stdio.h"
#include "stdlib.h"
#include "getopt.h"
#include <sys/socket.h>
#include <netinet/in.h>

//#include "string.h"

struct globalArgs_t {
		int version;		// -v
		char *filename;		// -f
		char *hostname;		// -s
		int portnr;			// -p
		char *username;		// -n
		char *password;		// -P
		int active;			// -a
		char *mode;  		// -m 	
		char *logfile;		// -l
		
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

void display_version(void){
	printf("mftp 0.1\n");
	printf("Writen by Arne Dahl Bjune, arne@dahlbjune.no\n");
	
}

void display_help(void){
	printf("Usage: mftp [OPTION] \n");
	printf("Swarming ftp client\n\n");

	//Skriv inn fullstendig hjelp text
	printf(" -h, --help     display this help and exit\n");
	printf(" -v, --version	output version information and exit\n");

}

void print_globalArgs(void){


	printf("\n-------------- DEBUG -----------------\n");
	printf("Version: %i\n", globalArgs.version);
	printf("Filename: %s\n", globalArgs.filename);
	printf("Hostname: %s\n", globalArgs.hostname);
	printf("Portnr: %i\n", globalArgs.portnr);
	printf("Username: %s\n", globalArgs.username);
	printf("Password: %s\n", globalArgs.password);
	printf("Active: %i\n", globalArgs.active);
	printf("Mode: %s\n", globalArgs.mode);
	printf("Logfile: %s\n", globalArgs.logfile);
}

int connect_to_server(int portnr){
	int sockfd = 0, n = 0;
	struct sockaddr_in serv_addr;

   if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Error : Could not create socket \n");
        return -1;
    } 


	serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portnr); 


    if(inet_pton(AF_INET, globalArgs.hostname, &serv_addr.sin_addr)<=0){
        printf("\n inet_pton error occured\n");
        return -1;
    }

	if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
       printf("\n Error : Connect Failed to %s on port %i \n",globalArgs.hostname,globalArgs.portnr);
       return -1;
    } 


	return sockfd;
}

int main(int argc, char *argv[]) {

	int opt = 0;
	int longIndex;
	int comm_socket;
	char *temp;
	char recvBuffer[1024];
	char sendBuffer[1024];
	int n;
	char *message;
	char *temptest; 

	//Initializing default values
	globalArgs.version = 0;		
	globalArgs.filename = NULL;	
	globalArgs.hostname = NULL;	
	globalArgs.portnr = 21;		
	globalArgs.username = "anonymous";	
	globalArgs.password = "user@localhost.localnet";	
	globalArgs.active = 0;		
	globalArgs.mode = "binary";  	
	globalArgs.logfile = NULL;

	memset(recvBuffer, '0',sizeof(recvBuffer));	
	memset(sendBuffer, '0',sizeof(sendBuffer));	

	

	opt = getopt_long(argc, argv, optString,longOptions,&longIndex);
	while( opt != -1){
		switch( opt){
			case 'v':
				globalArgs.version = 1;
				display_version();
				break;
			case 'h':
				display_help();
				break;
			case 'f':
				globalArgs.filename = optarg;
				break;
			case 's':
				globalArgs.hostname = optarg;
				break;
			case 'p':
				globalArgs.portnr = atoi( optarg );
				break;
			case 'n':
				globalArgs.username = optarg;
				break;
			case 'P':
				globalArgs.password = optarg;
				break; 
			case 'a':
				globalArgs.active = 1;
				break;
			case 'm':
				globalArgs.mode = optarg;
				break;
			case 'l':
				globalArgs.logfile = optarg;
				break;
		}
		
		opt = getopt_long(argc, argv, optString,longOptions,&longIndex);

	}

	//comm_socket = connect_to_server(globalArgs.portnr);
    
    /*
    while ( n > 0){
        n = read(comm_socket, recvBuffer, sizeof(recvBuffer)-1);
        message = "USER ";
        snprintf(sendBuffer, "%s %s\n",message,globalArgs.username);

        write(comm_socket, sendBuffer, sizeof(sendBuffer));
        //recvBuffer = recvBuffer + " kake";
       	recvBuffer[n] = 0;
        printf("%s", recvBuffer);
        sleep(1);
    } 
	*/
		message = "USER ";
		temptest = malloc(snprintf(NULL,0,"%s %s", message, globalArgs.username)+1);
		snprintf(temptest, "%s",message);
        printf("SendBuffer: %s",temptest);

	print_globalArgs();
}
