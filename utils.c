//  utils.c
//  
//  contains a bunch of stuff I need.
//
//  Written in C but compiled with gcc, so I can use // comments. 

#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>

#include "utils.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_PATH_LENGTH 	120
#define MAX_HOSTNAME_LENGTH 	60

#define UTILS_DEBUG 	0

//  This is a dirty way of doing some enums with an array

#define NUMBER_OF_PROTOCOLS	5
char * PROTOCOL_NAMES[] = {"http", "ftp", "gopher", "mailto", "telnet"};

int initialize_url(url u)
{
	if (!u)		//  url is null
	{
		printf("initialize_url: passed NULL\n");
		return -1;
	}

	//  Put in a bunch of defaults

	u -> protocol = HTTP;

	u -> hostname = (char *) malloc((MAX_HOSTNAME_LENGTH+1) * sizeof(char));
	strcpy(u -> hostname, "guppy.cs.unc.edu");

	u -> port = 80;

	u -> path = (char *) malloc((MAX_PATH_LENGTH+1) * sizeof(char));
	strcpy(u -> path, "/index.html");

	return 0;
}

void pretty_print(url u)
{
	printf("Protocol is '%s'\n", PROTOCOL_NAMES [u -> protocol]);

	printf("hostname is '%s'\n", u -> hostname);
	printf("    port is  %d\n",   u -> port);
	printf("    path is '%s'\n", u -> path);

	if (u -> port >= 0)
	{
		printf("So the entire URL is ");
		printf("%s://%s:%d%s\n",
			PROTOCOL_NAMES [u -> protocol],
			u -> hostname,
			u -> port,
			u -> path);
	}
	else
	{
		printf("So the entire URL is ");
		printf("%s://%s%s\n",
			PROTOCOL_NAMES [u -> protocol],
			u -> hostname,
			u -> path);
	}
	printf("\n");
}

int parse_as_url(char * in, url u)
{
	char * result;
	char port_as_string[10];
	int temp;

	//  Get the first token, try to treat it as a protocol. 

	result = strtok(in, ":/");
	if (UTILS_DEBUG)
	printf("token is %s -- this should be the protocol\n", result);

	u -> protocol = parse_as_protocol(result);
	if (u -> protocol < 0)
	{
		printf("I'm sorry, I couldn't parse ""%s"" as a protocol.\n", result);
		return (-1);
	}
	else if (u -> protocol != HTTP)
	{
		printf("I'm sorry, only HTTP is supported in this client.\n");
		return(-1);
	}
	if (UTILS_DEBUG)
	printf("u -> protocol is %d\n", u -> protocol);

	//  Get the second token. See whether it has a : in it.
	//  If so, treat it as a hostname and port;
	//  If not, treat it as just a hostname.

	if (u -> hostname != NULL) 
		free(u -> hostname);

	result = strtok(NULL, "/");

	if (UTILS_DEBUG)
	printf("result is %s\n", result);
	
	if (result == NULL)
	{
		printf("Oops, ran out of URL to parse.\n");
		return(-1);
	}

	//  I'll look for a : using strcspn, because I am not sure what using
	//  strtok or strstr would do to the secret magic index variable. C 
	//  string functions are weird that way.

	int charstilcolon = strcspn(result, ":");
	int resultlength = strlen(result);

	if (UTILS_DEBUG)
	printf("token is %s -- this should include the port if present\n", result);

	if (charstilcolon == resultlength)	//  Just the hostname.
	{
		//  Copy the hostname into the struct, and set port to default.
		
		if (UTILS_DEBUG)
		printf("rest of string is %s -- port not present\n", result);

		u -> hostname = (char *) malloc((resultlength+3) * sizeof(char));
		strcpy(u -> hostname, result);

		// u -> hostname = strdup(result);

		if (UTILS_DEBUG)
		{
			printf("--------------\n");
			pretty_print(u);
			printf("--------------\n");
		}

		set_default_port(u);

		if (UTILS_DEBUG)
		printf("port number is %d (default)\n", u -> port);
	}
	else
	{
		//  First put in the hostname... that's chars 0 through temp-1,
		//  and slap a null after. 

		if (UTILS_DEBUG)
		{
			printf("rest of string is %s -- port is present\n", result);
			printf("number of chars til colon is %d\n", charstilcolon);
		}
	
		u -> hostname = (char *) malloc( (charstilcolon+1) * sizeof(char));
		strncpy(u -> hostname, result, charstilcolon);
		(u -> hostname)[charstilcolon] = '\0';

		if (UTILS_DEBUG)
		printf("Allocated hostname ok, it is %s\n", u -> hostname);

		strcpy(port_as_string, result+charstilcolon+1);
		u -> port = atoi(port_as_string);

		if (UTILS_DEBUG)
		printf("port number is %d\n", u -> port);
	}


	//  And everything else should be the path.

	if (UTILS_DEBUG) printf("1\n");

	if (u -> path != NULL) 
	{
		if (UTILS_DEBUG) printf("2\n");
		// free(u -> path);
	}
	if (UTILS_DEBUG) printf("3\n");

	result = strtok(NULL, "");
	if (UTILS_DEBUG) printf("4\n");

	if (result == NULL || strlen(u -> path) == 0)
	{
		if (UTILS_DEBUG) printf("5\n");
		u -> path = (char *) malloc (4 * sizeof(char));
		u -> path = "/";
	}
	else
	{
		if (UTILS_DEBUG) printf("6\n");
		char * foobar = strdup("/");
		strcat(foobar, result);

		int fbl = strlen(foobar);

		if (UTILS_DEBUG) printf("foobar = %s, length is %d\n", foobar, fbl);
		
		u -> path = (char *) malloc ((fbl+3) * sizeof(char));

		if (UTILS_DEBUG) printf("Allocated u -> path ok...\n");

		strcpy(u -> path, foobar);
	}
	if (UTILS_DEBUG) printf("7\n");

	return 0;
}

int parse_as_protocol(char * in)
{
	int i;

	for (i = 0; i<NUMBER_OF_PROTOCOLS; i++)
	{
		if (strcasecmp(in, PROTOCOL_NAMES[i]) == 0)
		{
			return i;
		}
	}
	return -1;
}

int set_default_port(url u)
{
	if (u -> protocol == HTTP)
	{
		u -> port = 80;
	}
	else if (u -> protocol == TELNET)
	{
		u -> port = 23;
	}
	else
		return (u -> port = -1);
}

int get_ip_int(url u)
{
	struct hostent * h;
	int i = 0;
	int result;
	char *temp;

	h = (struct hostent *) malloc (sizeof (struct hostent));

	if (UTILS_DEBUG) 
	{
		printf("hostname looking for is %s \n", u -> hostname);
		printf("g1\n");
		pretty_print(u);
	}

	h = gethostbyname(u -> hostname);
	if (UTILS_DEBUG) printf("g2\n");

	if (h == NULL)
	{
		printf("Host ""%s"": \n", u -> hostname); 
		herror("Got an error trying to look up the host ");
		return(-1);
	}	

	if (UTILS_DEBUG) printf("Host name is %s \n", h -> h_name);

	temp = (char *) inet_ntoa(*((struct in_addr *) h -> h_addr));

	if (UTILS_DEBUG) printf("IP address is %s\n", temp);

	result = inet_addr(temp);

	if (UTILS_DEBUG) printf("IP as an int is %x\n", result);
	
	return(result);
}

void allocate_and_fill_http_request(char ** addr_of_buffer, url u)
{
	if (UTILS_DEBUG) printf("Entered allocate_and_fill_http_request\n");

	char buffer[1000];
	buffer[0] = '\0';

	//  Method

	strcat(buffer, "GET ");
	if (UTILS_DEBUG) printf("buffer is %s, length is %ld\n", buffer, strlen(buffer));

	//  Request-URI

	strcat(buffer, u -> path);
	if (UTILS_DEBUG) printf("buffer is %s, length is %ld\n", buffer, strlen(buffer));

	//  HTTP-Version

	strcat(buffer, " HTTP/1.0");
	if (UTILS_DEBUG) printf("buffer is %s, length is %ld\n", buffer, strlen(buffer));
	
	//  CRLF  

	strcat(buffer, CRLF);		//  #defined as "\015\012\000"
	if (UTILS_DEBUG) printf("buffer is %s, length is %ld\n", buffer, strlen(buffer));

	//  User-Agent (and another CRLF)

	strcat(buffer, "User-Agent:MatuszekClient/1.0");
	strcat(buffer, CRLF);
	strcat(buffer, CRLF);
	if (UTILS_DEBUG) printf("buffer is %s, length is %ld\n", buffer, strlen(buffer));

   //  Allocate only what we need; return that pointer.

	int length = strlen(buffer);
	char * result;
	
	result = (char *) malloc ((length+1) * sizeof(char));
	strncpy(result, buffer, length);
	result[length] = '\0';

	*addr_of_buffer = result;
}

void allocate_and_fill_http_response(char ** addr_of_buffer, int status, int contentlength)
{
	//	HTTP/1.1 200 OK
	//	Date: Mon, 13 Sep 1999 01:03:58 GMT
	//	Server: Apache/1.3.4 (Unix)
	//	Last-Modified: Thu, 26 Aug 1999 14:32:38 GMT
	//	ETag: "4b1a1504-1865-37c55006"
	//	Accept-Ranges: bytes
	//	Content-Length: 6245
	//	Connection: close
	//	Content-Type: text/html
	
	char temp[100];

	if (UTILS_DEBUG) printf("Entered allocate_and_fill_http_request\n");

	char buffer[1000];
	buffer[0] = '\0';

	// if (UTILS_DEBUG) printf("buffer is %s, length is %d", buffer, strlen(buffer));

	//  HTTP status

	char * statusline;

	switch (status)
	{
		case 200: statusline = "HTTP/1.0 200 OK";
				  break;
		case 400: statusline = "HTTP/1.0 400 Bad Request";
				  break;
		case 403: statusline = "HTTP/1.0 403 Forbidden";
				  break;
		case 404: statusline = "HTTP/1.0 404 Not Found";
				  break;
		case 500: statusline = "HTTP/1.0 500 Internal Server Error";
				  break;
		case 501: statusline = "HTTP/1.0 501 Not Implemented";
				  break;
		case 503: statusline = "HTTP/1.0 503 Service Unavailable";
				  break;
		default:	
		{
			sprintf(temp, "HTTP/1.0 %d Some Other Error", status);
			statusline = temp;
			break;
		}
	}
	strcat(buffer, statusline);
	strcat(buffer, CRLF);

	//  Date
	
	strcat(buffer, "Date: Mon, 13 Sep 1999 01:03:58 GMT");
	strcat(buffer, CRLF);

	//  Content-length

	sprintf(temp, "Content-Length: %d", contentlength);
	strcat(buffer, temp);
	strcat(buffer, CRLF);

	//  Connection
	
	strcat(buffer, "Connection: close");
	strcat(buffer, CRLF);

	//  Content-Type
	
	strcat(buffer, "Content-Type: text/plain");	
	strcat(buffer, CRLF);

	//  CRLF
	
	strcat(buffer, CRLF);
	
	//  Allocate only what we need; return that pointer.

	int length = strlen(buffer);
	char * result;
	
	result = (char *) malloc ((length+1) * sizeof(char));
	strncpy(result, buffer, length);
	result[length] = '\0';

	*addr_of_buffer = result;
}


int content_offset_of(char * in)
{
	int last_was = 0;
	int i, length;

	length = strlen(in);

	for (i=0; i<length; i++)
	{

		if (in[i] == 13)
		{
			continue;
		}

		if (in[i] == 10)
		{
			if (last_was)
			{
				return (i+1);
			}
			else 
			{
				last_was = 1;
			}
		}
		else 
		{
			last_was = 0;
		}
	}
	return(-1);
}

int parse_response_header(char * in, int length, int *content_length)
{
	char * mycopy;
	char * currentline;
	char * currenttoken;
	char * temp;

	char ** placeincopy = &mycopy;
	char ** placeinline = &currentline;

	int responsecode = 0;

	mycopy = (char *) malloc ((length+1) * sizeof(char));
	strncpy(mycopy, in, length);

  //  Handle first line.
	
	temp = strtok_r(mycopy, "\n", placeincopy);
	if (temp) currentline = strdup(temp);

	if (UTILS_DEBUG)
	{
		printf("The first line of the header is:  ");
		printf("%s\n", currentline);
	}

	currenttoken = strtok_r(currentline, " ", placeinline);

	if (!strstr(currenttoken, "http"))
	{
		printf("Got an HTTP response.\n");
	}
	else
	{
		printf("Whoops!  We got a non-HTTP response. The first line was:\n");
		printf("%s\n", currentline);
		exit(0);
	}

	currenttoken = strtok_r(currentline, " ", placeinline);
	responsecode = atoi(currenttoken);

	currenttoken = strtok_r(currentline, "\015\012", placeinline);

	switch (responsecode)
	{
		case 200:	printf("Connection is okay.\n"); 
				break;
	  	case 400:	printf("Whoops -- it seems there was something ");
				printf("wrong with our request. My fault.\n");
				//				return(400);
				break;
		case 403:	printf("I'm sorry, the server says we can't ");
				printf("have that resource.\n");
				//return(403);
				break;
		case 404:	printf("I'm sorry, the server says it can't ");
				printf("find that resource.\n");
				//	return(404);
				break;
		case 500:	printf("Hmm -- the server is having some problem ");
				printf("fulfilling that request.\n");
				//return(500);
				break;
		case 501:	printf("The server says that method isn't ");
				printf("implemented.\n");
				//return(501);
				break;
		case 503:	printf("The service is unavailable -- the server might");
				printf(" have too many connections or something.\n");
				//return(503);
				break;
		default: 
		{
			if (responsecode >= 400)
			{
			    printf("Got an unknown response code of %d.\n",
					responsecode);
			    printf("The explanation is [%s].\n", currenttoken);
			    return(responsecode);
			}
			else
			{
			    printf("Got an unknown, but non-fatal, response code of %d.\n",
					responsecode);
			    printf("The explanation is [%s].\n", currenttoken);
			}
		}	
	}

  //  Handle remaining headers. Ignore most. 

	int found_content_length = 0;

	temp = strtok_r(mycopy, "\n", placeincopy);
	if (temp) currentline = strdup(temp);

	while (temp)
	{
		if (UTILS_DEBUG)
		{
			printf("The next line of the header is:  ");
			printf("%s\n", currentline);
		}

		currenttoken = strtok_r(currentline, " ", placeinline);

		if (!strcasecmp(currenttoken, "content-type:"))
		{
			currenttoken = strtok_r(currentline, " ", placeinline);
			
			if (1 || UTILS_DEBUG)
				printf("Content type is %s\n", currenttoken);
			if (strncasecmp(currenttoken, "text", 4))
			{
				printf("Sorry, but this client can only view text/* files.\n");
				return(600);
			}
		}
		else if (!strcasecmp(currenttoken, "content-length:"))
		{
			currenttoken = strtok_r(currentline, " ", placeinline);
			
			*content_length = atoi(currenttoken);
			
			found_content_length = 1;
			
			if (1 || UTILS_DEBUG)
			{
				// printf("Content length is %s\n", currenttoken);
				printf("Content length is %d\n", *content_length);				
			}
		}
		else
		{
			while (currenttoken)
			{
				if (UTILS_DEBUG) 
					printf("\tThe next token in the first line is %s\n",
						currenttoken);
				currenttoken = strtok_r(currentline, " ", placeinline);
			}		
		}

		temp = strtok_r(mycopy, "\n", placeincopy);
		if (temp) currentline = strdup(temp);

	}
	if (!found_content_length)
	{
		*content_length = IGNORE_THIS_VALUE;
		
		// printf("I'm sorry, but this client can only receive a resource if\n");
		// printf("the server tells it the Content-Length (that is, how big the\n");
		// printf("file is). The reason for this is that otherwise, the client\n");
		// printf(".... actually, let's see whether we can get around that.\n");
	}
	
	if (UTILS_DEBUG)
	{
		printf("Finished parsing...\n");
		printf("Returning responsecode == %d\n", responsecode);
	}
	return (responsecode);
}


int parse_request_header(char * in, char **filename, int max_filename_length)
{
	//  First off, anything that's not a GET is ignored.
	
	char * mycopy;
	char * currentline;
	char * currenttoken;
	char * temp;

	char ** placeincopy = &mycopy;
	char ** placeinline = &currentline;

	mycopy = (char *) malloc ((strlen(in)+4) * sizeof(char));
	strncpy(mycopy, in, max_filename_length);

  //  Handle first line.
	
	temp = strtok_r(mycopy, "\n", placeincopy);
	if (temp) currentline = strdup(temp);

	if (1 || UTILS_DEBUG)
	{
		printf("The first line of the header is:  ");
		printf("%s\n", currentline);
	}

	currenttoken = strtok_r(currentline, " ", placeinline);
	printf("First token is: %s\n", currenttoken);
	
	if (!strcasecmp(currenttoken, "GET"))
	{
		printf("Got a GET request.\n");
	}
	else
	{
		printf("Whoops!  We got a non-GET method.\n");
		return(-1);
	}

	currenttoken = strtok_r(currentline, " ", placeinline);
	printf("Next token is: %s\n", currenttoken);

	if (currenttoken[0] == '/')
	{
		printf("Looks like a filename, let's copy it in.\n");
		strcpy(*filename, currenttoken);
		printf("*filename set to %s\n", *filename);
		return(0);
	}
	else
	{
		printf("That doesn't sound like a URI to me, I'm afraid.\n");
		return(-1);
	}
	
	//  Yes, we're just going to ignore anything else that may 
	//  exist in the header.
	
	return 0;
}

















