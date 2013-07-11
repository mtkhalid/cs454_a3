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
#include <pthread.h>
#include <map>

#include "rpc.h"
#include "RequestMessage.h"

using namespace std;

#define MAXHOSTNAME 2000

struct sockaddr_in my_addr;
socklen_t sl =  sizeof(my_addr);
socklen_t addrlen;

vector<func> database;

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

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int rpcInit(){
	struct sockaddr_in sa;
	struct hostent *hp;
	int yes=1;
	int sockfd, numbytes;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];


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

	// listen to up to 10 connections
	if (listen(listener, 10) == -1) {
		perror("listen");
		exit(3);
	}

	listening_port = ntohs(my_addr.sin_port);

	cout<<"SERVER LISTENER HAS BEEN CREATED"<<endl;
	cout<<"SERVER ADDRESS is "<<myname<<endl;
	cout<<"SERVER PORT is "<<listening_port<<endl;

	//////////////////////////
	//	Connect to Binder 	//
	//////////////////////////

	char* binder_address = getenv("BINDER_ADDRESS");
	cout << "Binder Address is" << binder_address << endl;

	char* binder_port = getenv("BINDER_PORT");
	cout << "Binder Port is" << binder_port << endl;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(binder_address, binder_port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "Can't Get Address info for Binder: %s\n", gai_strerror(rv));
		return 1;
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
		return -1;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("Server: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure

	FD_SET(sockfd, &master);

	return 0;
}

int rpcCall(char* name, int* argTypes, void** args){

}

int rpcCacheCall(char* name, int* argTypes, void** args){

}

int rpcRegister(char* name, int* argTypes, skeleton f){
	//Add to database

	func new_func;
	new_func.signature.name = name;
	new_func.signature.argTypes = argTypes;
	new_func.f = f;

	database.insert(new_func);

	//Calculate Length of Message
	int hostname = sizeof(char) * (strlen(myname)) + 1;
	int port = listening_port;
	int len_name = sizeof(char) * (strlen(name)) + 1;
	int argnum = calc_argnum(argTypes);
	int arg_length = sizeof(int) * (argnum);

	int content_length = len_name + hostname + port + arg_length;

	rpc_message message;
	message.type = REGISTER;
	message.length = content_length;


	write(binderfd, &message, sizeof(message));
	char header[1000];

	cout<<"Name is "<<name<<" and num params is "<<arg_length<<endl;

	char *sendbuf = header;
	memcpy(sendbuf, &hostname, sizeof(int));
	sendbuf+=sizeof(int);
	memcpy(sendbuf, myname, hostname);
	sendbuf+=hostname;
	memcpy(sendbuf, &port, sizeof(int));
	sendbuf+=sizeof(int);
	memcpy(sendbuf, &len_name, sizeof(int));
	sendbuf+=sizeof(int);
	memcpy(sendbuf, name, len_name);
	sendbuf+=len_name;
	memcpy(sendbuf, &arg_length, sizeof(int));
	sendbuf+=sizeof(int);
	memcpy(sendbuf, argTypes, arg_length);
	sendbuf+=sizeof(arg_length);

	send(binderfd, header, 1000, 0);

	// Done Registering

}

int rpcExecute(){
	   FD_SET(listener, &master);
	   struct sockaddr_storage remoteaddr; // client address
	   char remoteIP[INET6_ADDRSTRLEN];
	   int newfd, i;
	   int terminate = 0;
	    // keep track of the biggest file descriptor
	    fdmax = listener; // so far, it's this one
	    for(;terminate == 0;) {
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
	                }

			else if(i == binderfd) {
				cout<<"Message Received from Binder"<<endl;
				rpc_message message;
				read(binderfd, &message, sizeof(message));
				if(message.type == TERMINATE) {
					terminate = 1;
					break;
				}
			}
			else {
	                     	cout<<"Server: Received Data From Someone"<<endl;
				pthread_t t;
				ThreadData* td = (ThreadData*) malloc(sizeof(ThreadData));

	   			td->id = i;
				pthread_create(&t, NULL, &parse_message,td);

				FD_CLR(i, &master);
				thread_pool.push_back(t);

	                } // END handle data from client
	            } // END got new incoming connection
	        } // END looping through file descriptors
	    } // END for(;;)--and you thought it would never end!


		for(i = 0; i < thread_pool.size(); i++) {
			pthread_join(thread_pool[i], NULL);
		}
		cout<<"Server is shutting down. Bye!"<<endl;

}

int rpcTerminate(){

}

int main(){}
