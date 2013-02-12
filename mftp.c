/*
CS176B HW2 Swarming ftp client
by Arne Dahl Bjune
arne@dahlbjune.no
*/

#include "mftp.h"

int nthreads = 1;
struct ftpArgs_t thread_data[1];


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

void print_globalArgs(void *structure){

	struct globalArgs_t *gArgs;
	gArgs = (struct globalArgs_t *)structure;

	printf("\n-------------- DEBUG -----------------\n");
	printf("Filename: %s\n", gArgs->filename);
	printf("Hostname: %s\n", gArgs->hostname);
	printf("Portnr: %i\n", gArgs->portnr);
	printf("Username: %s\n", gArgs->username);
	printf("Password: %s\n", gArgs->password);
	printf("Active: %i\n", gArgs->active);
	printf("Mode: %s\n", gArgs->mode);
	printf("Logfile: %s\n", gArgs->logfile);
}

void print_ftpArgs(void *structure){

	struct ftpArgs_t *fArgs;
	fArgs = (struct ftpArgs_t *)structure;

	printf("\n-------------- DEBUG - ftpArgs ----------------\n");
	printf("Filename: %s\n", fArgs->filename);
	printf("Hostname: %s\n", fArgs->hostname);
	printf("Portnr: %i\n", fArgs->portnr);
	printf("Username: %s\n", fArgs->username);
	printf("Password: %s\n", fArgs->password);
	printf("Tid: %d\n", fArgs->tid);
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

int connect_to_server(int portnr, char *hostname){
	int sockfd = 0, n = 0;
	struct sockaddr_in serv_addr;

   if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("\n Error : Could not create socket \n");
        return -1;
    } 


	serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portnr); 


    if(inet_pton(AF_INET, hostname, &serv_addr.sin_addr)<=0){
        printf("\n inet_pton error occured\n");
        return -1;
    }

	if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
       printf("\n Error : Connect Failed to %s on port %i \n",hostname,portnr);
       return -1;
    } 


	return sockfd;
}

int authenticate(int comm_socket,char *username, char *password){
	
	char recvBuffer[1024];
	char sendBuffer[1024];
	int n;

    n = read(comm_socket, recvBuffer, sizeof(recvBuffer)-1);
    recvBuffer[n] = 0;
    if(globalArgs.logging==1) logToFile(recvBuffer,0);
    //printf("R: %s", recvBuffer);
    
    if(strncmp(recvBuffer,"220",3)==0){
    	strcpy(sendBuffer, "USER ");
		strcat(sendBuffer, username);
    	strcat(sendBuffer,"\r\n");
    	if(globalArgs.logging==1) logToFile(sendBuffer,1);
    	//printf("S: %s", sendBuffer);
    	write(comm_socket, sendBuffer, strlen(username) + 7);

    }else{
    	return -1;
    }
    
    n = read(comm_socket, recvBuffer, sizeof(recvBuffer)-1);
    recvBuffer[n] = 0;
    if(globalArgs.logging==1) logToFile(recvBuffer,0);
    //printf("R: %s", recvBuffer);
    
    if(strncmp(recvBuffer,"331",3)==0){
    	strcpy(sendBuffer, "PASS ");
		strcat(sendBuffer, password);
    	strcat(sendBuffer,"\r\n");
    	if(globalArgs.logging==1) logToFile(sendBuffer,1);
    	
    	write(comm_socket, sendBuffer, strlen(password) + 7);

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
    
    i = regexec (&r, text, 10, m, 0);
    if(i != 0){
    	return -1;
    }

    for(i = 1; i < 10;i++){
        if (m[i].rm_so == -1) {
            break;
        }
        memcpy(&results[size*(i-1)],&text[m[i].rm_so],m[i].rm_eo-m[i].rm_so);
        results[(i-1)*size + m[i].rm_eo-m[i].rm_so] = '\0';
    }
    
      return 0;
}

int settings_from_file (char *filename, void *ftpArgsP){
	//Takler ikke at swarming file manger brukernav og passord

    int i = 0;
    int regexp;
	char *user;
	char *pass;
	char *file;
	char *hostname;
	char ipandport[7*15];
    char temp[40];
    char file_regexp[129];
    char line[100];

	FILE *swarmfile;
	struct ftpArgs_t *ftpArgs;
	ftpArgs = (struct ftpArgs_t *)ftpArgsP;

	memset(ipandport,0,sizeof(ipandport));
	strcpy(file_regexp,"ftp\\:\\/\\/([[:alnum:]]+)\\:([[:alnum:]]+)@([[:digit:]]+)\\.([[:digit:]]+)\\.([[:digit:]]+)\\.([[:digit:]]+)\\/([[:print:]]+)");
    
    swarmfile = fopen(filename, "rt");
    while(fgets(line, 100, swarmfile) != 0){
    	if(strlen(line)>8){
	    	
	    	printf("Total lengde: %d\n", strlen(line));
			regexp = match_with_regexp(file_regexp,line,15,ipandport);
			if(regexp != 0){
				//invalid input
				return -1;
			}
			user = malloc(strlen(&ipandport[0*15]));
			strcpy(user, &ipandport[0*15]);
			
			pass = malloc(strlen(&ipandport[1*15]));
			strcpy(pass, &ipandport[1*15]);
			
			file = malloc(strlen(&ipandport[6*15]));
			strcpy(file,&ipandport[6*15]);
			
			sprintf(temp, "%s.%s.%s.%s",&ipandport[2*15],&ipandport[3*15],&ipandport[4*15],&ipandport[5*15]);
			hostname = malloc(strlen(temp));
			strcpy(hostname,temp);
			
			ftpArgs->username = user;
			ftpArgs->password = pass;
			ftpArgs->hostname = hostname;
			ftpArgs->filename = file;
	    	printf("%s\n", line);
	    	i++;
    
    		
    	}
    }
    printf("Total %d lines\n", i);
  
	return 0;
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

int build_port_message(int port, char ip[]){
	int port1,port2;
	char ip_regexp[63];
	char my_ip[4*4];

	strcpy(ip_regexp,"([[:digit:]]+)\\.([[:digit:]]+)\\.([[:digit:]]+)\\.([[:digit:]]+)");

	port2 = port % 256;
    port1 = (port - port2) / 256;

	match_with_regexp(ip_regexp,ip,4,my_ip);

	sprintf(ip, "PORT %s,%s,%s,%s,%d,%d\r\n", &my_ip[0],&my_ip[4],&my_ip[8],&my_ip[12],port1,port2);
	//	printf("%s\n", ip);

	return 0;
}

int set_mode_active(int comm_socket){
	int data_socket, connection_socket, n;
	struct sockaddr_in client_addr;
	struct sockaddr_in temp;
	char hostname[50];
	
	char	sendBuffer[1024];
	char	recvBuffer[1024];

	memset(&client_addr,'0',sizeof(client_addr));
	client_addr.sin_family = AF_INET;
	client_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	client_addr.sin_port = htons(0);
	socklen_t adressLenth = sizeof(client_addr);

	if((data_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("\n Error : Could not create socket \n");
        return -1;
    }

    bind(data_socket,(struct sockaddr*)&client_addr,sizeof(client_addr));
    
    getsockname(data_socket, (struct sockaddr*)&temp,&adressLenth);
	gethostname(hostname,sizeof(hostname));
	hostname_translation(hostname);
	build_port_message((int) ntohs(temp.sin_port),hostname);

	strcpy(sendBuffer,hostname);
	if(globalArgs.logging==1) logToFile(sendBuffer,1);
	write(comm_socket, sendBuffer, strlen(hostname));

	n = read(comm_socket, recvBuffer, sizeof(recvBuffer)-1);
    recvBuffer[n] = 0;
    if(globalArgs.logging==1) logToFile(recvBuffer, 0);
 
    listen(data_socket,10);

	return data_socket;
}

int retrive_and_get_filesize_from_server(int socket, char *filename){
	char recvBuffer[1024];
	char sendBuffer[1024];
	int n;
	char byte_regexp[25]; 
	char numberOfBytesToRecieve[100];

	strcpy(byte_regexp, "\\(([[:digit:]]+)\\ bytes\\)");

	strcpy(sendBuffer, "RETR ");
	strcat(sendBuffer, filename);
	strcat(sendBuffer, "\r\n");
	if(globalArgs.logging==1) logToFile(sendBuffer,1);
	write(socket, sendBuffer, strlen(filename) + 7);

	n = read(socket, recvBuffer, sizeof(recvBuffer)-1);
    recvBuffer[n] = 0;
 	if(globalArgs.logging==1) logToFile(recvBuffer,0);

    match_with_regexp(byte_regexp,recvBuffer,100,numberOfBytesToRecieve);

    return atoi(numberOfBytesToRecieve);
}

int save_file_from_server_binary(int socket, int numberOfBytes, char *filename){
	int bytesLeft = numberOfBytes;
	unsigned char recvBuffer[1024];
	char sendBuffer[1024];
	int n;
	FILE *file;

	file = fopen(filename,"w");
	sleep(0.1);
	while(bytesLeft > 0){
		n = read(socket, recvBuffer, sizeof(recvBuffer)-1);
	    fwrite(recvBuffer, sizeof(recvBuffer[0]), n, file);
	    bytesLeft = bytesLeft - n;

	}
    fclose(file);
}

int save_file_from_server_ascii(int socket, int numberOfBytes, char *filename){
	int bytesLeft = numberOfBytes;
	unsigned char recvBuffer[1024];
	char sendBuffer[1024];
	int n;
	FILE *file;

	file = fopen(filename,"w");
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

int close_connection(int socket){
	char recvBuffer[1024];
	char sendBuffer[1024];
	int n;


	n = read(socket, recvBuffer, sizeof(recvBuffer)-1);
    recvBuffer[n] = 0;
 	if(globalArgs.logging==1) logToFile(recvBuffer,0);


	strcpy(sendBuffer, "QUIT\r\n");
	if(globalArgs.logging==1) logToFile(sendBuffer,1);
	//printf("S: %s", sendBuffer);
	write(socket, sendBuffer, 6);
	
	n = read(socket, recvBuffer, sizeof(recvBuffer)-1);
    recvBuffer[n] = 0;
 	if(globalArgs.logging==1) logToFile(recvBuffer,0);
}

void *download_with_ftp(void *settings){
	char recvBuffer[1024];
	char sendBuffer[1024];
	int n;
	int comm_socket, data_socket;
	int authenticated;
	struct ftpArgs_t *ftpArgs;
	ftpArgs = (struct ftpArgs_t *)settings;


	memset(recvBuffer, '0',sizeof(recvBuffer));	
	memset(sendBuffer, '0',sizeof(sendBuffer));	

	hostname_translation(ftpArgs->hostname); //feiler nar adresse ikke er satt fra comandolinje

	
	if(!(comm_socket = connect_to_server(ftpArgs->portnr,ftpArgs->hostname))){
		printf("Can't connect to server \n");;
    }
    if(authenticated = authenticate(comm_socket, ftpArgs->username, ftpArgs->password)){
    	printf("Authentification Failed \n");
    	//return -10;
    }

    
    n = 1;

    if(globalArgs.active==0){
	    int socketAndSize[2];
	    int dataport,size,data_socket;
	 
    	dataport = set_mode_passive(comm_socket);
	    
		if (globalArgs.mode == "binary"){
			set_type_binary(comm_socket);
		}else{
			set_type_ascii(comm_socket);
		}

		data_socket = connect_to_server(dataport,ftpArgs->hostname);
		size = retrive_and_get_filesize_from_server(comm_socket, ftpArgs->filename);
   
		if (globalArgs.mode == "binary"){
			char filtest[50];
			strcpy(filtest, ftpArgs->password);
			strcat(filtest, ftpArgs->filename);
		 	save_file_from_server_binary(data_socket,size, filtest);
		}else{
		 	save_file_from_server_ascii(data_socket,size, ftpArgs->filename);
		}
	printf("Running download_with_ftp\n");

		close_connection(comm_socket);


    }else{
    	int size;
    	int data_socket = set_mode_active(comm_socket);
    	int connection_socket;
		if (globalArgs.mode == "binary"){
			set_type_binary(comm_socket);
		}else{
			set_type_ascii(comm_socket);
		}
		
		size = retrive_and_get_filesize_from_server(comm_socket, ftpArgs->filename);
		
		connection_socket = accept(data_socket,(struct sockaddr*)NULL, NULL);

		if (globalArgs.mode == "binary"){
		 	save_file_from_server_binary(connection_socket,size, ftpArgs->filename);
		}else{
		 	save_file_from_server_ascii(connection_socket,size, ftpArgs->filename);
		}		
		close_connection(comm_socket);
    }

    return 0;
}

void fill_thread_data(int i){
	thread_data[0].filename = globalArgs.filename;
	thread_data[0].hostname = globalArgs.hostname;
	thread_data[0].portnr = globalArgs.portnr;
	thread_data[0].username = globalArgs.username;
	thread_data[0].password = globalArgs.password;
	thread_data[0].tid = i;
}

int main(int argc, char *argv[]) {
	int i;
	int opt = 0;
	int longIndex;
	char *temp;
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
	globalArgs.swarmfile = NULL;
	globalArgs.swarming = 0;

	//default vaules for testing
	globalArgs.filename = "polarbear.jpg";
	globalArgs.hostname = "128.111.68.216";
	globalArgs.hostname = "location.dnsdynamic.com";
	globalArgs.swarming = 1;

 	memset(thread_data,0,sizeof(thread_data));

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
				globalArgs.logging = 1;
				break;
			case 'w':
				globalArgs.swarming = 1;
				globalArgs.swarmfile = optarg;
				break;
		}
		
		opt = getopt_long(argc, argv, optString,longOptions,&longIndex);

	}
	
	globalArgs.swarming = 1;
	if(globalArgs.swarming){
		int rc;
		thread_data[0].tid = 1;
	    thread_data[0].portnr = 21;
	    if(i = settings_from_file("swarm.test", &thread_data[0])){
	    	printf("Invalid input from swarmfile\n");
	    }
			print_ftpArgs(&thread_data[0]);
	    
	    for (i = 0; i < nthreads; ++i){
			//download_with_ftp(&thread_data[i]);
			rc = pthread_create(&threads[i], NULL, download_with_ftp, &thread_data[i]);
			if (rc){
				printf("ERROR; return code from pthread_create() is %d\n", rc);
				exit(-1);
			}
		}

	}else{
		//	fill_thread_data(i);

		thread_data[0].portnr = globalArgs.portnr;
		thread_data[0].tid = 0;
		thread_data[0].filename = globalArgs.filename;
		thread_data[0].hostname = globalArgs.hostname;
		thread_data[0].username = globalArgs.username;
		thread_data[0].password = globalArgs.password;
		download_with_ftp(&thread_data[0]);
	}
	
	
    

   // printf("User: %X %X Pass %s File: %s \n", * &thread_data[0].username[0], * &thread_data[0].username[1],thread_data[0].password,thread_data[0].filename);
   	//fill_thread_data(0);
    //printf("User %x, %d\n", &thread_data 	[0].username, sizeof(thread_data[0].username));
	//	thread_data[0].tid = 0;
	//	skriv_til_struct(&thread_data[0]);
	//	print_globalArgs(&globalArgs);
/*
	if(globalArgs.swarming){
		char ipandport[7*15];
    	char file_regexp[129];
    	char line[100];
		int regexp;
		char temp[15];
		char *hostname;
		FILE *swarmfile;
		
		memset(ipandport,0,sizeof(ipandport));
		strcpy(file_regexp,"ftp\\:\\/\\/([[:alnum:]]+)\\:([[:alnum:]]+)@([[:digit:]]+)\\.([[:digit:]]+)\\.([[:digit:]]+)\\.([[:digit:]]+)(\\/[[:print:]]+)");


		swarmfile = fopen(globalArgs.filename, "rt");
	    while(fgets(line, 100, swarmfile) != 0){
    		if(strlen(line)>8){
				regexp = match_with_regexp(file_regexp,line,15,ipandport);
				if(regexp != 0){
				//invalid input
					return -1;
				}
				printf("%s\n", &ipandport[2*15]);
				sprintf(temp, "%s.%s.%s.%s",&ipandport[2*15],&ipandport[3*15],&ipandport[4*15],&ipandport[5*15]);
				printf("%s\n", temp);
				//strcpy(hostname,"temp");
				thread_data[0].hostname = "hostnam";

    		}
    	}

		print_ftpArgs(&thread_data[0]);

	}	
*/
}
