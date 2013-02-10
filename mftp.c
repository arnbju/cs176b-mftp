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
	char sendBuffer[1024];
	int n;

    n = read(comm_socket, recvBuffer, sizeof(recvBuffer)-1);
    recvBuffer[n] = 0;
    if(globalArgs.logging==1) logToFile(recvBuffer,0);
    //printf("R: %s", recvBuffer);
    
    if(strncmp(recvBuffer,"220",3)==0){
    	strcpy(sendBuffer, "USER ");
		strcat(sendBuffer, globalArgs.username);
    	strcat(sendBuffer,"\r\n");
    	if(globalArgs.logging==1) logToFile(sendBuffer,1);
    	//printf("S: %s", sendBuffer);
    	write(comm_socket, sendBuffer, strlen(globalArgs.username) + 7);

    }else{
    	return -1;
    }
    
    n = read(comm_socket, recvBuffer, sizeof(recvBuffer)-1);
    recvBuffer[n] = 0;
    if(globalArgs.logging==1) logToFile(recvBuffer,0);
    //printf("R: %s", recvBuffer);
    
    if(strncmp(recvBuffer,"331",3)==0){
    	strcpy(sendBuffer, "PASS ");
		strcat(sendBuffer, globalArgs.password);
    	strcat(sendBuffer,"\r\n");
    	if(globalArgs.logging==1) logToFile(sendBuffer,1);
    	//printf("S: %s", sendBuffer);
    	write(comm_socket, sendBuffer, strlen(globalArgs.password) + 7);

    }else{
    	return -1;
    }
    sleep(1);
    n = read(comm_socket, recvBuffer, sizeof(recvBuffer)-1);
    recvBuffer[n] = 0;
    if(globalArgs.logging==1) logToFile(recvBuffer,0);
    //printf("R: %s", recvBuffer);
    
    if(strncmp(recvBuffer,"230",3)==0){
    	
    	return 0;

    }else{
    	return -1;
    }
    
    
    return 0;
}

int match_with_regexp (char * expression, char * text, int size, char results[]){
    regex_t r;
    int i;
    regmatch_t m[10];

    regcomp (&r, expression, REG_EXTENDED|REG_NEWLINE);
	/*
    int status = regcomp (&r, expression, REG_EXTENDED|REG_NEWLINE); //kopi
    if (status != 0) {
    char error_message[MAX_ERROR_MSG];
    regerror (status, &r, error_message, MAX_ERROR_MSG);
        printf ("Regex error compiling '%s': %s\n",
                 expression, error_message);
        return 1;
    }
 	*/   
    
    regexec (&r, text, 10, m, 0);

    for(i = 1; i < 10;i++){
        if (m[i].rm_so == -1) {
            break;
        }
        memcpy(&results[size*(i-1)],&text[m[i].rm_so],m[i].rm_eo-m[i].rm_so);
        results[(i-1)*size + m[i].rm_eo-m[i].rm_so] = '\0';
    }
    
      return 0;
}

int settings_from_file (char *ftpserver){
	char ipandport[8*15];
	char user[15];
	char pass[15];
	char file[15];
    char file_regexp[129];

	ftpserver = "ftp://socket:programming@192.168.0.2/cook.pdf";
	char *testtemp;

	strcpy(file_regexp,"([[:alnum:]]+)\\:\\/\\/([[:alnum:]]+)\\:([[:alnum:]]+)@([[:digit:]]+)\\.([[:digit:]]+)\\.([[:digit:]]+)\\.([[:digit:]]+)(\\/[[:print:]]+)");

	match_with_regexp(file_regexp,ftpserver,15,ipandport);
	memcpy(user,&ipandport[1*15],15);
	memcpy(pass,&ipandport[2*15],15);
	memcpy(file,&ipandport[7*15],15);
	printf("User %s\n", user);
	//	Trenger medtode for a kopiere til globalArgs, strcpy segfaulter

}

void set_type_binary(int socket){
	char recvBuffer[1024];
	char sendBuffer[1024];
	int n;

	strcpy(sendBuffer, "TYPE I\r\n");
	if(globalArgs.logging==1) logToFile(sendBuffer,1);

	
	//printf("S: %s", sendBuffer);
   	write(socket, sendBuffer, 8);
		 	
   	n = read(socket, recvBuffer, sizeof(recvBuffer)-1);
    recvBuffer[n] = 0;
    if(globalArgs.logging==1) logToFile(recvBuffer,0);
    //printf("R: %s", recvBuffer);


}

void set_type_ascii(int socket){
	char recvBuffer[1024];
	char sendBuffer[1024];
	int n;

	strcpy(sendBuffer, "TYPE A\r\n");
	if(globalArgs.logging==1) logToFile(sendBuffer,1);
	//printf("S: %s", sendBuffer);
   	write(socket, sendBuffer, 8);
	
	 	
   	n = read(socket, recvBuffer, sizeof(recvBuffer)-1);
    recvBuffer[n] = 0;
    if(globalArgs.logging==1) logToFile(recvBuffer,0);
    //printf("R: %s", recvBuffer);


}

int set_mode_passive(int socket){
	char recvBuffer[1024];
	char sendBuffer[1024];
	int n, dataport;
    char pasv_regexp[125];
	char ipandport[6*4];
	char portval1[4];
	char portval2[4];

	strcpy(pasv_regexp,"[[:digit:]]+[^[:digit:]]+\\(([[:digit:]]+)\\,([[:digit:]]+)\\,([[:digit:]]+)\\,([[:digit:]]+)\\,([[:digit:]]+)\\,([[:digit:]]+)\\)\\.");

	strcpy(sendBuffer, "PASV\r\n");
	if(globalArgs.logging==1) logToFile(sendBuffer,1);
	//printf("S: %s", sendBuffer);
	write(socket, sendBuffer, 6);
	
	n = read(socket, recvBuffer, sizeof(recvBuffer)-1);
    recvBuffer[n] = 0;
    if(globalArgs.logging==1) logToFile(recvBuffer, 0);
   // printf("R: %s", recvBuffer);
    
    match_with_regexp(pasv_regexp,recvBuffer,4,ipandport);
    memcpy(portval1,&ipandport[4*4],4);
    memcpy(portval2,&ipandport[5*4],4);
    dataport = atoi(portval1)*256 + atoi(portval2);

    return dataport;
}

int get_file_from_server(int socket, int port, int sizeAndSocket[]){
	char recvBuffer[1024];
	char sendBuffer[1024];
	int n;
	char byte_regexp[25]; 
	char numberOfBytesToRecieve[100];

	strcpy(byte_regexp, "\\(([[:digit:]]+)\\ bytes\\)");

	strcpy(sendBuffer, "RETR ");
	strcat(sendBuffer, globalArgs.filename);
	strcat(sendBuffer, "\r\n");
	if(globalArgs.logging==1) logToFile(sendBuffer,1);
	//printf("S: %s", sendBuffer);
	write(socket, sendBuffer, strlen(globalArgs.filename) + 7);


 	sizeAndSocket[0] = connect_to_server(port);
	n = read(socket, recvBuffer, sizeof(recvBuffer)-1);
    recvBuffer[n] = 0;
 	if(globalArgs.logging==1) logToFile(recvBuffer,0);
    //printf("R: %s", recvBuffer);


    match_with_regexp(byte_regexp,recvBuffer,100,numberOfBytesToRecieve);

    sizeAndSocket[1] = atoi(numberOfBytesToRecieve);

    return 0;
}


int save_file_from_server_binary(int socket, int numberOfBytes){
	int bytesLeft = numberOfBytes;
	unsigned char recvBuffer[1024];
	char sendBuffer[1024];
	int n;
	FILE *file;

	file = fopen(globalArgs.filename,"w");
	sleep(1);
	while(bytesLeft > 0){
		n = read(socket, recvBuffer, sizeof(recvBuffer)-1);
	    fwrite(recvBuffer, sizeof(recvBuffer[0]), n, file);
	    bytesLeft = bytesLeft - n;

	}
    fclose(file);
    //printf("R: %s", recvBuffer);

}

int save_file_from_server_ascii(int socket, int numberOfBytes){
	int bytesLeft = numberOfBytes;
	unsigned char recvBuffer[1024];
	char sendBuffer[1024];
	int n;
	FILE *file;

	file = fopen(globalArgs.filename,"w");
	sleep(1);
	while(bytesLeft > 0){
		n = read(socket, recvBuffer, sizeof(recvBuffer)-1);
	    recvBuffer[n] = 0;
	    fprintf(file, "%s", recvBuffer);
	    bytesLeft = bytesLeft - n;
	}
    fclose(file);
    //printf("R: %s", recvBuffer);
}

int logToFile(char logtext[], int send){
	char temp[1024 + 6];

	if(send == 1){
		strcpy(temp,"C->S: ");
	}else{
		strcpy(temp,"S->C: ");
	}
	strcat(temp,logtext);

	if(!strcmp(globalArgs.logfile,"-")){
		printf("%s", &temp[0]);
	}else{
		
		FILE *file;	
		file = fopen(globalArgs.logfile,"a+");
		fprintf(file, "%s", temp);
		fclose(file);
	}
	return 0;
	
}

int main(int argc, char *argv[]) {

	int opt = 0;
	int longIndex;
	int comm_socket, data_socket;
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
	globalArgs.logging = 0;

	//default vaules for testing
	globalArgs.filename = "polarbear.jpg";
	globalArgs.hostname = "128.111.68.216";

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
				//temp value for testing
				globalArgs.filename = "test.txt";
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
				globalArgs.logging = 1;
				break;
		}
		
		opt = getopt_long(argc, argv, optString,longOptions,&longIndex);

	}
	if(0){
		printf("No server specified, defaulting to 128.111.68.216\n");
		//globalArgs.hostname = "ftp.ucsb.edu\n";
	}
//	printf("Server er : %s\n", globalArgs.hostname);
	hostname_translation(globalArgs.hostname); //feiler nar adresse ikke er satt fra comandolinje
//	printf("Hostname funnet\n");
	
	if(!(comm_socket = connect_to_server(globalArgs.portnr))){
		printf("Can't connect to server \n");;
    }
    if(authenticated = authenticate(comm_socket)){
    	printf("Authentification Failed \n");
    	return -10;
    }
    
    n = 1;

    if(globalArgs.active==0){
	    int socketAndSize[2];
	    int dataport;
	 	
	 
    	dataport = set_mode_passive(comm_socket);
	    
		if (globalArgs.mode == "binary"){
			set_type_binary(comm_socket);
		}else{
			set_type_ascii(comm_socket);
		}


		get_file_from_server(comm_socket,dataport,socketAndSize);
   
	    
	 	save_file_from_server_binary(socketAndSize[0],socketAndSize[1]);



    }
	

//    settings_from_file("testing");
	print_globalArgs();
}
