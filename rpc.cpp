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

int serverBinder;
int clientBinder;
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

//////////////////////////////
//		Helper Functions	//
//////////////////////////////

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa){
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// Determines appropriate action based on message type
void* parseRequestForServer(void * id) {
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

// Prints binder address and port
void printBinderInfo(char* binder_address, char* binder_port){
	
	cout << "BINDER_ADDRESS is" << binder_address << endl;
	cout << "BINDER_PORT is" << binder_port << endl;
}

// Prints server address and port
void printServerInfo(char* myname, int listening_port){
	cout<<"SERVER ADDRESS is "<<myname<<endl;
	cout<<"SERVER PORT is "<<listening_port<<endl;
}

char* moveArgsToBuffer(int numArgs, int * argTypes, void **args) {

	// create a buffer (malloc) to hold all the args

	//A for loop that processes all the argTypes. If it equals 0, it means you reached the end
	for(int i=0; argTypes[i]!=0; i++){
	
		//Handle all the different types for each argType
		if(type == ARG_CHAR) {
			//Check if your copying a single data type or an array of data types
			//Allocate enough space on the buffer
			//Move the data into the buffer
		}else if(type == ARG_SHORT) {
		
		}else if(type == ARG_INT) {
			
		}else if(datatype == ARG_LONG) {
		
		}
		else if(datatype == ARG_DOUBLE) {
		
		}
		else if(datatype == ARG_FLOAT) {
		
		}
	}

	// return newly created buffer containing all the args;
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

// Connect to Server - Very similar to how we connect to the binder above!
int connectToServer(char *hostname, int port){
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	//Only difference btwn connecting to server vs binder is this:
	char server_port[5];
    int res = snprintf(server_port, 5, "%d", port);

    if ((rv = getaddrinfo(hostname, server_port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "Can't Get Address info for Server: %s\n", gai_strerror(rv));
        return 1;
    }
	
	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("Client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("Client: connect");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "Client: failed to connect\n");
		return -2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("Client: connecting to %s\n", s);

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

	printServerInfo(myname, listening_port);

	serverBinder = connectToBinder();
	FD_SET(serverBinder, &master);

	return 0;
}

int rpcCall(char* name, int* argTypes, void** args){
	
	//Connect to Binder 
	clientBinder = connectToBinder();
	
	//Calculate length of hostname
	int functionNameLength =(strlen(name)) * sizeof(char) + 1;

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
	int msgLength = sizeof(int) + functionNameLength + sizeof(int) + argLength;
	
	//Now that we know the length, we can create the message to send
	RPC_MSG msg;
	msg.type = LOC_REQUEST;
	msg.length = msgLength;
	
	write(clientBinder, &msg, sizeof(msg));

	char header[msgLength];
	char* sendbuf = header;
	
	memcpy(sendbuf, &functionNameLength, sizeof(int));
	sendbuf+=sizeof(int);
	memcpy(sendbuf, name, functionNameLength);
	sendbuf+=functionNameLength;
	
	memcpy(sendbuf, &argLength, sizeof(int));
	sendbuf+=sizeof(int);
	memcpy(sendbuf, argTypes, argLength);
	sendbuf+=sizeof(argLength);

	send(clientBinder, header, msgLength, 0);
	
	//Wait for the binder to send back a response indicating which server will serve the function request
	
	RPC_MSG msg_response;
    read(clientBinder, &msg_response, sizeof(msg_response));
	
	if(msg_response.type == LOC_FAILURE) {
		return -1;
	}
	
	//Otherwise it returned LOC_SUCCESS
	
	//Found a server to execute the function - Process message to find out which one
	
	int responseMsgLength = msg_response.length;

	char recvbuf[msgLength];
	recv(clientBinder, recvbuf, msgLength, 0);
	
	char *responseMsg = recvbuf;
	
	int hostnameLength;
	//Extract the length of the hostname
	memcpy(&hostnameLength, responseMsg, sizeof(int));
	responseMsg+=sizeof(int);
	
	//Allocatee enough memory to store the hostname
	char *hostname = (char*) malloc(hostnameLength);
	
	//Extract the hostname
	memcpy(hostname, responseMsg, hostnameLength);
	responseMsg+=hostnameLength;

	int portNum;
	
	memcpy(&portNum, responseMsg,  sizeof(int));
	responseMsg+=sizeof(int);
	
	cout <<"The function will be served by Server: " << hostname << " and it can be found on Port No: "<< portNum << endl;
	
	// Found the server name and port no. so now we need to send an EXECUTE msg
	
	// Connect to Server
	int serverfd = connectToServer(hostname, portNum);

	// Note we have already calculated the functionNameLength, numArgs and argLength before. 
	
	// Now we need to calculate the length of the actual values of the args
	int argValueLength = (numArgs - 1) * sizeof(void *);
	
	//int args_size = calc_args_size(argTypes);
	
	//Calculate length of the complete msg
	int msg2Length = sizeof(int) + functionNameLength + sizeof(int) + argLength + sizeof(int) + argLength;
	
	//Now that we know the length, we can create the message to send	
	RPC_MSG msg2;
	msg2.type = EXECUTE;
	msg2.length = msg2Length;
	
	write(serverfd, &msg, sizeof(msg));
	
	char header2[msg2Length];
	char* sendbuf2 = header2;
	
	memcpy(sendbuf2, &functionNameLength, sizeof(int));
	sendbuf2+=sizeof(int);
	memcpy(sendbuf2, name, functionNameLength);
	sendbuf2+=functionNameLength;

	memcpy(sendbuf2, &argLength, sizeof(int));
	sendbuf2+=sizeof(int);
	memcpy(sendbuf2, argTypes, argLength);
	sendbuf2+=sizeof(argLength);
	
	send(serverfd, header2, msg2Length, 0);
	
	///////////////////////////////TODO:
	//char* arg_buffer = append_args(args_size, args, argTypes);
	//send(serverfd, arg_buffer, args_size, 0);
	
	//Wait for the server to send back a response indicating whether the execution was successful or not
	
	RPC_MSG msg2_response;
    read(serverfd, &msg2_response, sizeof(msg2_response));
	
	if(msg2_response.type == EXECUTE_FAILURE) {
		int reasonCode = 0;
		
		//Read the reason code from the response
		read(serverfd, &reasonCode, sizeof(int));
		
		return reasonCode;
	}
	
	//Otherwise it returned EXECUTE_SUCCESS
	
	int msg2ResponseLength = msg2_response.length;
	
	cout<<"Execution successful by Server."<<endl;
	
	char recvbuf2[msg2ResponseLength];
	recv(serverfd, recvbuf2, msg2ResponseLength, 0);
	
	char *responseMsg2 = recvbuf2;
	
	//Must retrieve the returned name, argTypes and args
	
	char* returnedName;
	int* returnedArgTypes;
	void** returnedArgs;
	
	//Extract the length of the function name
	memcpy(&functionNameLength, responseMsg2, sizeof(int));
	responseMsg2+=sizeof(int);
	
	//Allocatee enough memory to store the returned function name
	returnedName = (char*) malloc(functionNameLength);
	
	//Extract the function name
	memcpy(returnedName, responseMsg2, functionNameLength);
	responseMsg2+=functionNameLength;
	
	cout<<"Name of method is "<<name<<endl;
	
	//Extract the length of the returned argTypes
	memcpy(&argLength, responseMsg2, sizeof(int));
	responseMsg2+=sizeof(int);
	
	cout<<"Length of argTypes is "<<argLength<<endl;
	
	// Allocate enough memory to store the returned argTypes
	returnedArgTypes = (int *) malloc(argLength);
	
	// Extract the returned argTypes
	memcpy(returnedArgTypes, responseMsg2, argLength);
	responseMsg2+=sizeof(argLength);
	

	///////////////////////////////TODO:
	
	//char* args_buffer = (char*) malloc(args_size * sizeof(char));
	//recv(serverfd, args_buffer, args_size, 0);
	//copy_args(args_buffer, args, argTypes);
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

	write(serverBinder, &msg, sizeof(msg));
	
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

	send(serverBinder, header, 1000, 0);

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

	                        
				} else if(i == serverBinder) {
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
					pthread_create(&t, NULL, &parseRequestForServer, threadData);
					
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
