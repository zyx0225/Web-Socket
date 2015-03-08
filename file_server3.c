/*
** modify talker.c (datagram sockets "client") as file server 1 in phase1
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/wait.h>  
#include <signal.h>    

#define MYPORT_UDP "24992"	// the port file server3 will use to connect directory server
#define SERVERPORT 21992	// the port file server3 will be connecting to
#define MYPORT_TCP "43992"  //the port file server3 will use in phase 3

#define BACKLOG 10  //how many pending connections queue will hold
#define MAXBUFLEN 100

#define DS_HN "nunki.usc.edu"
char FS3_HN[128];
char DS_IP[100],FS3_IP[100];

void *get_in_addr(struct sockaddr *sa)  //get sockaddr, IPv4 or IPv6
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

int hostname_to_ip(char * hostname , char* ip) /* function to retrieve the IP address from a host name*/
{
    struct hostent *he;
    struct in_addr **addr_list;
    int i;
         
    if ( (he = gethostbyname( hostname ) ) == NULL) 
    {
        // get the host info
        herror("gethostbyname");
        return 1;
    }
 
    addr_list = (struct in_addr **) he->h_addr_list;
     
    for(i = 0; addr_list[i] != NULL; i++) 
    {
        //Return the first one;
        strcpy(ip , inet_ntoa(*addr_list[i]) );
        return 0;
    }
     
    return 1;
}

static int phase1()
{
	int sockfdc;
	struct addrinfo hints, *clientinfo, *pc, *serverinfo;
	struct sockaddr_in servaddr;
	int rv;
	int numbytes;
	char msg[100]="File_Server3 43992"; /*msg send to directory server*/
	
	gethostname(FS3_HN, sizeof FS3_HN);
	hostname_to_ip(DS_HN, DS_IP);    //convert hostname to ip address
	hostname_to_ip(FS3_HN, FS3_IP); 

	memset(&servaddr, 0, sizeof(servaddr));   //load directory server info
	servaddr.sin_family=AF_INET;
	inet_pton(AF_INET,DS_IP, &servaddr.sin_addr.s_addr);
	servaddr.sin_port=htons(SERVERPORT);
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	
	if ((rv = getaddrinfo(FS3_HN, MYPORT_UDP, &hints, &clientinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	} 
 
 	// loop through all the results, create and bind socket
	for(pc = clientinfo; pc != NULL; pc = pc->ai_next) {
		if ((sockfdc = socket(pc->ai_family, pc->ai_socktype,
				pc->ai_protocol)) == -1) {
			perror("talker: socket");
			continue;
		}
		if (bind(sockfdc, pc->ai_addr, pc->ai_addrlen) == -1) {
			close(sockfdc);
			perror("talker: bind");
			continue;
		}
		break;
	}

	if (pc == NULL) {
		fprintf(stderr, "talker: failed to bind socket clientinfo\n");
		return 2;
	}

	struct sockaddr_in fs3addr;
	socklen_t fs3addr_len = sizeof(fs3addr);

	getsockname(sockfdc,(struct sockaddr *)&fs3addr,&fs3addr_len); //get the port number and IP add

	printf("Phase 1: File Server 3 has UDP port number %d and IP address %s. \n",ntohs(fs3addr.sin_port), inet_ntoa(fs3addr.sin_addr));

	if ((numbytes = sendto(sockfdc, msg, strlen(msg), 0, (struct sockaddr *)&servaddr, sizeof(servaddr))) == -1) {
		perror("talker: sendto");
		exit(1);
	}
 
	printf("Phase 1: The Registration request from File Server 3 has been sent to the Directory Server.\n");

	close(sockfdc);

	printf("Phase 1: End of Phase 1 for File Server 3.\n");

	return 0;
}


static int phase3()  //phase 3 for file server
{
	int sockfd, new_fd, numbytes;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char buf[MAXBUFLEN];
	char *file, *client, *p1;
	char s[INET6_ADDRSTRLEN];
	int rv;
	const char *d;
    d=" ";

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(FS3_HN, MYPORT_TCP, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	struct sockaddr_in fs3addr;
	socklen_t fs3addr_len = sizeof(fs3addr);

	getsockname(sockfd,(struct sockaddr *)&fs3addr,&fs3addr_len); //get the port number and IP add

	printf("Phase 3: File Server 3 has TCP port number %d and IP address %s.\n",ntohs(fs3addr.sin_port), inet_ntoa(fs3addr.sin_addr));

	freeaddrinfo(servinfo); // all done with this structure

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			if(errno==EINTR)
				continue;
			perror("accept");
				exit(1);
		}

		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);

		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener
			
			//receive from the client
			//identify the requested file and send it back to client
			if ((numbytes = recv(new_fd, buf, MAXBUFLEN-1, 0)) == -1) {
					perror("recv");
					exit(1);
			}
			buf[numbytes] = '\0';

			if(strcmp(buf,"Client1 doc1")==0)   
			{
				if (send(new_fd, "doc1", 4, 0) == -1)  // send requested file to client1
					perror("send to Client1");
			}
			else                                 
			{
				if (send(new_fd, "doc2", 4, 0) == -1)  //send requested file to client2
					perror("send to Client2");
			}

			p1=strtok(buf,d);
			if(p1) client=p1;
			p1=strtok(NULL,d);
			if(p1) file=p1;

			printf("Phase 3: File Server 3 received the request from the %s for the file %s.\n", client, file);
			printf("Phase 3: File Server 3 has sent %s to %s.\n", file, client);

			close(new_fd);
			exit(0);
		}
		close(new_fd);  // parent doesn't need this
	}

	return 0;
}


int main(void)
{
 //execute phase1 when start up
  if((phase1())!=0){
          printf("Phase1 execute unsuccessful!");
          exit(1);
  }
  
 //execute phase 3
  if((phase3())!=0){
          printf("Phase2 execute unsuccessful!");
          exit(1);
  }
  
  return(0);
}