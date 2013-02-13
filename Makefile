mftp: mftp.c mftp.h
	gcc -o mftp mftp.c -lpthread

clean: 
	rm *.o
	rm mftp


