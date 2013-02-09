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
    char pasv_regexp[125]; 


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
	strcpy(pasv_regexp,"[[:digit:]]+[^[:digit:]]+\\(([[:digit:]]+)\\,([[:digit:]]+)\\,([[:digit:]]+)\\,([[:digit:]]+)\\,([[:digit:]]+)\\,([[:digit:]]+)\\)\\.");
	

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
	if(comm_socket = connect_to_server(globalArgs.portnr)){
		printf("Cannont connect to server \n");;
    }
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
	    char ipandport[6*4];
	    char portval1[4];
	    char portval2[4];
	    int dataport;
	 

    	strcpy(sendBuffer, "PASV\r\n");
		printf("S: %s", sendBuffer);
    	write(comm_socket, sendBuffer, strlen(globalArgs.username) + 7);
		
		n = read(comm_socket, recvBuffer, sizeof(recvBuffer)-1);
	    recvBuffer[n] = 0;
	    printf("R: %s", recvBuffer);
	    
	    match_with_regexp(pasv_regexp,recvBuffer,4,ipandport);
	    memcpy(portval1,&ipandport[4*4],4);
	    memcpy(portval2,&ipandport[5*4],4);
	    dataport = atoi(portval1)*256 + atoi(portval2);
	 	    	
    }
	

    settings_from_file("testing");
	print_globalArgs();
}
