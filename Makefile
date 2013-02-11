mftp: mftp.c mftp.h
	gcc -o mftp mftp.c -pthread

clean: 
	rm *.o
	rm mftp


