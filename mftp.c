/*
CS176B HW2 Swarming ftp client
by Arne Dahl Bjune
arne@dahlbjune.no
*/

#include "mftp.h"

int nthreads = 1;
struct ftpArgs_t *thread_data;
FILE *savedfile;
FILE *printlocation;

pthread_mutex_t filelock;
pthread_mutex_t loglock;


void display_version(void){
	printf("mftp 0.1\n");
	printf("Writen by Arne Dahl Bjune, arne@dahlbjune.no\n");
}

void display_help(void){
	fprintf(printlocation,"Usage: mftp [OPTIONS] \n");
	fprintf(printlocation,"Swarming ftp client\n\n");

	//Skriv inn fullstendig hjelp text
	fprintf(printlocation,"  -a, --active 		    	forces active mode (default passive)\n");
	fprintf(printlocation,"  -f, --file [filename]		specifies the filename to download\n");
	fprintf(printlocation,"  -h, --help        		display this help and exit\n");
	fprintf(printlocation,"  -l, --logfile [filename] 	logs communication with the server to file, - logs to stdout\n");
	fprintf(printlocation,"  -m, --mode [mode]   		set mode to ASCII or binary (default binary)\n");
	fprintf(printlocation,"  -n, --username [username]	specifies the username to use (default anonymous)\n");
	fprintf(printlocation,"  -p, --port [portnr]		specifies the portnr to use (default 21)\n");
	fprintf(printlocation,"  -P, --password [password]	specifies the password to use\n");
	fprintf(printlocation,"  -s, --server [hostname]	specifies the server to use\n");
	fprintf(printlocation,"  -v, --version	    		output version information and exit\n");
	fprintf(printlocation,"  -w, --swarm [config file]	enables swarming mode\n");
	
	fprintf(printlocation,"\nExit codes:\n");
	fprintf(printlocation,"  0 	Completed Successfully\n");
	fprintf(printlocation,"  1 	Invalid command line arguments\n");
	fprintf(printlocation,"  2 	Invalid hostname\n");
	fprintf(printlocation,"  3 	Authentification failed\n");
	fprintf(printlocation,"  4 	Could not connect to server\n");



}

void print_globalArgs(void *structure){

	struct globalArgs_t *gArgs;
	gArgs = (struct globalArgs_t *)structure;

	printf("\n-------------- DEBUG - globalArgs -----------------\n");
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
		fprintf(stderr,"Could not find hostname %s\n",hostname);
		exit(2);
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
		fprintf(stderr,"Could not find hostname %s\n",hostname);
        exit(2);
    }

	if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
		fprintf(stderr,"Could not connect to server n");
		exit(4);
    } 


	return sockfd;
}

int authenticate(int comm_socket,char *username, char *password,int tid){
	
	char recvBuffer[1024];
	char sendBuffer[1024];
	int n;

    n = read(comm_socket, recvBuffer, sizeof(recvBuffer)-1);
    recvBuffer[n] = 0;
    if(globalArgs.logging==1) logToFileWithTid(recvBuffer,0,tid);
    
    if(strncmp(recvBuffer,"220",3)==0){

		sprintf(sendBuffer,"USER %s\r\n",username);
		sendAndRecieve(comm_socket, tid, sendBuffer, recvBuffer);
    	

    }else{
    	fprintf(stderr, "Authentification Failed\n");
    	exit(3);
    }
    
    
    if(strncmp(recvBuffer,"331",3)==0){
    	sprintf(sendBuffer,"PASS %s\r\n",password);
		sendAndRecieve(comm_socket, tid, sendBuffer, recvBuffer);

    }else{
    	fprintf(stderr, "Authentification Failed\n");
    	exit(3);
    }
    
        
    if(strncmp(recvBuffer,"230",3)==0){
    	
    	return 0;

    }else{
    	fprintf(stderr, "Authentification Failed\n");
    	exit(3);
    }
    
    
    return 0;
}

int match_with_regexp (char * expression, char * text, int size, char results[]){
    regex_t r;
    int i;
    regmatch_t m[10];

    regcomp (&r, expression, REG_EXTENDED|REG_NEWLINE);
    
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

int get_number_of_swarming_servers(char *filename){
   	FILE *swarmfile;
   	char line[100];
   	int i = 0;

   	swarmfile = fopen(filename, "rt");
	while(fgets(line, 100, swarmfile) != 0){
    	if(strlen(line)>8){
    		i++;
    	}
    }
  	fclose(swarmfile);
	return i;
}

int settings_from_file (char *filename, void *ftpArgsP, int n){
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
	    	if(n == i){

			regexp = match_with_regexp(file_regexp,line,15,ipandport);
			if(regexp != 0){
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
			ftpArgs->portnr = globalArgs.portnr;
    
    		}
    		i++;
    	}
    }
  	fclose(swarmfile);
	return 0;
}

void set_type_binary(int socket,int tid){
	char recvBuffer[1024];
	char sendBuffer[1024];
	int n;

	strcpy(sendBuffer, "TYPE I\r\n");
	sendAndRecieve(socket, tid, sendBuffer, recvBuffer);
}

void set_type_ascii(int socket,int tid){
	char recvBuffer[1024];
	char sendBuffer[1024];
	int n;

	strcpy(sendBuffer, "TYPE A\r\n");
	sendAndRecieve(socket, tid, sendBuffer, recvBuffer);
}

int set_mode_passive(int socket, int tid){
	char recvBuffer[1024];
	char sendBuffer[1024];
	int n, dataport;
    char pasv_regexp[125];
	char ipandport[6*4];
	char portval1[4];
	char portval2[4];

	strcpy(pasv_regexp,"[[:digit:]]+[^[:digit:]]+\\(([[:digit:]]+)\\,([[:digit:]]+)\\,([[:digit:]]+)\\,([[:digit:]]+)\\,([[:digit:]]+)\\,([[:digit:]]+)\\)\\.");

	strcpy(sendBuffer, "PASV\r\n");
	sendAndRecieve(socket, tid, sendBuffer, recvBuffer);

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

	return 0;
}

int set_mode_active(int comm_socket, int tid){
	int data_socket, connection_socket, n;
	struct sockaddr_in client_addr;
	struct sockaddr_in temp;
	char hostname[50];
	
	char	recvBuffer[1024];

	memset(&client_addr,'0',sizeof(client_addr));
	client_addr.sin_family = AF_INET;
	client_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	client_addr.sin_port = htons(0);
	socklen_t adressLenth = sizeof(client_addr);

	if((data_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("\n Error : Could not create socket \n");
        exit(-1);
    }

    bind(data_socket,(struct sockaddr*)&client_addr,sizeof(client_addr));
    
    getsockname(data_socket, (struct sockaddr*)&temp,&adressLenth);
	gethostname(hostname,sizeof(hostname));
	hostname_translation(hostname);
	build_port_message((int) ntohs(temp.sin_port),hostname);

	sendAndRecieve(comm_socket, tid, hostname, recvBuffer);


    listen(data_socket,10);

	return data_socket;
}

int retrive_and_get_filesize_from_server(int socket, char *filename, int tid){
	char recvBuffer[1024];
	char sendBuffer[1024];
	int n;
	char byte_regexp[25]; 
	char numberOfBytesToRecieve[100];

	strcpy(byte_regexp, "\\(([[:digit:]]+)\\ bytes\\)");

	strcpy(sendBuffer, "RETR ");
	strcat(sendBuffer, filename);
	strcat(sendBuffer, "\r\n");
	sendAndRecieve(socket, tid, sendBuffer, recvBuffer);

    match_with_regexp(byte_regexp,recvBuffer,100,numberOfBytesToRecieve);

    return atoi(numberOfBytesToRecieve);
}

int retrive_part_n_from_server(int socket, char *filename, int tid){
	char recvBuffer[1024];
	char sendBuffer[1024];
	int n;
	char byte_regexp[25];
	char filesize[20];
	char numberOfBytesToRecieve[100];
	int startposition;
	int totalsize;

	strcpy(byte_regexp, "\\(([[:digit:]]+)\\ bytes\\)");

	sprintf(sendBuffer, "SIZE %s\r\n",filename);
	sendAndRecieve(socket, tid, sendBuffer, recvBuffer);
    match_with_regexp("213\\ ([[:digit:]]+)",recvBuffer,20,filesize);

	startposition = tid * (atoi(filesize) /nthreads); 
	sprintf(sendBuffer,"REST %d\r\n",startposition);
	sendAndRecieve(socket, tid, sendBuffer, recvBuffer);

	sprintf(sendBuffer,"RETR %s\r\n",filename);
	sendAndRecieve(socket, tid, sendBuffer, recvBuffer);
    match_with_regexp(byte_regexp,recvBuffer,100,numberOfBytesToRecieve);

    return atoi(filesize);
}

int save_file_from_server_binary(int socket, int numberOfBytes, char *filename, int tid){
	int bytesLeft, *bytesWriten;
	unsigned char recvBuffer[1024];
	char sendBuffer[1024];
	int n;
	FILE *file;
	bytesWriten = malloc(sizeof(int)*nthreads);
	bytesWriten[tid] = 0;

	if(tid == nthreads -1){
		if(numberOfBytes == (numberOfBytes / nthreads) * nthreads){
			bytesLeft = numberOfBytes / nthreads;
		}else{
			bytesLeft = numberOfBytes % (numberOfBytes / nthreads);
		}
	}else{
		bytesLeft = numberOfBytes / nthreads;

	}

	sleep(0.1);

	while(bytesLeft > 0){

		if(bytesLeft > sizeof(recvBuffer) -1){
			n = read(socket, recvBuffer, sizeof(recvBuffer)-1);
		}else{
			n = read(socket, recvBuffer, bytesLeft);
		}

		pthread_mutex_lock(&filelock);

		fseek(savedfile, (numberOfBytes/nthreads)*tid + bytesWriten[tid], SEEK_SET);
	    fwrite(recvBuffer, sizeof(recvBuffer[0]), n, savedfile);

		pthread_mutex_unlock(&filelock);
	   
	    bytesLeft = bytesLeft - n;
	    bytesWriten[tid] =  bytesWriten[tid] + n;
	}

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
}

int logToFileWithTid(char logtext[], int send, int tid){
	char temp[1024 + 14];

	if(globalArgs.swarming == 1){
		if(send == 1){
			sprintf(temp,"Tid: %i C->S: %s",tid, logtext);
		}else{
			sprintf(temp,"Tid: %i S->C: %s",tid, logtext);
		}
	}else{
		if(send == 1){
			sprintf(temp,"C->S: %s", logtext);
		}else{
			sprintf(temp,"S->C: %s", logtext);
		}

	}
	if(!strcmp(globalArgs.logfile,"-")){
		printf("%s", &temp[0]);
	}else{
		FILE *file;	
		file = fopen(globalArgs.logfile,"a+");
		pthread_mutex_lock(&loglock);
		fprintf(file, "%s", temp);
		pthread_mutex_unlock(&loglock);

		fclose(file);
	}
	return 0;
}

int close_connection(int socket, int tid){
	char recvBuffer[1024];
	char sendBuffer[1024];
	int n;


	n = read(socket, recvBuffer, sizeof(recvBuffer)-1);
    recvBuffer[n] = 0;
 	if(globalArgs.logging==1) logToFileWithTid(recvBuffer,0,tid);


	strcpy(sendBuffer, "QUIT\r\n");
	sendAndRecieve(socket, tid, sendBuffer, recvBuffer);
	
}

int sendAndRecieve(int socket, int tid, char *sendBuffer, char *recvBuffer){
	int n;
	if(globalArgs.logging==1) logToFileWithTid(sendBuffer,1,tid);
	write(socket, sendBuffer, strlen(sendBuffer));
	
	n = read(socket, recvBuffer, 1024-1);
    recvBuffer[n] = 0;
 	if(globalArgs.logging==1) logToFileWithTid(recvBuffer,0,tid);
}

void *download_with_ftp(void *settings){
	char recvBuffer[1024];
	char sendBuffer[1024];
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
    if(authenticated = authenticate(comm_socket, ftpArgs->username, ftpArgs->password,ftpArgs->tid)){
    	printf("Authentification Failed \n");
    }
    

    if(globalArgs.active==0){
	    int dataport,size,data_socket;
	 
    	dataport = set_mode_passive(comm_socket,ftpArgs->tid);
	    
		if (globalArgs.mode == "binary"){
			set_type_binary(comm_socket,ftpArgs->tid);
		}else{
			set_type_ascii(comm_socket,ftpArgs->tid);
		}

		data_socket = connect_to_server(dataport,ftpArgs->hostname);
		
		if(ftpArgs->tid==0){
			size = retrive_and_get_filesize_from_server(comm_socket, ftpArgs->filename,ftpArgs->tid);
		}else{
			size = retrive_part_n_from_server(comm_socket, ftpArgs->filename, ftpArgs->tid);
		}
		
		if (globalArgs.mode == "binary"){
		 	save_file_from_server_binary(data_socket,size, ftpArgs->filename, ftpArgs->tid);
		}else{
		 	save_file_from_server_ascii(data_socket,size, ftpArgs->filename);
		}

		close_connection(comm_socket, ftpArgs->tid);


    }else{
    	int size;
    	int data_socket = set_mode_active(comm_socket,ftpArgs->tid);
    	int connection_socket;
		if (globalArgs.mode == "binary"){
			set_type_binary(comm_socket,ftpArgs->tid);
		}else{
			set_type_ascii(comm_socket,ftpArgs->tid);
		}
		
		if(ftpArgs->tid==0){
			size = retrive_and_get_filesize_from_server(comm_socket, ftpArgs->filename,ftpArgs->tid);
		}else{
			size = retrive_part_n_from_server(comm_socket, ftpArgs->filename,ftpArgs->tid);

		}
		connection_socket = accept(data_socket,(struct sockaddr*)NULL, NULL);

		if (globalArgs.mode == "binary"){
		 	save_file_from_server_binary(connection_socket,size, ftpArgs->filename, ftpArgs->tid);
		}else{
		 	save_file_from_server_ascii(connection_socket,size, ftpArgs->filename);
		}
		close_connection(comm_socket,ftpArgs->tid);
    }

    return 0;
}

void fill_thread_data(){
	thread_data[0].portnr = globalArgs.portnr;
	thread_data[0].tid = 0;
	thread_data[0].filename = globalArgs.filename;
	thread_data[0].hostname = globalArgs.hostname;
	thread_data[0].username = globalArgs.username;
	thread_data[0].password = globalArgs.password;
}

int main(int argc, char *argv[]) {
	int i;
	int opt = 0;
	int longIndex;
	char *temp;
	char *message;
	char *temptest;
//	pthread_t threads[2];
	pthread_t *threads;
 
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
	//globalArgs.hostname = "128.111.68.216";
	//globalArgs.hostname = "location.dnsdynamic.com";
	
	opt = getopt_long(argc, argv, optString,longOptions,&longIndex);
	while( opt != -1){
		switch( opt){
			case 'v':
				display_version();
				return 0;
			case 'h':
				printlocation = stdout;
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
				if(!strcmp(optarg,"binary")){
					globalArgs.mode = optarg;
				}else if(!strcmp(optarg,"BINARY")){
					globalArgs.mode = "binary";
				}else if(!strcmp(optarg,"ASCII")){
					globalArgs.mode = optarg;
				}else if(!strcmp(optarg,"ascii")){
					globalArgs.mode = "ASCII";
				}
				else{
					fprintf(stderr, "Invalid mode, choose ASCII or binary\n");
					exit(1);
				}
				break;
			case 'l':
				globalArgs.logfile = optarg;
				globalArgs.logging = 1;
				break;
			case 'w':
				globalArgs.swarming = 1;
				globalArgs.swarmfile = optarg;
				break;
			case 'd':
				printf("DEBUG MODE ACTIVATED\n");
				globalArgs.swarming = 1;
				globalArgs.swarmfile = "swarm.test";
				globalArgs.filename = "polarbear.jpg";
				globalArgs.hostname = "128.111.68.216";
				break;
			case '?':
				printlocation = stderr;
				display_help();
				exit(1);
		}
		
		opt = getopt_long(argc, argv, optString,longOptions,&longIndex);

	}
	if(globalArgs.filename == NULL && !globalArgs.swarming){
		fprintf(stderr,"Invalid Commandline options,\nat minimum filename and server has to be specified\n");
		
		exit(1);
	}else if(globalArgs.hostname == NULL && !globalArgs.swarming){
		fprintf(stderr,"Invalid Commandline options,\nat minimum filename and server has to be specified\n");
		exit(1);
	}else if(globalArgs.swarming && !strcmp(globalArgs.mode,"ASCII")){
		fprintf(stderr, "Swarming only available in binary mode\n");
		exit(100); // endre
	}
	
	//Handeling swarming
	if(globalArgs.swarming){
		int rc;
		nthreads = get_number_of_swarming_servers(globalArgs.swarmfile);
	    
	    thread_data = malloc(sizeof(struct ftpArgs_t) * nthreads);
	    threads = malloc(sizeof(pthread_t) * nthreads);

	    for(i = 0; i < nthreads; i++){
	    	thread_data[i].tid = i;
	    	if(rc = settings_from_file("swarm.test", &thread_data[i], i)){
	    		printf("Invalid input from swarmfile\n");
	    	}
	    }
		savedfile = fopen(thread_data[0].filename,"w");

	    for (i = 0; i < nthreads; ++i){
			rc = pthread_create(&threads[i], NULL, download_with_ftp, &thread_data[i]);
			if (rc){
				fprintf(stderr,"Error, could not create threads\n");
				exit(-1);
			}
		}
	//Non swarming download
	}else{
	    thread_data = malloc(sizeof(struct ftpArgs_t) * nthreads);

		fill_thread_data();
		savedfile = fopen(thread_data[0].filename,"w");
		
		download_with_ftp(&thread_data[0]);
	}
	
	pthread_exit(NULL);
    fclose(savedfile);
    free(threads);
    free(thread_data);
    return 0;
	    
}
