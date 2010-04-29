#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define IGNORE_THIS_VALUE	-13

#define CRLF	"\015\012\000"

#define HTTP	0
#define FTP	1
#define GOPHER	2
#define MAILTO	3
#define TELNET	4

typedef struct
{
	int protocol;
	char * hostname;
	int port;
	char * path;	
} url_struct;

typedef url_struct * url;

int initialize_url(url u);
void pretty_print(url u);
int parse_as_url(char * in, url u);
int parse_as_protocol(char * in);
int get_ip_int(url u);
int set_default_port(url u);
void allocate_and_fill_http_request(char ** addr_of_buffer, url u);
void allocate_and_fill_http_response(char ** addr_of_buffer, int status, int contentlength);
int content_offset_of(char * in);
int parse_response_header(char * in, int length, int *content_length);
int parse_request_header(char * in, char **filename, int max_filename_length);


#endif



