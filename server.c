#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <netinet/in.h>
#include <time.h>

#define PORT 8080
#define OK_200 "HTTP/1.0 200 OK"
#define IMG "image/gif"
#define TXT "text/html"
#define NF_404 "HTTP/1.0 404 NOT FOUND"

//flag: if 0 it is a HEAD request. if 1 it is a GET request
char* build_header(char address[],int file_flag, int type_flag)
{
	struct stat file;
	char server_build[]="myhttpd/1.0 Welcome to the matrix.", timestamp[200],last_modified[200],status[20], content_type[20];
	time_t t = time(NULL);
	struct tm *tmp;
	tmp = gmtime(&t);
	strftime(timestamp,sizeof(timestamp),"%a %b %d, %Y %I:%M:%S %p",tmp);
	char* h;
	
	if(stat(address,&file)<0)
	{
		perror("Stat error.");
		exit(1);
	}
	tmp = gmtime(&file.st_mtime);
	strftime(last_modified,sizeof(last_modified),"%a %b %d, %Y %I:%M:%S %p",tmp);
	
	switch(file_flag)
	{
		case 0:
			strcpy(status,"HTTP/1.0 404 NOT FOUND");
			break;
		case 1:
			strcpy(status,"HTTP/1.0 200 OK");
			break;
	}
	switch(type_flag)
	{
		case 0:
			strcpy(content_type,"text/html");
			break;
		case 1:
			strcpy(content_type,"image/gif");
			break;
	}
	int size = asprintf(&h,"%s\r\nDate: %s\r\nServer: %s\r\nLast-Modified: %s\r\nContent-Type: %s\r\nContent-Length:%lld\r\n\r\n",status,timestamp,server_build,last_modified,content_type,(long long) file.st_size);
	char* header = (char*) malloc(sizeof(h));
	strcpy(header,h);
	return header;
}


int server_init (uint16_t port)
{ 
	int sockfd;
	struct sockaddr_in servaddr;

	/* Create the socket. */
	sockfd = socket (PF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		perror ("socket");
		exit (EXIT_FAILURE);
	}

	/* Give the socket a name. */
	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	servaddr.sin_addr.s_addr = htonl (INADDR_ANY);
	if (bind (sockfd, (struct sockaddr *) &servaddr, sizeof (servaddr)) < 0)
	{
		perror ("bind");
		exit (EXIT_FAILURE);
	}

	if (listen(sockfd,5)<0)
	{
		perror("Listen error");
		exit(1);
	}
	return sockfd;
}

void accept_func(int sockfd)
{
	struct sockaddr_in clientaddr;
	int newsockfd, i;
	char message[1000];
	char* header = build_header("index.html",1,0);
	socklen_t len = sizeof(clientaddr);
	while(1)
	{
		newsockfd = accept(sockfd, (struct sockaddr*) &clientaddr,&len);
		if(newsockfd<0)
		{
			perror("Accept error.");
			exit(1);
		}
		bzero(&message,sizeof(message));
		if((i=read(newsockfd,message,sizeof(message)))<0)
		{
			perror("Receive error.");
			exit(1);
		}
		if(i==0)
		{
			printf("Socket closed.");
		}
		if(i>0)
		{
			printf("Number of bytes received: %d\n",i);
			printf("Message is:\n%s",message);
		}
		if(i=send(newsockfd,header,strlen(header),0)<0)
		{
			perror("write error.");
			exit(1);
		}
		printf("send:%d\n",i);
		close(newsockfd);
		
	}
	close(sockfd);
}

int main(int argc, char* argv[])
{
	int sockfd;
	sockfd = server_init(PORT);
	accept_func(sockfd);
}