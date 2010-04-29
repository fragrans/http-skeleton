# CFLAGS = -pedantic

CFLAGS = 

# LIBS =        -lXt \
#       -lX11 \
#       -lm 

LIBS = 

CC = gcc

all:: Client Server

#  Dependencies. The executable depends on the .o, the .o on the .c

Client: client.o utils.o
	$(CC) -o Client client.o utils.o $(CFLAGS) $(LIBS)

client.o: client.c

	$(CC) -c client.c $(CFLAGS)

Server: server.o utils.o
	$(CC) -o Server server.o utils.o $(CFLAGS) $(LIBS)

server.o: server.c
	$(CC) -c server.c $(CFLAGS)

utils.o: utils.c
	$(CC) -c utils.c $(CFLAGS)

