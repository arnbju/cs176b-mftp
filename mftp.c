/*
CS176B HW2 Swarming ftp client
by Arne Dahl Bjune
arne@dahlbjune.no
*/

#include "mftp.h"

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
	printf("Filename: %s\n", globalArgs.filename);
	printf("Hostname: %s\n", globalArgs.hostname);
	printf("Portnr: %i\n", globalArgs.portnr);
	printf("Username: %s\n", globalArgs.username);
	printf("Password: %s\n", globalArgs.password);
	printf("Active: %i\n", globalArgs.active);
	printf("Mode: %s\n", globalArgs.mode);
	printf("Logfile: %s\n", globalArgs.logfile);
}

int hostname_translation(char * hostname){
	struct hostent *he;
	struct in_addr **addr_list;
		
	if ( (he = gethostbyname( hostname ) ) == NULL) {
		herror("gethostbyname");
		return 1;
	}

	addr_list = (struct in_addr **) he->h_addr_list;

	strcpy(hostname , inet_ntoa(*addr_list[0]) );
	return 0;
}


int connect_to_server(int portnr){
	int sockfd = 0, n = 0;
	struct sockaddr_in serv_addr;

   if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
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

int authenticate(int comm_socket){
	
	char recvBuffer[1024];
	char sendBuffer[1025];
	int n;

    n = read(comm_socket, recvBuffer, sizeof(recvBuffer)-1);
    recvBuffer[n] = 0;
    printf("R: %s", recvBuffer);
    
    if(strncmp(recvBuffer,"220",3)==0){
    	strcpy(sendBuffer, "USER ");
		strcat(sendBuffer, globalArgs.username);
    	strcat(sendBuffer,"\r\n");
    	printf("S: %s", sendBuffer);
    	write(comm_socket, sendBuffer, strlen(globalArgs.username) + 7);

    }else{
    	return -1;
    }
    
    n = read(comm_socket, recvBuffer, sizeof(recvBuffer)-1);
    recvBuffer[n] = 0;
    printf("R: %s", recvBuffer);
    
    if(strncmp(recvBuffer,"331",3)==0){
    	strcpy(sendBuffer, "PASS ");
		strcat(sendBuffer, globalArgs.password);
    	strcat(sendBuffer,"\r\n");
    	printf("S: %s", sendBuffer);
    	write(comm_socket, sendBuffer, strlen(globalArgs.password) + 7);

    }else{
    	return -1;
    }
    sleep(1);
    n = read(comm_socket, recvBuffer, sizeof(recvBuffer)-1);
    recvBuffer[n] = 0;
    printf("R: %s", recvBuffer);
    
    if(strncmp(recvBuffer,"230",3)==0){
    	
    	return 0;

    }else{
    	return -1;
    }
    
    
    return 0;
}

int main(int argc, char *argv[]) {

	int opt = 0;
	int longIndex;
	int comm_socket;
	int authenticated;
	char *temp;
	char recvBuffer[1024];
	char sendBuffer[1024];
	int n;
	char *message;
	char *temptest; 

	//Initializing default values
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
				display_version();
				return 0;
			case 'h':
				display_help();
				return 0;
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

	hostname_translation(globalArgs.hostname);
	comm_socket = connect_to_server(globalArgs.portnr);
    
    if(authenticated = authenticate(comm_socket)){
    	printf("Authentification Failed \n");
    	return -10;
    }
    
    n = 1;
/*
	while ( n > 0){
        sleep(1);
        n = read(comm_socket, recvBuffer, sizeof(recvBuffer)-1);
        recvBuffer[n] = 0;
        printf("  --  \nR: %s", recvBuffer);

    }
*/
    if(globalArgs.active==0){
	    
	    
    	strcpy(sendBuffer, "PASV\r\n");
		printf("S: %s", sendBuffer);
    	write(comm_socket, sendBuffer, strlen(globalArgs.username) + 7);
		
		n = read(comm_socket, recvBuffer, sizeof(recvBuffer)-1);
	    recvBuffer[n] = 0;
	    printf("R: %s", recvBuffer);
	    

	    //regexp for a hente ut portnr og ip fra server
	    	
    }
	

        
	print_globalArgs();
}
