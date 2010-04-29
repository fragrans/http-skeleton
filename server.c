#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#include "utils.h"

#define SERVER_DEBUG 	0
#define MY_PORT		48080
#define MY_PORT_2	58080
#define QUEUE_LENGTH	10
#define GUPPY_IP        "152.2.128.185"

#define MAX_REQUEST_SIZE	1000
#define MAX_FILENAME_LENGTH	200
#define FILE_CHUNK_SIZE		500

#define WEB_DOC_ROOT	"html"

int handle_connection(int the_socket);
FILE * try_to_open(char * filename);

static int gr;
static int *global_running = &gr;

int main()
{
	int my_socket, new_socket;  // conceptually socket descriptors 
	int returncode, addrlen;    // utility variables 

	struct sockaddr_in my_addr;
	struct sockaddr_in their_addr;
	char buffer[MAX_REQUEST_SIZE + 1];
	
	*global_running = 1;
	
  //  Zeroth, initialize buffer 

	int i;
	
	for(i=0; i<MAX_REQUEST_SIZE; i++) buffer[i] = '-';

  //  First, create an Internet socket 

	printf("Welcome to Steve Matuszek's HTTP server for COMP 243.\n");
	printf("I'm not great at threads in C, so hit control-C to exit.\n\n");
	
	printf("Server starting up.\n");

	my_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (my_socket < 0)
	{
		printf("Server: error %d creating socket\n", errno);
		perror("      ");
		exit(1);
	}

	if (SERVER_DEBUG)
		printf("Server: my_socket's file descriptor is %d\n", my_socket);

  //  Bind it to a port, chosen for us by using 0, or not 

	my_addr.sin_family = AF_INET;			//  fine

	// my_addr.sin_port = 0;				//  pick for me
	my_addr.sin_port = htons(MY_PORT);		//  no, don't

	// my_addr.sin_addr.s_addr = inet_addr(GUPPY_IP);	//  use guppy's 
	my_addr.sin_addr.s_addr = INADDR_ANY;		//  use mine

	bzero(&(my_addr.sin_zero), 8);			//  zero the rest

	addrlen = sizeof(struct sockaddr);

	returncode = bind(my_socket, (struct sockaddr *) &my_addr, addrlen);
	if (returncode < 0)
	{
		printf("Server: error %d binding name\n", errno);
		perror("      ");
		printf("Going to pick a different port.\n");
		
		my_addr.sin_port = htons(MY_PORT_2);		//  Try the other one
		
		returncode = bind(my_socket, (struct sockaddr *) &my_addr, addrlen);
		if (returncode < 0)
		{
			printf("Server: error %d binding name\n", errno);
			perror("      ");
			printf("Giving up.\n");
			exit(1);
		}
		else
		{
			printf("NOTE!: Started instead on port %d\n",
				my_addr.sin_port);
		}
	}
	else
	{
			printf("Started on port %d\n",
				my_addr.sin_port);
	}

  //  Have it listen  

	returncode = listen(my_socket, QUEUE_LENGTH);
	if (returncode < 0)
	{
		printf("Server: error %d listening\n", errno);
		perror("      ");
		exit(1);
	}
	else if (1 || SERVER_DEBUG) 
		printf("Server listening...\n");

  //  Ignore child process termination 

	signal(SIGCHLD, SIG_IGN);

  //  Start a thread to wait for input. Afraid I don't know a better way.
  
	if (fork() == 0)
	{
		while (1)
		{
			// printf("1 loop: *global_running (addr %d) is %d...\n", 
				// global_running, *global_running);
			char c = getchar();
			printf("Char is %c\n", c);
			*global_running = 0;
			break;
		}
		// printf("1 loop: *global_running (addr %d) is %d...\n", 
			// global_running, *global_running);
		exit(0);
	}

  //  Go into the infinite loop waiting for connections.
  
	while(*global_running)
	{
		// printf("connect loop: *global_running (addr %d) is %d...\n", 
			// global_running, *global_running);
	  new_socket = accept(my_socket, (struct sockaddr*)&their_addr, &addrlen);
		if (new_socket < 0)
		{
			printf("Server: error %d accepting\n", errno);
			perror("      ");
			exit(1);
		}
		else if (SERVER_DEBUG)
			printf("Connection accepted...\n");

		//  Here we fork when accept()ing a connect()ion.
		//  Have a pretty good handle on this at this point.

		if (fork() == 0)			//  Then we're the child...
		{
			close(my_socket);
			if (SERVER_DEBUG)
				printf("                   ...Child started\n");

			returncode = handle_connection(new_socket);
			if (returncode < 0)
			{
				printf("Server: error handling connection\n", errno);
				exit(1);
			}
			if (SERVER_DEBUG)
				printf("Done handling connection.\n");
			exit(0);
		}
		else
		{
			// printf("Forking...\n");
		}
	}

  //  Clean up. 

	close(my_socket);
}

int handle_connection(int the_socket)
{
	char buffer[MAX_REQUEST_SIZE+1];
	int returncode, i;
	struct sockaddr_in peer_addr;
	int addrlen;
	char *peer_name;

	char *message;
	int messagelength;
	int bytes_sent;
	int status = 500;

  //  All this stuff does is tell me who the connection's from. 

	addrlen = sizeof(struct sockaddr);
	returncode = getpeername(the_socket, (struct sockaddr*)&peer_addr, &addrlen);
	if (returncode < 0)
	{
		printf("Server: error %d getting peer name\n", errno);
		perror("      ");
	}
	else if (1 || SERVER_DEBUG)
	{
		peer_name = inet_ntoa(peer_addr.sin_addr);
		printf("Connection from %s\n", peer_name);
	}

  //  Now, actually listen for a message from the client. 

	if (SERVER_DEBUG)
		printf("Waiting for connection...\n");
	returncode = recv(the_socket, buffer, MAX_REQUEST_SIZE, 0);
	if (returncode < 0)
	{
		printf("Server: error %d receiving message\n", errno);
		perror("      ");
		return(-1);
	}
	if (SERVER_DEBUG)
	{
		printf("Message received from socket was:\n");

		buffer[returncode] = '\0';
		printf("%s\n", buffer);
	}
	
  //  Okay, so let's parse the request.
  
  	char * filename;
  	
	filename = (char *) malloc ((MAX_FILENAME_LENGTH+1) * sizeof(char));
  
  	returncode = parse_request_header(buffer, 
  		(&filename), MAX_FILENAME_LENGTH);
  		
  	//  If we couldn't parse the request, we've got a 400.	
  		
	if (returncode < 0)
	{
		char *fourhun = "400: Bad Request.\n";
	
		printf("Hmm, we seem to have gotten a bad request.\n");
		status = 400;
		allocate_and_fill_http_response((char **) &message, status, sizeof(fourhun));

		strcat(message, fourhun);

		messagelength = strlen(message);
	
		if (SERVER_DEBUG)
			printf("Response is \n----------\n%s\n------------\n", message);
			
		bytes_sent = send(the_socket, message, messagelength, 0);
	
		if (bytes_sent < 0)   
		{
			printf("Server: error %d sending header\n", errno);
			perror("      ");  
			exit(1);
		}
			
		close(the_socket);
		return(1);
	}

  //  Here's where the code goes to actually open the file, etc.

  	printf("Filename was set: it is %s\n", filename);
  	
  	FILE * file = try_to_open(filename);
  	char * file_chunk = (char *) malloc ((FILE_CHUNK_SIZE+1) * sizeof(char));
	int current, filesize=-1;
	
	if (file != NULL)
	{
		status = 200;
		do
		{
			current = fgetc(file);
			filesize++;
		} while (current != EOF);
		
		printf("file size is %d characters\n", filesize);
		
		rewind(file);
	}
	else 
	{
		status = 404;

		file_chunk = "404: Not found.\n";
		filesize = strlen(file_chunk);
		allocate_and_fill_http_response((char **) &message, status, filesize);
		strcat(message, file_chunk);
		messagelength = strlen(message);
		
		bytes_sent = send(the_socket, message, messagelength, 0);
	
		if (bytes_sent < 0)   
		{
			printf("Server: error %d sending header\n", errno);
			perror("      ");  
			exit(1);
		}
		return(1);
	}
  //  To respond: first, the HTTP header.

	allocate_and_fill_http_response((char **) &message, status, filesize);
	messagelength = strlen(message);

	if (SERVER_DEBUG)
		printf("Response is \n----------\n%s\n------------\n", message);
		
	bytes_sent = send(the_socket, message, messagelength, 0);

	if (bytes_sent < 0)   
	{
		printf("Server: error %d sending header\n", errno);
		perror("      ");  
		exit(1);
	}
	
  //  Next, send the content.

	while(filesize > 0)
	{
		for (i=0; i<FILE_CHUNK_SIZE; i++)
		{
			current = fgetc(file);
	
			if (current == EOF) break;
	
			else file_chunk[i] = (char) current;		
		}
	
		file_chunk[i] = '\0';
	  	
	  	printf("Got a chunk from the file:\n%s\n", file_chunk);
	
		bytes_sent = send(the_socket, file_chunk, strlen(file_chunk), 0);
		
		filesize -= bytes_sent;
		
	 	if (bytes_sent < 0)   
		{
			printf("Server: error %d sending content\n", errno);
			perror("      ");  
			exit(1);
		}
	}
	if (1 || SERVER_DEBUG)
	{
		printf("Request fulfilled.\n");
	}
	
	fclose(file);
	//send(the_socket, message, 1, 0);

	close(the_socket);	
}

FILE * try_to_open(char * filename)
{
	char * fullname;
	char * cwd;
	FILE * result;
	
	if ((cwd = getcwd(NULL, 64)) == NULL)
	{
		perror("Trying to open docs, couldn't get working dir");
		exit(2);
	}
	
	int fnl = strlen(filename) + strlen(cwd) + strlen(WEB_DOC_ROOT) + 12;
	
	printf("fnl == %d\n", fnl);

	fullname = (char *) malloc(fnl * sizeof(char));
	
	strcat(fullname, cwd);
	strcat(fullname, "/");
	strcat(fullname, WEB_DOC_ROOT);
	strcat(fullname, filename);
	
	if (fullname[strlen(fullname)-1] == '/')
	{
		strcat(fullname, "index.html");
	}
	
	printf("Full file name is %s\n", fullname);
	
	result = fopen(fullname, "r");
	
	if (result == NULL)
	{
		perror("Opening file");
		printf("Couldn't find the file %s...\n", fullname);
		return NULL;
	}
	else return(result);
	
	return NULL;
}
















