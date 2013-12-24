#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/sendfile.h>

#define PORT 8081
#define EXEC_THREADS 1
void* execution();

int freethreads = EXEC_THREADS;

struct request_data
{
	long int ms_since_epoch;
	char message[1000];
	int type;
	int filetype;
	int acceptfd;
	long long filesize;
	char filename[1000];
	int file_found;
	struct sockaddr_in clientaddr;
};

struct node
{
	struct request_data request;
	struct node* next;
} *temp=NULL, *head=NULL,*last=NULL,*out=NULL;

pthread_mutex_t generic_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t insertion_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t exec_mutex=PTHREAD_MUTEX_INITIALIZER;
//pthread_cond_t list_volume = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_out = PTHREAD_COND_INITIALIZER;
//TO-DO: Implement file types.

void insertion(int acceptfd, char* message)
{
	//adopted from http://stackoverflow.com/questions/1952290/how-can-i-get-utctime-in-milisecond-since-january-1-1970-in-c-language
	struct timeval tp;
	gettimeofday(&tp,0);
	long int ms = tp.tv_sec + tp.tv_usec;
	//adopted code ends
	struct stat sb;
	temp = (struct node*)malloc(sizeof(struct node));
	temp->request.ms_since_epoch=ms;
	//temp->request.message = (char*)malloc(strlen(message));
	strcpy(temp->request.message,message);
	temp->request.acceptfd=acceptfd;
	char *mcopy = strdup(message);
	printf("hi there. after strdup message 1st.\n");
	char *type = strtok(mcopy," ");
	if(strcmp(type,"GET")==0)
		temp->request.type = 1;
	if(strcmp(type,"HEAD")==0)
		temp->request.type = 0;
	char *file = strtok(NULL," ");
	mcopy = strdup(message);
	printf("hi there. after strdup message 2nd.\n");
	strtok(mcopy,".");
	char *ft = strtok(NULL,".");
	printf("File:%s\n",file);
	if(strcmp(file,"/")==0)
	{
		if(stat("index.html",&sb)<0)
			temp->request.file_found=0;
		else
		{
			temp->request.file_found=1;
			//temp->request.filename = (char*)malloc(strlen("index.html"));
			strcpy(temp->request.filename,"index.html");
			temp->request.filesize = (long long)sb.st_size;
			temp->request.filetype = 0;
		}
	}
	else
	{   
		file++;
		if(stat(file,&sb)<0)
			temp->request.file_found=0;
		else
		{
			temp->request.file_found=1;
			//temp->request.filename = (char*)malloc(strlen(file));
			strcpy(temp->request.filename,file);
			stat(file,&sb);
			temp->request.filesize = (long long)sb.st_size;
			if(strcmp(ft,"gif")==0)
				temp->request.filetype=1;
			else if((strcmp(ft,"jpg")==0)||(strcmp(ft,"jpeg")==0))
				temp->request.filetype=2;
			else
				temp->request.filetype=3;

		}

	}
	if(head==NULL)
	{
		head = temp;
		temp = NULL;
		head->next=NULL;
		last = head;
		//pthread_cond_signal(&list_volume);
	}
	else
	{
		last->next=temp;
		last=temp;
		last->next=NULL;
		temp=NULL;
	}

}

//flag: if 0 it is a HEAD request. if 1 it is a GET request

char* build_header(char address[],int file_flag, int type_flag)
{
	printf("in build_header");
	struct stat file;
	char server_build[]="myhttpd/1.0 Welcome to the matrix.", timestamp[200],last_modified[200],status[20], content_type[20];
	time_t t = time(NULL);
	struct tm *tmp;
	tmp = gmtime(&t);
	strftime(timestamp,sizeof(timestamp),"%a %b %d, %Y %I:%M:%S %p",tmp);
	char* h;
	printf("Before stat in header.");   
	fflush(stdout);
	if(stat(address,&file)<0)
	{
		perror("Stat error.");
		exit(1);
	}
	fflush(stdout);
	tmp = gmtime(&file.st_mtime);
	strftime(last_modified,sizeof(last_modified),"%a %b %d, %Y %I:%M:%S %p",tmp);
	printf("Before first switch in header");
	fflush(stdout);
	switch(file_flag)
	{
		case 0:
			strcpy(status,"HTTP/1.0 404 NOT FOUND");
			break;
		case 1:
			strcpy(status,"HTTP/1.0 200 OK");
			break;
	}
	printf("before 2nd switch in header");
	fflush(stdout);
	switch(type_flag)
	{
		case 0:
			strcpy(content_type,"text/html");
			break;
		case 1:
			strcpy(content_type,"image/gif");
			break;
		case 2:
			strcpy(content_type,"image/jpg");
		case 3:
			strcpy(content_type,"application/octet-stream");
	}
	printf("after 2nd switch in header");
	fflush(stdout);
	int size = asprintf(&h,"%s\r\nDate: %s\r\nServer: %s\r\nLast-Modified: %s\r\nContent-Type: %s\r\nContent-Length:%lld\r\n\r\n",status,timestamp,server_build,last_modified,content_type,(long long) file.st_size);
	printf("After asprintf");
	fflush(stdout);
	char* header = (char*) malloc(strlen(h)+1);
	strcpy(header,h);
	//free(h);
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
		exit (1);
	}

	/* Give the socket a name. */
	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	servaddr.sin_addr.s_addr = htonl (INADDR_ANY);
	if (bind (sockfd, (struct sockaddr *) &servaddr, sizeof (servaddr)) < 0)
	{
		perror ("bind");
		exit (1);
	}

	if (listen(sockfd,5)<0)
	{
		perror("Listen error");
		exit(1);
	}
	return sockfd;
}
//for listening to incoming http requests
void* accept_func(void* sck)
{
//  printf("Entered accept\n");
	int *sockfd=(int*)sck;
	struct sockaddr_in clientaddr;
	int newsockfd, i;
	char message[2000];
	//char* header = build_header("index.html",1,0);
	socklen_t len = sizeof(clientaddr);
	while(1)
	{
		printf("Entered while in accept\n");
		newsockfd = accept(*sockfd, (struct sockaddr*) &clientaddr,&len);
		if(newsockfd<0)
		{
			perror("Accept error.");
			exit(1);
		}
		bzero(&message,sizeof(message));

		if((i=read(newsockfd,&message,sizeof(message)))<0)
		{
			perror("Receive error.");
			exit(1);
		}
		printf("i=%d\n",i);
		if(i>0)
		{
			printf("Number of bytes received: %d\n",i);
			printf("Message is:\n%s",message);
		}
		fflush(stdout);
/*      if(i=write(newsockfd,header,strlen(header))<0)
		{
			perror("write error.");
			exit(1);
		}
		printf("send:%d\n",i);
		close(newsockfd);*/
		pthread_mutex_lock(&insertion_mutex);
		printf("newsockfd=%d\n", newsockfd);
		insertion(newsockfd,message);
		pthread_mutex_unlock(&insertion_mutex);
		
	}
	close(*sockfd);
}

void* scheduler(void* sch)
{
	printf("Entered scheduler\n");
	int *sch_alg = (int*)sch;
	int i;
	//printf("Execution threads created.\n");
	if(*sch_alg==0)
	{
		//sleep(30);
		while(1)
		{
			sleep(2);
			//pthread_mutex_lock(&mutex);
			//while(head==NULL)
			//{
			//  pthread_cond_wait(&list_volume,&mutex);
			//}
			//pthread_mutex_unlock(&mutex);
			if(head==NULL)
				continue;
			pthread_mutex_lock(&insertion_mutex);
			out = head;
			head = head->next;
			if( pthread_cond_signal(&cond_out)!=0)
			{
				perror("cond signal failed");
				exit(1);
			}
			printf("scheduled a thing\n");
			pthread_mutex_unlock(&insertion_mutex);
		}
	}


}

void* execution()
{
	printf("worker spawned\n");
	int fd;
	off_t offset =0;
	int i,rc;
	struct stat stat_buf;
	char *header;
	char buffer[20000];

	while(1)
	{
		int n;
		printf("in exec before generic mutex\n");
		pthread_mutex_lock(&generic_mutex);
		fflush(stdout);
		pthread_mutex_lock(&exec_mutex);
		printf("In execution thread after exec_mutex\n");
		while(out==NULL)
		{
			pthread_cond_wait(&cond_out,&exec_mutex);
		}
	//  printf("i am up\n");
		pthread_mutex_unlock(&exec_mutex);
		//pthread_mutex_lock(&generic_mutex);
		struct node currentNode = *out;
		//pthread_mutex_unlock(&generic_mutex);
	//  printf("1\n");
		header = build_header(currentNode.request.filename,currentNode.request.file_found,currentNode.request.filetype);
		if(i=write(currentNode.request.acceptfd,header,strlen(header))<0)
		{
			perror("write error.");
			exit(1);
		}
	//  printf("2\n");
		fflush(stdout);
		memset(buffer,0,20000);     
		fd= open(currentNode.request.filename,O_RDONLY);
		while (1) {
		// Read data into buffer.  We may not have enough to fill up buffer, so we
		// store how many bytes were actually read in bytes_read.

			int bytes_read = read(fd, buffer, 20000);
			if (bytes_read == 0) // We're done reading from the file
				break;
	
			if (bytes_read < 0) {
			// handle errors
			}
	
			// You need a loop for the write, because not all of the data may be written
			// in one call; write will return how many bytes were written. p keeps
			// track of where in the buffer we are, while we decrement bytes_read
			// to keep track of how many bytes are left to write.
			char *p = buffer;
			while (bytes_read > 0) {
				int bytes_written = write(currentNode.request.acceptfd, p, bytes_read);
				printf("bytes_written = %d\n",bytes_written);
				if (bytes_written <= 0) {
					// handle errors
				}
				bytes_read -= bytes_written;
				p += bytes_written;
			}
		}   
		
		offset = 0;
	
	//  printf("i shouldnt be here\n");
		pthread_mutex_unlock(&generic_mutex);
	}
	

}

void printUsage(void)
{
	/* https://gist.github.com/kyunghoj/6986073 */
	fprintf(stderr, "Usage: myhttpd [−d] [−h] [−l file] [−p port] [−r dir] [−t time] [−n thread_num]  [−s sched]\n");
	fprintf(stderr,
			"\t−d : Enter debugging mode. That is, do not daemonize, only accept\n"
			"\tone connection at a time and enable logging to stdout.  Without\n"
			"\tthis option, the web server should run as a daemon process in the\n"
			"\tbackground.\n"
			"\t−h : Print a usage summary with all options and exit.\n"
			"\t−l file : Log all requests to the given file. See LOGGING for\n"
			"\tdetails.\n"
			"\t−p port : Listen on the given port. If not provided, myhttpd will\n"
			"\tlisten on port 8080.\n"
			"\t−r dir : Set the root directory for the http server to dir.\n"
			"\t−t time : Set the queuing time to time seconds.  The default should\n"
			"\tbe 60 seconds.\n"
			"\t−n thread_num : Set number of threads waiting ready in the execution thread pool to\n"
			"\tthreadnum.  The default is 4 execution threads.\n"
			"\t−s sched : Set the scheduling policy. It can be either FCFS or SJF.\n"
			"\tThe default will be FCFS.\n"
			);
}

int main(int argc, char* argv[])
{
	int sockfd;
	int sch_alg=0;
	int i,c;
	int opterr = 0;
	int debug = 0;
    int port = 8080;
    char *logfile = NULL;
    char *rootdir  = "/";
    int n_threads = 4;
    int queueing_time = 60;
 
    char *schedPolicy = "FCFS";
 
 
	if (argc < 2)
	{
		printUsage();
		exit(1);
	}
 
	while ( ( c = getopt (argc, argv, "dhl:p:r:t:n:s:") ) != -1 )
	{
		switch (c)
		{
			case 'd':
				debug = 1;
				break;
			case 'h':
				printUsage();
				exit(1);
			case 'l':
				logfile = optarg;
				break;
			case 'p':
				port = atoi(optarg);
				if (port < 1024)
				{
					fprintf(stderr, "[error] Port number must be greater than or equal to 1024.\n");
					exit(1);
				}
				break;
			case 'r':
				rootdir = optarg;
				chdir(rootdir);
				break;
			case 't':
				queueing_time = atoi(optarg);
				if (queueing_time < 1)
				{
						fprintf(stderr, "[error] queueing time must be greater than 0.\n");
						exit(1);
				}
				break;
			case 'n':
				n_threads = atoi(optarg);
				if (n_threads < 1)
				{
						fprintf(stderr, "[error] number of threads must be greater than 0.\n");
						exit(1);
				}
				break;
			case 's':
				schedPolicy = optarg;
				break;
			default:
				printUsage();
				exit(1);
		}
	} // while (...)
 
	if (debug == 1)
	{
		fprintf(stderr, "myhttpd logfile: %s\n", logfile);
		fprintf(stderr, "myhttpd port number: %d\n", port);
		fprintf(stderr, "myhttpd rootdir: %s\n", rootdir);
		fprintf(stderr, "myhttpd queueing time: %d\n", queueing_time);
		fprintf(stderr, "myhttpd number of threads: %d\n", n_threads);
		fprintf(stderr, "myhttpd scheduling policy: %s\n", schedPolicy);
	}
	else
	{
		pid_t pid;
		if ((pid=fork())<0) return (-1);
		else if (pid!=0) exit (0);
		setsid();
		umask(0);
	}
	sockfd = server_init(port);
	pthread_t sch_thread, acc_thread;
	pthread_create(&sch_thread,NULL,scheduler,(void*)&sch_alg);
	pthread_create(&acc_thread,NULL,accept_func,(void*)&sockfd);
	//sleep(20);
	pthread_t exec_thread[EXEC_THREADS];
	for(i=0;i<EXEC_THREADS;i++)
		 pthread_create(&exec_thread[i],NULL,execution,NULL);
	pthread_join(acc_thread,NULL);
	pthread_join(sch_thread,NULL);
 
	return 0;
}