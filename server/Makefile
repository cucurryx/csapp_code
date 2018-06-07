CC = gcc
CFLAGS = -O2 -Wall -I .

# This flag includes the Pthreads library on a Linux box.
# Others systems will probably require something different.
LIB = -lpthread

echo_server_p: process_conecho_server.c csapp.o
	$(CC) $(CFLAGS) -o echo_server process_conecho_server.c csapp.o $(LIB)

echo_server_mc: multi_conecho_server.c csapp.o
	$(CC) $(CFLAGS) -o echo_server multi_conecho_server.c csapp.o $(LIB)

echo_server_m: multi_echo_server.c csapp.o
	$(CC) $(CFLAGS) -o echo_server multi_echo_server.c csapp.o $(LIB)

csapp.o: csapp.c
	$(CC) $(CFLAGS) -c csapp.c

clean:
	rm -f *.o tiny *~
	(cd cgi-bin; make clean)

