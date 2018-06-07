CC = gcc
CFLAGS = -O2 -Wall -I .

# This flag includes the Pthreads library on a Linux box.
# Others systems will probably require something different.
LIB = -lpthread

thread: thread.c csapp.o
	$(CC) $(CFLAGS) -o thread thread.c csapp.o $(LIB)

mutex: mutex.c csapp.o
	$(CC) $(CFLAGS) -o mutex mutex.c csapp.o $(LIB)

echoserver_pre: echoserver_pre.c csapp.o
	$(CC) $(CFLAGS) -o echoserver_pre echoserver_pre.c csapp.o $(LIB)

csapp.o: csapp.c
	$(CC) $(CFLAGS) -c csapp.c

clean:
	rm -f *.o tiny *~
	(cd cgi-bin; make clean)

