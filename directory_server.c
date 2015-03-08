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

#define MYPORT_P1 "21992"	// the static port file server will connect to in phase 1
#define MYPORT_P2 "31992"	// the static port file server will connect to in phase 2

#define MAXBUFLEN 100

char DS_HN[128];
char DS_IP[100];

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
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

static int phase1(void)   //phase1
{
	FILE *fp; 
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	char directory_txt[3][MAXBUFLEN]; //the two dimension array used to store the directory msg from file servers
	int flag=0;
	int i;
	int txt_exist;
	int rv;
	int numbytes;
	struct sockaddr_storage their_addr;
	struct sockaddr_in dsaddr;
	char buf[MAXBUFLEN];
	char ch;
	socklen_t addr_len;
	char s[INET6_ADDRSTRLEN];

	gethostname(DS_HN, sizeof DS_HN);
	hostname_to_ip(DS_HN, DS_IP);

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(DS_HN, MYPORT_P1, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}


	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("listener: socket");
			continue;
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("listener: bind");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "listener: failed to bind socket\n");
		return 2;
	}

	freeaddrinfo(servinfo);

	socklen_t dsaddr_len = sizeof (dsaddr);

	getsockname(sockfd,(struct sockaddr *)&dsaddr,&dsaddr_len); //get the port number and IP add

	printf("Phase 1: The Directory Server has UDP port number %d and IP address %s.\n",ntohs(dsaddr.sin_port), inet_ntoa(dsaddr.sin_addr));

 //using 'while' to receive registration information from 3 file servers  
 
 //check whether directory.txt already exists
 if (access("/home/scf-12/yixinz/directory.txt",0)==0){
    remove("/home/scf-12/yixinz/directory.txt");
 }
 else{
 }
 
 addr_len = sizeof their_addr;

 while(directory_txt[2][0]!='F'){
  if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
		(struct sockaddr *)&their_addr, &addr_len)) == -1) {
		perror("recvfrom");
		exit(1);
	}

  printf("Phase 1: The Directory Srver has received request from File Server %c.\n", buf[11]);
	buf[numbytes] = '\n';
 	buf[numbytes+1] = '\0';
 
  for(i=0;i<MAXBUFLEN;i++){
  directory_txt[flag][i]=buf[i];
  }
  flag++;
  
 
 //create the file directory.txt and write the received msg into it
   if((fp=fopen("/home/scf-12/yixinz/directory.txt","a+"))==NULL)
   {
      printf("\nConnot open file directory.txt!");
  getchar();
  exit(1);
   }
   fputs(buf,fp);
   rewind(fp);
   fclose(fp);
}   
 
	close(sockfd);
	printf("Phase 1: The directory.txt file has been created. \n");
	printf("Phase 1: End of Phase 1 for the Directory Server.\n");

	return 0;
}

char* response(char *client)  //Directory server's response to client--find the best fileserver
{
	FILE *resource,*topology;
	char line[100];
	int flag,i,j,a;
	int comp[3]={100000,100000,100000};
	const char *d;
	char *p;
	int rsc[3][2]={0};
	int topo[2][3]={0};
	char *fs;
	char str[100]=" ";
	d=" ";
	i=j=0;
    topology=fopen("/home/scf-12/yixinz/topology.txt","r");
   if(topology!=NULL)
   {
	   while((fgets(line,100,topology))!=NULL)
	   {
		   if(line==" ")
			   fgets(line,100,topology);
		   if(line!=" ")
		   {
			  strcpy(str,line);
			  p=strtok(str,d);
			while(p!=NULL)
			 {
				   topo[i][j]=atoi(p);
				   j++;
				   p=strtok(NULL,d);
			 }
		   }
		   i=i+1;
		   j=0;
	   }
	}
   else printf("\nConnot open file topology.txt!");

   if((resource=fopen("/home/scf-12/yixinz/resource.txt","r"))!=NULL)
   {
	   while((fgets(line,100,resource))!=NULL)
	   {
		   if(line==" ")
			   fgets(line,100,resource); 
		   if(line!=" ")
		   {
			  strcpy(str,line);
			  p=strtok(str,d);
			  while(p!=NULL)
			   {
				   if(0==strcmp(p,"File_Server1"))
					   flag=0;
				   if(0==strcmp(p,"File_Server2"))
					   flag=1;
				   if(0==strcmp(p,"File_Server3"))
					   flag=2;
				   if((0==strcmp(p,"doc1"))||(0==strcmp(p,"doc1\n")))
				       rsc[flag][0]=1;  
				   if((0==strcmp(p,"doc2"))||(0==strcmp(p,"doc2\n")))
					   rsc[flag][1]=1;

				   p=strtok(NULL,d);
			   }
		   }
	   }
   }
   else printf("\nConnot open file resource.txt!");

 
   if(0==strcmp(client,"Client1 doc1"))
	   j=0;
   else if(0==strcmp(client,"Client2 doc2"))
	   j=1;
   for(i=0;i<=2;i++)
   {
	   if(rsc[i][j]==1)
		   comp[i]=topo[j][i];
   }

   	if((comp[0]<comp[1])&&(comp[0]<comp[2]))
		fs="File_server1 41992";
	if((comp[1]<comp[0])&&(comp[1]<comp[2]))
	    fs="File_server2 42992";    
	if((comp[2]<comp[0])&&(comp[2]<comp[1]))
		fs="File_server3 43992";

	return fs;
}


static int phase2(void)  //phase2
{
	int sockfds;
	struct addrinfo hints, *servinfo, *ps;
	struct sockaddr_storage their_addr;  //connector's address information
	int rv1;
	int no_rsc;
	int numbytes;
	int yes=1;
	char buf[MAXBUFLEN];
	char *FS=NULL;
	socklen_t addr_len;
	char s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv1 = getaddrinfo(DS_HN, MYPORT_P2, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv1));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(ps = servinfo; ps != NULL; ps = ps->ai_next) {
		if ((sockfds = socket(ps->ai_family, ps->ai_socktype,
				ps->ai_protocol)) == -1) {
			perror("listener: socket at server side");
			continue;
		}

		if (bind(sockfds, ps->ai_addr, ps->ai_addrlen) == -1) {
			close(sockfds);
			perror("listener: bind at server side");
			continue;
		}

		break;
	}
    
	freeaddrinfo(servinfo);

	if (ps == NULL) {
		fprintf(stderr, "listener: failed to bind socket serverinfo\n");
		return 2;
	}

	struct sockaddr_in dsaddr;
	socklen_t dsaddr_len = sizeof(dsaddr);

	getsockname(sockfds,(struct sockaddr *)&dsaddr,&dsaddr_len); //get the port number and IP add

	printf("Phase 2: The Directory Server has UDP port number %d and IP address %s.\n",ntohs(dsaddr.sin_port), inet_ntoa(dsaddr.sin_addr));

	addr_len = sizeof their_addr;
 
 
 while(1){         //main recvfrom loop
  if ((numbytes = recvfrom(sockfds, buf, MAXBUFLEN-1 , 0,
		(struct sockaddr *)&their_addr, &addr_len)) == -1) {
		perror("recvfrom");
		exit(1);
	}

  printf("Phase 2: The Directory Srver has received request from Client %c.\n", buf[6]);

 	buf[numbytes] = '\0';
	FS=response(buf);  //find the right fileserver for each request

	//send response to client
	if ((numbytes = sendto(sockfds, FS, strlen(FS), 0, (struct sockaddr *)&their_addr, addr_len)) == -1)   {
		perror("talker: sendto");
		exit(1);
		}    
	printf("Phase 2: File server details has been sent to Client %c.\n", buf[6]);
	printf("Phase 2: End of Phase 2 for the Directory Server.\n");
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
  
 //execute phase 2
  if((phase2())!=0){
          printf("Phase2 execute unsuccessful!");
          exit(1);
  }
  
  return(0);
}