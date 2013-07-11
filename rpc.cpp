#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <iostream>
#include <errno.h>
#include <iostream>
#include <vector>
#include <pthread.h>
#include <map>

#include "rpc.h"
#include "RequestMessage.h"

using namespace std;

#define MAXHOSTNAME 2000

int sockfd, numbytes;
struct sockaddr_in my_addr;
socklen_t sl =  sizeof(my_addr);
socklen_t addrlen;

fd_set master;    // master file descriptor list
fd_set read_fds;  // temp file descriptor list for select()
int fdmax;        // maximum file descriptor number

int binderfd;
int binderfd_client;
int listener;
int listening_port;
char   myname[MAXHOSTNAME+1];

int newfd;        // newly accept()ed socket descriptor
struct sockaddr_storage remoteaddr; // client address

struct addrinfo hints, *servinfo, *p;
int rv;
char s[INET6_ADDRSTRLEN];

vector<Function> functionDB;
vector<pthread_t> serverThreads;

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

//////////////////////////////
//		Helper Functions	//
//////////////////////////////

void* parseRequest(void * id) {
	int* requestThreadID = (int*) id;
	int socket = *requestThreadID;
	
	cout<<"socket is"<<socket<<endl;
	
	RPC_MSG msg;
	read(socket, &msg, sizeof(msg));
	
	if(msg.type == EXECUTE) {
		cout<<"EXECUTE MESSAGE RECEIVED"<<endl;
		//execute_function(msg.length, socket);
	}
	else if(msg.type == TERMINATE) {
		cout<<"TERMINATE MESSAGE RECEIVED"<<endl;
	}
	else {
		cout<<"UNKNOWN MESSAGE RECEIVED"<<msg.type<<endl;
	}
}

void printBinderInfo(char* binder_address, char* binder_port){
	
	cout << "Binder Address is" << binder_address << endl;
	cout << "Binder Port is" << binder_port << endl;
}

//	Connect to Binder 	
int connectToBinder(){
	char* binder_address = getenv("BINDER_ADDRESS");
	char* binder_port = getenv("BINDER_PORT");
	
	printBinderInfo(binder_address, binder_port);

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(binder_address, binder_port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "Can't Get Address info for Binder: %s\n", gai_strerror(rv));
		return -1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("Server: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("Server: connect");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "Server: failed to connect\n");
		return -2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("Server: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure
	
	return sockfd;
}


int rpcInit(){
	
	
	struct sockaddr_in sa;
	struct hostent *hp;
	int yes=1;

	//////////////////////////
	//	Create Listener 	//
	//////////////////////////

    FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);

    // get us a socket and bind it

    memset(&sa, 0, sizeof(struct sockaddr_in)); /* clear our address */
	gethostname(myname, MAXHOSTNAME);           /* who are we? */
	hp= gethostbyname(myname);                  /* get our address info */
	if (hp == NULL)                             /* we don't exist !? */
		return(-1);
	sa.sin_family= hp->h_addrtype;              /* this is our host address */
	sa.sin_port= htons(0);                		/* this is our port number */

	listener = socket(AF_INET, SOCK_STREAM, 0);
	if (listener < 0) {
		perror("listener: socket");
		return(-1);  ;
	}

	// lose the pesky "address already in use" error message
	setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

	if (bind(listener,(struct sockaddr *)&sa,sizeof(struct sockaddr_in)) < 0) {
		close(listener);
		perror("listener: bind");
		return(-1);                               /* bind address to socket */
	}

	if(getsockname(listener, (struct sockaddr *) &my_addr, &sl) == -1) {
		cout<<"Can't Get socket name";
		return -1;
	}

	// listen
	if (listen(listener, 10) == -1) {
		perror("listen");
		exit(3);
	}

	listening_port = ntohs(my_addr.sin_port);

	cout<<"SERVER LISTENER HAS BEEN CREATED"<<endl;
	cout<<"SERVER ADDRESS is "<<myname<<endl;
	cout<<"SERVER PORT is "<<listening_port<<endl;

	binderfd = connectToBinder();
	FD_SET(binderfd, &master);

	return 0;
}

int rpcCall(char* name, int* argTypes, void** args){

}

int rpcCacheCall(char* name, int* argTypes, void** args){

}

int rpcRegister(char* funcName, int* argTypes, skeleton f){


	//Add to database

	Function newFunction;
	
	newFunction.name = funcName;
	newFunction.argTypes = argTypes;
	newFunction.skel = f;

	functionDB.push_back(newFunction);

	//Calculate length of hostname
	int hostnameLength = (strlen(myname))*sizeof(char)  + 1;
	
	//Calculate length of hostname
	int portLength = listening_port;
	
	//Calculate length of hostname
	int functionNameLength =(strlen(funcName)) * sizeof(char) + 1;

	//Calculate length needed for total number of arguments
	int numArgs = 0;
	
	while(true) {
		if(argTypes[numArgs]==0) {
			break;
		}
		
		numArgs++;
	}
	
    numArgs++;
	
	int argLength =(numArgs) * sizeof(int);

	//Calculate length of the complete msg
	int msgLength = hostnameLength + portLength + functionNameLength + argLength;

	RPC_MSG msg;
	msg.type = REGISTER;
	msg.length = msgLength;

	write(binderfd, &msg, sizeof(msg));
	
	cout << "Name is " << funcName<<" and num params is "<< argLength<<endl;
	
	char header[1000];
	char *sendbuf = header;
	
	memcpy(sendbuf, &hostnameLength, sizeof(int));
	sendbuf+=sizeof(int);
	memcpy(sendbuf, myname, hostnameLength);
	sendbuf+=hostnameLength;
	
	memcpy(sendbuf, &portLength, sizeof(int));
	sendbuf+=sizeof(int);
	
	memcpy(sendbuf, &functionNameLength, sizeof(int));
	sendbuf+=sizeof(int);
	memcpy(sendbuf, funcName, functionNameLength);
	sendbuf+=functionNameLength;
	
	memcpy(sendbuf, &argLength, sizeof(int));
	sendbuf+=sizeof(int);
	memcpy(sendbuf, argTypes, argLength);
	sendbuf+=sizeof(argLength);

	send(binderfd, header, 1000, 0);

}

int rpcExecute(){

	FD_SET(listener, &master);
	struct sockaddr_storage remoteaddr; // client address
	char remoteIP[INET6_ADDRSTRLEN];
	int newfd, i;
	bool terminateServer = false;

	fdmax = listener; // so far, it's this one

		
	// main loop
	while (!terminateServer) {
		read_fds = master; // copy it
		if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
			perror("select");
			exit(4);
		}
			
		// run through the existing connections looking for data to read
        for(i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) { // we got one!!
                if (i == listener) {
                    // handle new connections
                    addrlen = sizeof remoteaddr;
                    newfd = accept(listener,
                        (struct sockaddr *)&remoteaddr,
                        &addrlen);
						
					if (newfd == -1) {
                        perror("accept");
                    } else {
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax) {    // keep track of the max
                            fdmax = newfd;
                        }
                        printf("selectserver: new connection from %s on "
                            "socket %d\n",
                            inet_ntop(remoteaddr.ss_family,
                                get_in_addr((struct sockaddr*)&remoteaddr),
                                remoteIP, INET6_ADDRSTRLEN),
                            newfd);
                    }

	                        
				} else if(i == binderfd) {
					cout<<"Message Received from Binder"<<endl;
					RPC_MSG msg;
					read(i, &msg, sizeof(msg));
					
					if(msg.type == REGISTER_SUCCESS) {
						cout << "Registration was succesfull!" << endl;
					}else if(msg.type == REGISTER_FAILURE){
						cout << "Registration failed!" << endl;	
					}else if(msg.type == TERMINATE) {
						terminateServer = true;
						break;
					}
				} else {	
					cout<<"Server: Received Data From Client"<<endl;
				
					pthread_t t;
					int* threadData = (int*) malloc(sizeof(int));
					*threadData = i;
					pthread_create(&t, NULL, &parseRequest, threadData);
					
					//serverThreads.push_back(t);
					
					FD_CLR(i, &master);
					
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END while loop--and you thought it would never end!


	//for(int i = 0; i < serverThreads.size(); i++) {
	//	pthread_join(serverThreads[i], NULL);
	//}
	
	cout<<"Server is shutting down. Bye!"<<endl;

}

int rpcTerminate(){


}
