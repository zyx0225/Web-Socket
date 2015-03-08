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

#define MYPORT "33992"	// the port client2 will use to connect directory server
#define SERVERPORT 31992	// the port client2 will be connecting to in phase 1
#define MAXBUFLEN 100

#define DS_HN "nunki.usc.edu"
#define FS_HN "nunki.usc.edu"

char C2_IP[100],DS_IP[100],FS_IP[100];
char C2_HN[128];

char response[MAXBUFLEN]; //response from directory server in phase 2 and will be used in phase 3

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

static int phase2(void)
{
	int sockfdc;
	struct addrinfo hints, *clientinfo, *pc;
 	struct sockaddr_storage their_addr;
	struct sockaddr_in servaddr, clientaddr;
	char buf[MAXBUFLEN];
	char *p,*fs,*port;
	const char *d;
	d=" ";
	socklen_t addr_len;
	char s[INET6_ADDRSTRLEN];
	int rv;
	int numbytes;
	char msg[100]="Client2 doc2"; /*msg send to directory server*/

	gethostname(C2_HN, sizeof C2_HN);
	hostname_to_ip(DS_HN, DS_IP);    //convert hostname to ip address
	hostname_to_ip(C2_HN, C2_IP); 

	memset(&servaddr, 0, sizeof(servaddr));   //load directory server info
	servaddr.sin_family=AF_INET;
	inet_pton(AF_INET,DS_IP, &servaddr.sin_addr.s_addr);
	servaddr.sin_port=htons(SERVERPORT);
	

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
 
	if ((rv= getaddrinfo(C2_HN, MYPORT, &hints, &clientinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	} 
 
 	// loop through all the results, create and bind socket
	for(pc = clientinfo; pc != NULL; pc = pc->ai_next) {
		if ((sockfdc = socket(pc->ai_family, pc->ai_socktype,
				pc->ai_protocol)) == -1) {
			perror("talker: socket at client side");
			continue;
		}

		if (bind(sockfdc, pc->ai_addr, pc->ai_addrlen) == -1) {
			close(sockfdc);
			perror("talker: bind at client side");
			continue;
		}

		break;
	}

	if (pc == NULL) {
		fprintf(stderr, "talker: failed to bind socket clientinfo\n");
		return 2;
	}
	
	freeaddrinfo(clientinfo);

	socklen_t clientaddr_len = sizeof (clientaddr);

	getsockname(sockfdc,(struct sockaddr *)&clientaddr,&clientaddr_len); //get the port number and IP add

	printf("Phase 2: Client 2 has UDP port number %d and IP address %s.\n",ntohs(clientaddr.sin_port),  inet_ntoa(clientaddr.sin_addr));

	//send request to directory server
	if ((numbytes = sendto(sockfdc, msg, strlen(msg), 0, (struct sockaddr *)&servaddr, sizeof(servaddr))) == -1) {
		perror("talker: sendto");
		exit(1);
	}
 	printf("Phase 2: The File request from Client 2 has been sent to the Directory Server.\n", msg);

	//recvfrom directory server
	addr_len = sizeof their_addr;
	if ((numbytes = recvfrom(sockfdc, buf, MAXBUFLEN-1 , 0,
		(struct sockaddr *)&their_addr, &addr_len)) == -1) {
		perror("recvfrom");
		exit(1);
	}

	strcpy(response,buf);  

	p=strtok(buf,d);
	if(p) fs=p;
	p=strtok(NULL,d);
	if(p) port=p;    

	printf("Phase 2: The File requested by Client 2 is present in %s and the File Server's TCP port number is %s.\n",fs,port);
	
  	close(sockfdc);
	
	printf("Phase 2: End of Phase 2 for Client 2.\n");

	return 0;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


static int phase3(void)  //phase 3
{
	int sockfdc,numbytes;  
	char buf[MAXBUFLEN];
	const char *FILESERVER_PORT;
	char *p,*fs;
	char client_TCP_port[1024];
	char client_TCP_host[1024];
	struct addrinfo hints, *clientinfo, *pc;
	struct sockaddr_in servaddr, clientaddr;
	int rv, servaddr_len;
	char s[INET6_ADDRSTRLEN];
	const char *d;
    d=" ";
	
	hostname_to_ip(FS_HN, FS_IP); 
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	p=strtok(response,d);
	if(p) fs=p;
	p=strtok(NULL,d);
	if(p) FILESERVER_PORT=p; 

	memset(&servaddr, 0, sizeof(servaddr));   //load file server info
	servaddr.sin_family=AF_INET;
	inet_pton(AF_INET,FS_IP, &servaddr.sin_addr.s_addr);
	servaddr.sin_port=htons(atoi(FILESERVER_PORT));

	if ((rv = getaddrinfo(C2_HN, NULL, &hints, &clientinfo)) != 0) {   //use dynamic port number
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(pc = clientinfo; pc != NULL; pc = pc->ai_next) {
		if ((sockfdc = socket(pc->ai_family, pc->ai_socktype,
				pc->ai_protocol)) == -1) {
				perror("Phase 3: socket at client side");
			continue;
		}

		if (connect(sockfdc, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
			close(sockfdc);
			perror("Phase 3: client: connect");
			continue;
		}

		break;
	}

	if (pc == NULL) {
		fprintf(stderr, "Phase 3: failed to connect socket\n");
		return 2;
	}
	

	freeaddrinfo(clientinfo); // all done with this structure

	socklen_t clientaddr_len = sizeof(clientaddr);

	getsockname(sockfdc,(struct sockaddr *)&clientaddr,&clientaddr_len); //get the dynamic port number

	printf("Phase 3: Client 2 has dynamic TCP port number %d and IP address %s.\n",ntohs(clientaddr.sin_port), inet_ntoa(clientaddr.sin_addr));

	if (send(sockfdc, "Client2 doc2", 12, 0) == -1)
		perror("Phase 3: send to file server");

	printf("Phase 3: The file request from Client 2 has been sent to the %s.\n", fs);

	if ((numbytes = recv(sockfdc, buf, MAXBUFLEN-1, 0)) == -1) {
	    perror("recv from file server");
	    exit(1);
	}
	buf[numbytes] = '\0';

	printf("Phase 3: Client 2 received %s from %s.\n", buf, fs);

	close(sockfdc);

	printf("Phase 3: End of Phase 3 for Client 2.\n");

	return 0;
}

int main(void)
{
 //execute phase2 when start up
  if((phase2())!=0){
          printf("Phase2 execute unsuccessful!");
          exit(1);
  }
  
 //execute phase 3
  if((phase3())!=0){
          printf("Phase3 execute unsuccessful!");
          exit(1);
  }
  
  return(0);
}