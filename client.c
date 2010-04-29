#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/in.h>
#include <strings.h>
#include "utils.h"
#include <arpa/inet.h>

#define CLIENT_DEBUG 	1
#define MAX_RECEIVABLE	80000
#define MAX_URL_LENGTH	80

int main(int argc, char * argv[])
{
	int server_socket;  	  	//  conceptually a socket descriptor
	int returncode, addrlen;	//  utility variables
	int dest_ip_int, dest_port;
	int i, messagelength, bytes_sent;

	struct sockaddr_in * server_addr;
	char buffer[MAX_RECEIVABLE + 1];
	char url_input[MAX_URL_LENGTH+1];
	char *message;
	url connect_to;

	printf("Hi, welcome to Steve Matuszek's HTTP client for COMP 243.\n");
	printf("If you get an unexplained seg fault, it's this memory leak\n");
	printf("I wasn't able to track down... sorry!\n\n");

  //  Negative first, complain about usage 

	if (!buffer)
	{
		printf("Ran out of memory trying to allocate buffer.\n");
		exit(-1);
	}

	if (argc < 2)
	{
		// printf("Usage: Client url\n");
		// exit(1);

STARTOFF:

		printf("\n----------------------------------------\n");
		printf("Please enter a URL. Leave blank to quit.\n> ");

		url_input[0] = getchar();
		if (url_input[0] == '\n')
		{
			printf("Thanks for playing.\n");
			exit(0);
		}	
			
		i = 1;
		while (i < MAX_URL_LENGTH)
		{
			url_input[i] = getchar();
			// printf("url_input[%2d] is %c\n", i, url_input[i]);
			if (url_input[i] == '\n')
			{
				url_input[i] = '\0';
				break;
			}
			i++;
		}
		// scanf("%s", &url_input);
		fflush(stdin);
		
		printf("\nLooking for %s\n", url_input);
	}
	else
	{
		strcpy(url_input, argv[1]);
	}
	
  //  Zeroth, get the args  

	connect_to = (url) malloc (sizeof(url_struct));
	initialize_url(connect_to);
	// returncode = parse_as_url(argv[1], connect_to);
	returncode = parse_as_url(url_input, connect_to);

	if (returncode != 0)
	{
		printf("I'm sorry, I couldn't parse that URL.\n\n");
		goto STARTOFF;
		exit(1);
	}
	if (CLIENT_DEBUG) 
		pretty_print(connect_to);
	
	// dest_ip_int = inet_addr(connect_to -> hostname);

	dest_ip_int = get_ip_int(connect_to);
	
	if (dest_ip_int == -1)
	{
		printf("Sorry... please try another.\n");
		goto STARTOFF;
	}
	
	dest_port = connect_to -> port;

  //  First, create a socket

	if (CLIENT_DEBUG) printf("Client starting up.\n");

	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket < 0)
	{
		printf("Client: error %d creating socket\n", errno);
		perror("      ");
		exit(1);
	}

	if (CLIENT_DEBUG)
		printf("Client: server_socket's file descriptor is %d\n", 
			server_socket);

  //  Set up server_addr to be the name of the server socket 

	//  Allocate space for server_addr, which is now a pointer
	//  to a sockaddr_in struct, not the struct itself -- 03/2002

	server_addr = (struct sockaddr_in *) malloc(sizeof(struct sockaddr_in));
	memset(server_addr, 0, sizeof(server_addr));
	server_addr->sin_family = AF_INET;
	server_addr->sin_port = htons(dest_port);
	server_addr->sin_addr.s_addr = dest_ip_int;

	if (CLIENT_DEBUG)
	{
		printf("Trying to connect to port %d on host %s\n",
		       ntohs(server_addr->sin_port),
		       inet_ntoa(server_addr->sin_addr));
	}

  //  Connect to the server  

	addrlen = sizeof(struct sockaddr);
	
	returncode = connect(server_socket, 
			     (struct sockaddr *)server_addr, 
		addrlen);
	if (returncode < 0)
	{
		printf("Client: error %d connecting\n", errno);
		perror("      ");
		goto STARTOFF;
		exit(1);
	}
	
   //  If we get this far we can assume we've connected. 

	if (CLIENT_DEBUG)
	{
		printf("Connected successfully to port %d on host %s\n",
		       ntohs(server_addr->sin_port),
		       inet_ntoa(server_addr->sin_addr));
	}

  //  So let's send a message. 

	// message = "Excuse me, what time is it?";
	allocate_and_fill_http_request(&message, connect_to);
	messagelength = strlen(message);
	
	bytes_sent = send(server_socket, message, messagelength, 0);

  //  Check whether the message was sent okay. 
	
	if (bytes_sent < 0)
	{
		printf("Client: error %d sending message\n", errno);
		perror("      ");
		exit(1);
	}

  //  Print out as much of the message as was sent. 

	if (CLIENT_DEBUG)
	{
		printf("The following message was successfully sent to the server:\n");
		for (i = 0; i<bytes_sent; i++)
			printf("%c", message[i]);
		printf("\n");
	}

	

	int first_iteration = 1;
	int content_offset = 0;
	int content_length = 0;
	int content_remaining = 1;
	int ignore_content_remaining = 0;

	do
	{
	  memset(buffer, 0, 80000);
		returncode = recv(server_socket, buffer, MAX_RECEIVABLE, 0);
		if (returncode < 0)
		{
			printf("Client: error %d receiving message\n", errno);
			perror("      ");
			exit(-1);
		}
	
		buffer[returncode] = '\0';
		printf("reponse length: %d\nheader: %d\nactual: %ld\n", returncode, content_offset_of(buffer), strlen(buffer));
		printf("---------------------------------------------------\n");
		printf("%s\n", buffer);
		printf("---------------------------------------------------\n");

		if (first_iteration)
		{
		  
			first_iteration = 0;

			content_offset = content_offset_of(buffer);
			int rc = parse_response_header(buffer, content_offset-1, &content_length);
			content_remaining = content_length;
			if (content_remaining == IGNORE_THIS_VALUE)
				ignore_content_remaining = 1;
			else
				ignore_content_remaining = 0;
			
			if (rc > 400)
			{
				printf("Quitting Client.\n");
			  
				goto STARTOFF;
			
				exit(1);
			}

			
			if (!ignore_content_remaining) {
				content_remaining -= strlen(buffer+content_offset);
			}
		}
		else 
		  {
			if (!ignore_content_remaining)
				content_remaining -= strlen(buffer);
		}
		
		if (CLIENT_DEBUG)
		{
			printf("Content remaining: %d bytes\n", content_remaining);
		}
	} while ((content_remaining > 0) || 
		     (ignore_content_remaining && returncode != 0));

	if (CLIENT_DEBUG && ignore_content_remaining)
		printf("Received %d bytes in last packet\n", returncode);
		
	// printf("---------------------------------------------------\n");			
	printf("\n");

	goto STARTOFF;
	
	exit(0);
}

