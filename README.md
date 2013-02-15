by Arne Dahl Bjune
arne@dahlbjune.no

Swarming ftp client for CS176B

	Usage: mftp [OPTIONS] 
	Swarming ftp client\n

	  -a, --active 		    	forces active mode (default passive)
	  -f, --file [filename]		specifies the filename to download
	  -h, --help        		display this help and exit
	  -l, --logfile [filename] 	logs communication with the server to file, - logs to stdout
	  -m, --mode [mode]   		set mode to ASCII or binary (default binary)
	  -n, --username [username]	specifies the username to use (default anonymous)
	  -p, --port [portnr]		specifies the portnr to use (default 21)
	  -P, --password [password]	specifies the password to use
	  -s, --server [hostname]	specifies the server to use
	  -v, --version	    		output version information and exit
	  -w, --swarm [config file]	enables swarming mode
	
	Exit codes:
	  0 	Completed Successfully
	  1 	Invalid command line arguments
	  2 	Invalid hostname
	  3 	Authentification failed
	  4 	Could not connect to server
	  5 	File not found
	  6 	Invalid swarm file
	  7 	Error with pthreads
	  8 	Error with FTP Server
	  10	Generic Error


In swarming mode the saved filename will have the same name as on the first server inn the list.
Username in swarm file can only contain letters and numbers
Password can contain all printable letters

Format for swarming file:
ftp://username:password@host/filename
ftp://host/filename

If no username and password is specified defaults anonymous and user@localhost.localnet are used