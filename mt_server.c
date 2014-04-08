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
#include <signal.h>
#include <time.h>
#include <fcntl.h>
#include <sys/time.h>

#define BACKLOG 10
#define STDIN 0

char PageContent[150000];

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

int count=0,i,j,k=0;

char HeaderTemplate[] =
    "HTTP/1.0 200 OK\r\n"
    "Server: PRIMITIVE Server\r\n"
    "Date Time: %s\r\n"
    "Content-Type: text/html\r\n"
    "Accept-Ranges: bytes\r\n"
    "Content-Length: %d\r\n\r\n";

char GMTNow[1000],f2[150000];
char ClntRequest[1000];
char HttpHeader[1000];

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

int main(int argc,char **argv)
{
strcpy(PageContent,"<html><head><title>SERVER</title></head><body BGCOLOR=\"#000000\" background=\"image.jpg\" TEXT=\"#80c0c0\" LINK=\"#ff8080\" VLINK=\"#ff0000\" ALINK=\"#a05050\"><div ALIGN=\"center\"><table BGCOLOR=\"#000080\" BORDER=3 BORDERCOLOR=\"#0000ff\"><tr><td><h1><font FACE=\"courier\" SIZE=6><font SIZE=7>W</font>ELCOME TO Server!</font></h1></td></tr></table></div><br><br><p>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Hi there! This Server is dedicated to multiple clients in one go ..!.</p><table BGCOLOR=\"#000080\"><tr><td WIDTH=50>&nbsp;</td><td><font COLOR=\"#00ff00\">Brought to you by:<font FACE=\"'comic sans ms', fantasy\" COLOR=\"#ffff00\"><u>Anshul Gupta</u> <u>Karmesh Gupta</u> <u>Ashutosh Shukla</u> <u>Anurag Vyas</u></font></td></tr></table><br> <div><b>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<font SIZE=\6 COLOR=\"#ffffff\">W</font>hen you are done looking through these masterpieces, I encourage you to visit again!</b></div></body></html>");

	time_t timer;
	char buffer[25],buf[100];
	struct tm* tm_info;
	struct timeval tv;
	fd_set readfds;
	tv.tv_sec=119;
	tv.tv_usec=1000000;

	FILE * slog;

        time(&timer);
        tm_info = localtime(&timer);

	int i, pFlag, lFlag, vFlag, port;
	strftime(buffer, 25, "%Y-%m-%d %H:%M:%S", tm_info);
        strcpy(GMTNow,buffer);

	int sockfd,acc,yes=1,gadd,num;
	struct addrinfo hints, *res, *p,f;
	struct sockaddr_storage serv_store; 
	socklen_t len_sock;
	struct sigaction sa;
	char s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; 
	hints.ai_protocol=0;

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
						slog=fopen(argv[i+1], "w");
						fprintf(slog, "Logfile Created");	
						lFlag=1;
					}
					else
					{
						printf("server.c : logfile name not specified with '%s'\n",argv[i]);
						return 1;
					}
					break;
				case 'v':
					vFlag = 1;
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
		//fcntl(sockfd, F_SETFL, O_NONBLOCK); // to prevent blocking of socket by kernel
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

	if (listen(sockfd, BACKLOG) == -1) 
	{
		perror("ERROR : LISTENING");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; 
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;

	if (sigaction(SIGCHLD, &sa, NULL) == -1) 
	{
		perror("ERROR : sigaction");
		exit(1);
	}

	printf("Server: waiting for connections(Press enter key for allowing to connect with client)...\n");
	// Server timer started
	FD_ZERO(&readfds);	
	FD_SET(STDIN,&readfds);
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
		while(1) 
		{ 
			len_sock = sizeof(serv_store);
			acc = accept(sockfd,(struct sockaddr *)&serv_store,&len_sock);
		
			sprintf(HttpHeader, HeaderTemplate, GMTNow, strlen(PageContent));

			if (acc == -1) 
			{
				perror("ERROR : accept");
				continue;	
			}

			recv(acc, ClntRequest, sizeof(ClntRequest), 0);
			printf("%s\n",HttpHeader);
			printf("%s\n",ClntRequest);
			//extracting the html request from client's request
			char ctr[100];
			for(i=0;i<strlen(ClntRequest);i++)
			{
				if(ClntRequest[i] == '/')
				{
					for(j=i+1;ClntRequest[j] != ' ';j++)
					{
						ctr[k]=ClntRequest[j];
						k++;
					}
					break;
				}
			}
			char f1[150000];
			if(strcmp(ctr,"") != 0)
			{
				printf("This is the file you requested for : %s\n",ctr);
			}
			else
			{
				printf("This is our server's Home Page.\n");
			}
			FILE *fp1=fopen(ctr,"r");
			char ftr1[100000],ftr[100000]="<html><head><title>ERROR</title></head><body><h1>404</h1><h1> Page Not Found </h1></body></html>";
			if(fp1 == NULL && strcmp(ctr,"") != 0)
			{				
					strcpy(PageContent,ftr);
			}	
			else if(fp1!=NULL)
			{
				//sleep(2);
				i=0;
				strcpy(PageContent," ");
				char ch;
				while(1)
				{
					ch=fgetc(fp1);
					if(ch==EOF)
					{
						break;
					}
					
					f1[i]=ch;
					i++;
				}
				int k=0;
				//sleep(2);
				for(j=0;j<i;j++)
				{
					if(f1[j] == '"')
					{
						PageContent[k]='\\';
						k++;		
						PageContent[k]=f1[j];
						k++;
					}
					else
					{
						PageContent[k]=f1[j];
						k++;
					}
				}
				PageContent[k]='\0';
				printf("%s\n",PageContent);
				fclose(fp1);
				//sleep(2);
			}	
			
			inet_ntop(serv_store.ss_family,get_in_addr((struct sockaddr *)&serv_store),s, sizeof s);
			printf("server: got connection from %s\n", s);
			//Log File
			time_t now;	// getting the time the job has been assigned to the serving thread
		        time(&now);
	       	        struct tm * ct=localtime(&now); //getting localtime
	       	        char ch[128], time_serve[128];
	       	        struct timeval tv;
	       	        strftime(ch, sizeof ch, "[%d/%b/%Y : %H:%M:%S %z]", ct); //format of the timestamp string we need
	       	        snprintf(time_serve, sizeof time_serve, ch, tv.tv_usec); //printing the needed timestamp string

			struct request r;
			count++;

			if(lFlag)
				fprintf(slog,"%s Request Received.", timestamp());
	
               		//child process removed
			close(sockfd); 
			
			if(send(acc, HttpHeader, strlen(HttpHeader),0) == -1)
			{
				perror("write");
			}
			int len=strlen(PageContent);

			sleep(2);
			//calculating exact time server took to send
			struct timeval stop, start;
			gettimeofday(&start, NULL);
			if(sendall(acc,PageContent,&len) == -1)
			{
				perror("write");	
			}
			gettimeofday(&stop, NULL);
			printf("Server took %lums time to send the data\n", stop.tv_usec - start.tv_usec);
			sleep(2);
			//bringing all the html files into foo.txt
			if(system("./test.sh") == -1)
			{
				perror("ERROR: BRINGING FILES");
			}
			FILE *fp=fopen("foo.txt","r");
			char c,f[100],str[100];
			int i=0;
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
			fclose(fp);
			/*send(acc,"Choose Files",12,0);
			//sending html files to client
			if(send(acc,f,strlen(f),0) == -1)
			{
				perror("write");	
			}*/
			//receiving file choice requests of client
			//recv(acc,str,strlen(str),0);
			//printf("You Chose %s file",str);
			close(acc);
			exit(0);
		}
	}
	return 0;
}
