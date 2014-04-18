#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>
#include <sys/time.h>

//#define BACKLOG 10
#define STDIN 0


char PageContent[150000];
int bl_counter=0;
pthread_mutex_t listen_lock;
int count=0,i,j,k=0;
char GMTNow[1000],f2[150000];
char ClntRequest[1000];
char HttpHeader[1000];
char HeaderTemplate[] =
    "HTTP/1.0 200 OK\r\n"
    "Server: PRIMITIVE Server\r\n"
    "Date Time: %s\r\n"
    "Content-Type: text/html\r\n"
    "Accept-Ranges: bytes\r\n"
    "Content-Length: %d\r\n\r\n";

time_t timer;
char buffer[25],buf[100];
struct tm* tm_info;
struct timeval tv;
fd_set readfds;

int BACKLOG=10;


FILE *slog;

struct sigaction sa;

int sockfd,acc,yes=1,gadd,num;
struct addrinfo hints, *res, *p,f;
struct sockaddr_storage serv_store; 
socklen_t len_sock;
char s[INET6_ADDRSTRLEN];


struct request
{
	int acceptfd;
	int size;
	char file_name[1024];
	unsigned int cli_ipaddr[1024];
	char time_arrival[1024];
	char in_buf[2048];
	
}r2;

int sendall(int s, char *buf, int *len)
{
	int total = 0; // how many bytes we've sent
	int bytesleft = *len; // how many we have left to send
	int n;
	while(total < *len) {
	n = send(s, buf+total, bytesleft, 0);
	if (n == -1) { break; }
	total += n;
	bytesleft -= n;
	}
	*len = total; // return number actually sent here
	return n==-1?-1:0; // return -1 on failure, 0 on success
}



char *timestamp()
{
	char *tstamp  = (char *)malloc(sizeof(char) * 16);
	time_t ltime;
	ltime=time(NULL);
	struct tm *tm;
	tm=localtime(&ltime);
	sprintf(tstamp,"%02d/%02d/%04d  %02d:%02d:%02d > ",  tm -> tm_mon, tm -> tm_mday, (tm -> tm_year)+1900, tm -> tm_hour, tm -> tm_min, 			tm -> tm_sec);
	return tstamp;
}

void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
	{
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void *sock_listen(void *socket_fd)
{
	strcpy(PageContent,"<html><head><title>SERVER</title></head><body bgcolor=\"f0f0f0\"><div ALIGN=\"center\"><table BORDER=3 ><tr><td><h1><font FACE=\"courier\" SIZE=6><font SIZE=7>M</font>ulti Threaded Web Server</font></h1></td></tr></table></div><br><center><<br><p>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;This is the homepage created for server</p><table><tr><td WIDTH=50>&nbsp;</td><td><font> Created by:<font FACE=\"'comic sans ms', fantasy\"><u>Anshul Gupta</u> <u>Ashutosh Shukla</u> <u>Karmesh Gupta</u> <u>Anurag Vyas</u></font></td></tr></table><br> <div><b>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<font SIZE=\6 >Y</font>ou are connected to Multi threaded Web Server</b></div></center><</body></html>");

	tv.tv_sec=119;
	tv.tv_usec=1000000;
	int sock_fd = *((int *)socket_fd);
	int req_counter=0;
	int pg_counter=0;

	FILE *slog=fopen("logfile.txt","a+");
	int lFlag=0;

	if (listen(sock_fd, BACKLOG) == -1) 
	{
		perror("ERROR : LISTENING");
		exit(1);
	}
	//to remove Zombie Processes
	sa.sa_handler = sigchld_handler; 
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	//sigaction reaps zombie Processes
	if (sigaction(SIGCHLD, &sa, NULL) == -1) 
	{
		perror("ERROR : sigaction");
		exit(1);
	}
	FD_ZERO(&readfds);	
	FD_SET(STDIN,&readfds);
	bl_counter++;
	if(bl_counter==1)
		printf("Server: waiting for connections(Press enter key for allowing to connect with client)...\n");

	int rv;
	rv = select(STDIN + 1, &readfds, NULL, NULL, &tv);
	if (rv == -1)
	{
		perror("select"); // error occurred in select()
	}
	else if (rv == 0)
	{
		printf("Sorry, Server Timed out! No data after 120 seconds.\n");
		exit(4);
	}
	else
	{
		pthread_mutex_lock(&listen_lock);
		//while(1) 
		//{
			len_sock = sizeof(serv_store);
			acc = accept(sock_fd,(struct sockaddr *)&serv_store,&len_sock);
			
			if (acc == -1) 
			{
				perror("ERROR : accept");
				//break;	
			}
			printf("%d", sock_fd);

			recv(acc, ClntRequest, sizeof(ClntRequest), 0);

			

			//printf("%s\n",HttpHeader);
			printf("%s\n",ClntRequest);
			
			inet_ntop(serv_store.ss_family,get_in_addr((struct sockaddr *)&serv_store),s, sizeof s);
			//extracting the html request from client's request
			char ctr[100];
			req_counter=0;
			for(i=0;i<strlen(ClntRequest);i++)
			{
				if(ClntRequest[i] == '/')
				{
					for(j=i+1;ClntRequest[j] != ' ';j++)
					{
						ctr[req_counter]=ClntRequest[j];
						req_counter++;
					}
					break;
				}
			}
			ctr[req_counter]='\0';
			char f1[150000];
			if(strcmp(ctr,"") != 0)
			{
				printf("This is the file you requested for : %s\n",ctr);
			}
			else
			{
				printf("This is our server's Home Page.\n");
				strcpy(PageContent,"<html><head><title>SERVER</title></head><body bgcolor=\"f0f0f0\"><div ALIGN=\"center\"><table BORDER=3 ><tr><td><h1><font FACE=\"courier\" SIZE=6><font SIZE=7>M</font>ulti Threaded Web Server</font></h1></td></tr></table>/div><br><center><<br><p>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;This is the homepage created for server</p><table><tr><td WIDTH=50>&nbsp;</td><td><font> Created by:<font FACE=\"'comic sans ms', fantasy\"><u>Anshul Gupta</u> <u>Ashutosh Shukla</u> <u>Karmesh Gupta</u> <u>Anurag Vyas</u></font></td></tr></table><br> <div><b>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<font SIZE=\6 >Y</font>ou are connected to Multi threaded Web Server</b></div></center><</body></html>");
			}
			FILE *fp1=fopen(ctr,"r");
			char ftr1[100000],ftr[100000]="<html><head><title>ERROR</title></head><body><h1>404</h1><h1> Page Not Found </h1><br><b>The html and txt files present in the server's root directory are :-</b><br><br></body></html>";
			if(fp1 == NULL && strcmp(ctr,"") != 0)
			{				
				strcpy(PageContent,ftr);

				//Log File
				if(acc != -1 && lFlag != 1)
				{
					time_t now;	// getting the time the job has been assigned to the serving thread
					time(&now);
			       	        struct tm * ct=localtime(&now); //getting localtime
			       	        char ch[128], time_serve[128];
			       	        struct timeval tv;
			       	        strftime(ch, sizeof ch, "  [%d/%b/%Y : %H:%M:%S %z]", ct); //format of the timestamp string we need
			       	        snprintf(time_serve, sizeof time_serve, ch, tv.tv_usec); //printing the needed timestamp string
					char log_c[100]=" > Fetch Error 404 NOT FOUND";
					//strcat(log_c,time_serve);
					fputs(s, slog);
					fputs(time_serve,slog);
					fputs(log_c, slog);
					fputc('\n', slog);
					struct request r;
					lFlag=1;
					count++;
				}
				//bringing all the html files into foo.txt
				if(system("./test.sh") == -1)
				{
					perror("ERROR: BRINGING FILES");
				}
				FILE *fp=fopen("foo.txt","r");
				char c,f[100],str[100];
			        i=0;
				//storing contents of foo.txt in a string	
				while(1)
				{
					c=fgetc(fp);
					if(c==EOF)
					{		
						break;
					}
					f[i]=c;
					i++;
				}
				strcat(PageContent,f);
				fclose(fp);

			}
			else if(fp1!=NULL)
			{
				//sleep(2);
				i=0;
				//PageContent[0]='\0';
				char ch;
				while(1)
				{
					ch=fgetc(fp1);
					if(ch==EOF)
					{
						break;
					}
					if(ch=='\n' || ch=='\t')
					{
						f1[i]=' ';
						i++;
						continue;
					}
					f1[i]=ch;
					i++;
				}
				f1[i]='\0';
				//printf("%s\n", f1);
				pg_counter=0;
				//sleep(2);
				for(j=0;j<i;j++)
				{
					if(f1[j] == '"')
					{
						PageContent[pg_counter]='\\';
						pg_counter++;		
						PageContent[pg_counter]=f1[j];
						pg_counter++;
					}
					else
					{
						PageContent[pg_counter]=f1[j];
						pg_counter++;
					}
				}
				PageContent[pg_counter]='\0';
				//printf("PageContent:\n%s\n", PageContent);
				fclose(fp1);
				//sleep(2);
			}	
			inet_ntop(serv_store.ss_family,get_in_addr((struct sockaddr *)&serv_store),s, sizeof s);
			printf("server: got connection from %s\n", s);

			sprintf(HttpHeader, HeaderTemplate, GMTNow, strlen(PageContent));
			printf("\n%s\n",HttpHeader);
			//Log File
			if(acc != -1 && lFlag != 1)
			{
				time_t now;	// getting the time the job has been assigned to the serving thread
				time(&now);
		       	        struct tm * ct=localtime(&now); //getting localtime
		       	        char ch[128], time_serve[128];
		       	        struct timeval tv;
		       	        strftime(ch, sizeof ch, "  [%d/%b/%Y : %H:%M:%S %z]", ct); //format of the timestamp string we need
		       	        snprintf(time_serve, sizeof time_serve, ch, tv.tv_usec); //printing the needed timestamp string
				char log_c[100]=" > Connection Success 200 OK";
				//strcat(log_c,time_serve);
				fputs(s, slog);
				fputs(time_serve,slog);
				fputs(log_c, slog);
				fputc('\n', slog);
				lFlag = 1;
				struct request r;
				count++;
			}
			//fprintf(slog,"%s Request Received.", timestamp());
	
               		//child process removed
			
			if(send(acc, HttpHeader, strlen(HttpHeader),0) == -1)
			{
				perror("write");
			}
			int len=strlen(PageContent);
			sleep(1);
			//calculating exact time server took to send
			struct timeval stop, start;
			gettimeofday(&start, NULL);
			//printf("%s", PageContent);
			if(sendall(acc,PageContent,&len) == -1)
			{
				perror("send");
			}
			gettimeofday(&stop, NULL);
			printf("Server took %lums time to send the data\n", stop.tv_usec - start.tv_usec);
			fclose(slog);
			//sleep(1);
			close(acc);
		//}
		pthread_mutex_unlock(&listen_lock);
	}
}


int main(int argc,char **argv)
{

        time(&timer);
        tm_info = localtime(&timer);

	int i, pFlag, lFlag, vFlag, port;
	strftime(buffer, 25, "%Y-%m-%d %H:%M:%S", tm_info);
        strcpy(GMTNow,buffer);

	memset(&hints, 0, sizeof hints);

	// Variables for threading and locking
	pthread_t listen_thread[BACKLOG];
	
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; 
	hints.ai_protocol=0;

	pthread_mutex_init(&listen_lock, NULL);

	for(i = 0;i < argc;i++)
	{
		if(argv[i][0] == '-')
		{
			if(argv[i][2] != '\0')
			{
				printf("server.c : option '%s' not recognized.\n",argv[i]);
				printf("server.c : '-h' provides argument help\n");
				return 1;
			}

			switch(argv[i][1])
			{
				case 'h':
					printf("Usage : ./server [options] [value]\n");
					printf("Options :\n");
					printf("\t-h\t\tDisplays this help.\n");
					printf("\t-p\t\tSpecify port.\n");
					printf("\t-l\t\tSpecify creation of logfile. Takes name as argument\n");
					printf("\t-v\t\tEnables verbose.\n");
					return 0;
					break;
				case 'p':
					if(((i+1) < argc) && (argv[i+1][0] != '-'))
					{
						if ((gadd = getaddrinfo(NULL, argv[i+1], &hints, &res)) != 0) 
						{
							perror("ERROR : getaddrinfo");
							return 1;
						}
						pFlag = 1;
					}
					else
					{
						printf("server.c : port no. not specified with '%s'\n",argv[i]);
						return 1;
					}
					break;
				case 'l':
					if(((i+1) < argc) && (argv[i+1][0] != '-'))
					{
						slog=fopen(argv[i+1], "a+");
						fprintf(slog, "Logfile Created\n");
						lFlag=1;
					}
					else
					{
						printf("server.c : logfile name not specified with '%s'\n",argv[i]);
						return 1;
					}
					break;
				case 'b':
					if(((i+1) < argc) && (argv[i+1][0] != '-'))
						BACKLOG=atoi(argv[i+1]);
					break;
				default:
					printf("server.c : option '%s' not recognized.\n",argv[i]);
					printf("server.c : '-h' displays arguments help\n");
					return 1;
			}
		}
	}

	for(p = res; p != NULL; p = p->ai_next)
	{
		if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1)
		{
		perror("ERROR : SOCKET CREATION");
		continue;
		}
		//fcntl(sockfd, F_SETFL, O_NONBLOCK);
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,sizeof(int)) == -1)
		{
			perror("setsockopt");
			exit(1);
		}
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) 
		{
			close(sockfd);
			perror("ERROR : SERVER BIND");
			continue;
		}
		break;
	}
	if (p == NULL) 
	{
		perror("ERROR : SERVER FAILED TO BIND");
		return 2;
	}

	freeaddrinfo(res); 
	
	// Thread Creation
	for(i=0;i<BACKLOG;i++)
		pthread_create(&listen_thread[i], NULL, (void *) &sock_listen, (void *)&sockfd);
	for(i=0;i<BACKLOG;i++)
		pthread_join(listen_thread[i], NULL);
	
	close(sockfd);
	pthread_mutex_destroy(&listen_lock);
	return 0;
}
