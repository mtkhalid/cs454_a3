#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <cstring>
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

struct ServerFunction{
	char* name;
	int* argTypes;
	skeleton skel;
};

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
int listenerPort;
char serverName[1000];

int newfd;        // newly accept()ed socket descriptor
struct sockaddr_storage remoteaddr; // client address

struct addrinfo hints, *servinfo, *p;
int rv;
char s[INET6_ADDRSTRLEN];

struct cmp_str
{
   bool operator()(char const *a, char const *b)
   {
      return std::strcmp(a, b) < 0;
   }
};

map<char*, ServerFunction, cmp_str> functionDB;
vector<pthread_t> serverThreads;

//////////////////////////////
//		Helper Functions	//
//////////////////////////////

//Forward Declarations

char* moveArgsToBuffer(int args_size, void **args, int * argTypes);
void extractArgsFromBuffer(char* sendbuffer, void** args, int* argTypes);


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa){
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int calcArgsSizeToAllocate(int* argTypes) {
	int count =0;
	
	for(int i=0; argTypes[i]!=0; i++){
	
		int argSize =0;
		int numElements = (argTypes[i]  >> 0) & 0xff;
		int datatype = (argTypes[i]  >> 16) & 0xff;
		
		//A for loop that processes all the argTypes. If it equals 0, it means you reached the end
        switch(datatype){
            case ARG_CHAR:
			{
				argSize = sizeof(char);
				
				if(numElements > 0) {
					argSize *= ((argTypes[i] >> 0) & 0xff);
				}
		
                break;
			}
            case ARG_SHORT:
			{
				argSize = sizeof(short);
				
				if(numElements > 0) {
					argSize *= ((argTypes[i] >> 0) & 0xff);
				}
				
                break;
			}
            case ARG_INT:
			{
				argSize = sizeof(int);
				
				if(numElements > 0) {
					argSize *= ((argTypes[i] >> 0) & 0xff);
				}
				
                break;
			}
            case ARG_LONG:
			{
				argSize = sizeof(long);
				
				if(numElements > 0) {
					argSize *= ((argTypes[i] >> 0) & 0xff);
				}

                break;
			}
            case ARG_DOUBLE:
			{
				argSize = sizeof(double);
				
				if(numElements > 0) {
					argSize *= ((argTypes[i] >> 0) & 0xff);
				}
				
                break;
			}
            case ARG_FLOAT:
			{
				argSize = sizeof(float);
				
				if(numElements > 0) {
					argSize *= ((argTypes[i] >> 0) & 0xff);
				}
			
				break;
			}
        }
		
		count += argSize;
    }
	return count;
}

void* execute(void * id) {
	
	cout << "Receiving Execute Message" << endl;
	
	int* requestThreadID = (int*) id;
	int socketfd = *requestThreadID;
	
	RPC_MSG executeMsg;
	read(socketfd, &executeMsg, sizeof(executeMsg));
	
	if(executeMsg.type != EXECUTE) {
		//cerr << "Fatal error << endl;
		exit(1);
	}
	
	//Receive and process the execute message
	char recvbuf[executeMsg.length];
	recv(socketfd, recvbuf, executeMsg	.length, 0);
	char *msg = recvbuf;
	
	int functionNameLength2, argLength2;	

	memcpy(&functionNameLength2, msg,  sizeof(int));
	msg+=sizeof(int);
	
	char *functionName = (char*) malloc(functionNameLength2);
	
	memcpy(functionName, msg, functionNameLength2);
	msg+=functionNameLength2;
	
	cout<<"Received exec msg for function:  "<< functionName <<endl;
	
	memcpy(&argLength2, msg,  sizeof(int));
	msg+=sizeof(int);
	
	int *argTypes = (int *) malloc(argLength2);
	
	memcpy(argTypes, msg, argLength2);
	msg+=argLength2;

	char *argsHeader = (char*) malloc(sizeof(char) * calcArgsSizeToAllocate(argTypes));
	recv(socketfd, argsHeader, calcArgsSizeToAllocate(argTypes), 0);
	
	void **args = (void **) malloc(calcArgsSizeToAllocate(argTypes));
	extractArgsFromBuffer(argsHeader, args, argTypes);

	cout<<"Searching for Function"<<endl;

	map<char *,ServerFunction>::iterator it;
	
	cout << "LOOKING FOR " << functionName << " IN A DB OF SIZE " << functionDB.size();
	it=functionDB.find(functionName);
	
	//TODO: Locate the proper function
	bool found = (it != functionDB.end());
	bool argsMatch = false;

	if(found) {
	
		// Check if the argTypes are the same
		int index = 0;
		while ((it->second).argTypes[index]!=0 && argTypes[index]!=0) {
			if ((it->second).argTypes[index]!=argTypes[index]) {
					break;
			}
			index++;
		}
		
		//Matched so far. Check that both of them have no remaining argTypes
		if((it->second).argTypes[index]!=0 || argTypes[index]!=0 ) {
			argsMatch = false;
		}else{
			argsMatch = true;
		}
		
		if(argsMatch){
			cout <<"Function has been found " << (it->second).name <<endl;
			skeleton execSkel = (it->second).skel;
			
			//Execute the skeleton of the function
			int result = (*execSkel)(argTypes, args);
	
			//Prepare to send the result back.
			
			//Use the argLength and functionNameLength that we found before
			int msgLength = sizeof(int) + functionNameLength2 + sizeof(int) + argLength2;
			
			RPC_MSG msg;
			msg.type = EXECUTE_SUCCESS;
			msg.length = msgLength;
			
			write(socketfd, &msg, sizeof(msg));
			
			//Prepare the data to send
			char* response_header = (char*)malloc(msgLength);
			char* message = response_header;
			
			memcpy(message, &functionNameLength2, sizeof(int));
			message+=sizeof(int);
			memcpy(message, functionName, functionNameLength2);
			message+=functionNameLength2;
			
			memcpy(message, &argLength2, sizeof(int));
			message+=sizeof(int);
			memcpy(message, argTypes, argLength2);
			message+=sizeof(calcArgsSizeToAllocate(argTypes));
			
			send(socketfd, response_header, msgLength, 0);
			char *argsHeader = (char*) malloc(sizeof(char) * calcArgsSizeToAllocate(argTypes));
			
			//Prepare the args results to send back
			char* argsBuffer = moveArgsToBuffer(calcArgsSizeToAllocate(argTypes), args, argTypes); 
	
			send(socketfd, argsBuffer, calcArgsSizeToAllocate(argTypes), 0);
		}else{
			cout<<"Function could not be found"<<endl;
			
			//Server Function Not Found
			int reasonCode = -1;
			
			RPC_MSG msg;
			msg.type = EXECUTE_FAILURE;
			msg.length = reasonCode;
			
			write(socketfd, &msg, sizeof(msg));		
		}
	}
	
	else {
		cout<<"Function could not be found"<<endl;
		
		//Server Function Not Found
		int reasonCode = -1;
		
		RPC_MSG msg;
		msg.type = EXECUTE_FAILURE;
		msg.length = reasonCode;
		
		write(socketfd, &msg, sizeof(msg));	
	}
	
}

/*
// Determines appropriate action based on message type
void* handleExecute(void * id) {

	int* requestThreadID = (int*) id;
	int socket = *requestThreadID;
	
	cout<<"socket is"<<socket<<endl;
	
	RPC_MSG msg;
	read(socket, &msg, sizeof(msg));
	
	if(msg.type == EXECUTE) {
		cout<<"Client has sent an execute message"<<endl;
		execute(msg.length, socket);
	}
	
		
}
*/

// Prints binder address and port
void printBinderInfo(char* binder_address, char* binder_port){
	
	cout << "BINDER_ADDRESS is" << binder_address << endl;
	cout << "BINDER_PORT is" << binder_port << endl;
}

// Prints server address and port
void printServerInfo(char* serverName, int listenerPort){
	cout<<"SERVER ADDRESS is "<<serverName<<endl;
	cout<<"SERVER PORT is "<<listenerPort<<endl;
}


char* moveArgsToBuffer(int args_size, void **args, int * argTypes){

	char *header = (char*) malloc(sizeof(char) * args_size);
	char* sendbuffer = header;
	int i=0;
	int type_size;
	int total_size = 0;
	while(argTypes[i]!=0) {
		int size = 0;

		int type = argTypes[i];
		
		int arraysize = (type >> 0) & 0xff;
		int datatype = (type >> 16) & 0xff;
		
		
		if(datatype == ARG_CHAR) {
			if(arraysize >0) {
				char *arg = (char*)malloc(arraysize * sizeof(char));
				arg = (char*)(args[i]);
				size = arraysize * sizeof(char);
				memcpy(sendbuffer, arg, size);
			}
			else if(arraysize == 0) {
				char *arg = (char*) args[i];
				size = sizeof(char);
				memcpy(sendbuffer, arg, size);
			}
			else {
				cout<<"Warning: Size less than 0"<<endl;
			}	
			
		}
		
		if(datatype == ARG_CHAR) {
			if(arraysize >0) {
				char *arg = (char*)malloc(arraysize * sizeof(char));
				size = arraysize * sizeof(char);
				memcpy(arg, sendbuffer, size);
				args[i] = (void*) arg;
				
			}
			else if(arraysize == 0) {
				char *arg = (char*) malloc(sizeof(char));
				size = sizeof(char);
				memcpy(arg, sendbuffer, size);
				args[i] = (void*) arg;
			}
			else {
				cout<<"Warning: Size less than 0"<<endl;
			}	
			
		}
		else if(datatype == ARG_SHORT) {
	     		if(arraysize >0) {
                                short *arg = (short*)malloc(arraysize * sizeof(short));
                                arg = (short*)(args[i]);
				size = arraysize * sizeof(short);
                                memcpy(sendbuffer, arg, size);
                        }
                        else if(arraysize == 0) {
                                short *arg = (short*) args[i];
				size = sizeof(short);
                                memcpy(sendbuffer, arg, size);
                        }
                        else {
                                cout<<"Warning: Size less than 0"<<endl;
                        }	
		}
		else if(datatype == ARG_INT) {
			if(arraysize >0) {
				
                                int *arg = (int*)malloc(arraysize * sizeof(int));
                                arg = (int*)(args[i]);
				size = arraysize * sizeof(int);
                                memcpy(sendbuffer, arg, size);
                        }
                        else if(arraysize == 0) {
                                int *arg = (int*) args[i];
                        	size = sizeof(int);
                                memcpy(sendbuffer, arg, size);
			}
                        else {
                                cout<<"Warning: Size less than 0"<<endl;
                        }
		}
		else if(datatype == ARG_LONG) {
		        if(arraysize >0) {
                                long *arg = (long*)malloc(arraysize * sizeof(long));
                                arg = (long*)(args[i]);
				size = arraysize * sizeof(long);
                                memcpy(sendbuffer, arg, size);
                        }
                        else if(arraysize == 0) {
                                long *arg = (long*) args[i];
				size = sizeof(long);
                                memcpy(sendbuffer, arg, size);
                        }
                        else {
                                cout<<"Warning: Size less than 0"<<endl;
                        }
		}
		else if(datatype == ARG_DOUBLE) {
			if(arraysize >0) {
                                double *arg = (double*)malloc(arraysize * sizeof(double));
                                arg = (double*)(args[i]);
				size = arraysize * sizeof(double);
                                memcpy(sendbuffer, arg, size);
                        }
                        else if(arraysize == 0) {
                                double *arg = (double*) args[i];
                        	size = sizeof(double);
                                memcpy(sendbuffer, arg, size);
			}
                        else {
                                cout<<"Warning: Size less than 0"<<endl;
                        }
		}
		else if(datatype == ARG_FLOAT) {
			if(arraysize >0) {
                                float *arg = (float*)malloc(arraysize * sizeof(float));
                                arg = (float*)(args[i]);
				size = arraysize * sizeof(float);
                                memcpy(sendbuffer, arg, size);
                        }
                        else if(arraysize == 0) {
                                float *arg = (float*) args[i];
				size = sizeof(float);
                                memcpy(sendbuffer, arg, size);
                        }
                        else {
                                cout<<"Warning: Size less than 0"<<endl;
                        }
		}
		else {
			cout<<"Unknown Type"<<endl;
			cout<<"MEEEEEEEESSSSSSSSSSSEEEEEEEEEED UP"<<endl;
		}
		sendbuffer+=size;
		cout<<size<<endl;
		total_size+=size;
		i++;
	}
	return header;
}

void extractArgsFromBuffer(char* sendbuffer, void** args, int* argTypes){

	int i=0;
	int type_size;
	int total_size = 0;
	while(argTypes[i]!=0) {
		int size = 0;

		int type = argTypes[i];
		
		int arraysize = (type >> 0) & 0xff;
		int datatype = (type >> 16) & 0xff;
		if(datatype == ARG_CHAR) {
			if(arraysize >0) {
				char *arg = (char*)malloc(arraysize * sizeof(char));
				size = arraysize * sizeof(char);
				memcpy(arg, sendbuffer, size);
				args[i] = (void*) arg;
				
			}
			else if(arraysize == 0) {
				char *arg = (char*) malloc(sizeof(char));
				size = sizeof(char);
				memcpy(arg, sendbuffer, size);
				args[i] = (void*) arg;
			}
			else {
				cout<<"Warning: Size less than 0"<<endl;
			}	
			
		}
		else if(datatype == ARG_SHORT) {
	     		if(arraysize >0) {
                                short *arg = (short*)malloc(arraysize * sizeof(short));
				size = arraysize * sizeof(short);
                                memcpy(arg, sendbuffer, size);
				args[i] = (void*)arg;
                        }
                        else if(arraysize == 0) {
                                short *arg = (short*)malloc (sizeof(short));
				size = sizeof(short);
                                memcpy(arg, sendbuffer, size);
				args[i] = (void*)arg;
                        }
                        else {
                                cout<<"Warning: Size less than 0"<<endl;
                        }	
		}
		else if(datatype == ARG_INT) {
			if(arraysize >0) {
                                int *arg = (int*)malloc(arraysize * sizeof(int));
				size = arraysize * sizeof(int);
                                memcpy(arg,sendbuffer, size);
				args[i] = (void*)arg;
                        }
                        else if(arraysize == 0) {
                                int *arg = (int*) malloc(sizeof(int));
                        	size = sizeof(int);
                                memcpy(arg, sendbuffer, size);
				args[i] = (void*) arg;
			}
                        else {
                                cout<<"Warning: Size less than 0"<<endl;
                        }
		}
		else if(datatype == ARG_LONG) {
		        if(arraysize >0) {
                                long *arg = (long*)malloc(arraysize * sizeof(long));
				size = arraysize * sizeof(long);
                                memcpy (arg, sendbuffer, size);
				args[i] = (void*) arg;
				
                        }
                        else if(arraysize == 0) {
                                long *arg = (long*) malloc(sizeof(long));
				size = sizeof(long);
                                memcpy(arg, sendbuffer, size);
				args[i] = (void*)arg;
                        }
                        else {
                                cout<<"Warning: Size less than 0"<<endl;
                        }
		}
		else if(datatype == ARG_DOUBLE) {
			if(arraysize >0) {
                                double *arg = (double*)malloc(arraysize * sizeof(double));
				size = arraysize * sizeof(double);
                                memcpy(arg, sendbuffer, size);
				args[i] = (void*) arg;
                        }
                        else if(arraysize == 0) {
                                double *arg = (double*)malloc(sizeof(double));
                        	size = sizeof(double);
                                memcpy(arg, sendbuffer, size);
				args[i] = (void*)arg;
			}
                        else {
                                cout<<"Warning: Size less than 0"<<endl;
                        }
		}
		else if(datatype == ARG_FLOAT) {
			if(arraysize >0) {
                                float *arg = (float*)malloc(arraysize * sizeof(float));
				size = arraysize * sizeof(float);
                                memcpy(arg, sendbuffer, size);
				args[i] = (void*)arg;
                        }
                        else if(arraysize == 0) {
                                float *arg = (float*) malloc(sizeof(float));
				size = sizeof(float);
				memcpy(arg, sendbuffer, size);
				args[i] = (void*) arg;
                        }
                        else {
                                cout<<"Warning: Size less than 0"<<endl;
                        }
		}
		else {
			cout<<"Unknown Type"<<endl;
		}	
		sendbuffer+=size;
		cout<<size<<endl;
		total_size+=size;
		i++;
	}

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
	char server_port[10];
    int result = snprintf(server_port, 10, "%d", port);

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
			close(sockfd);;
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


///////////////////////
//	rpcCall helpers  //
///////////////////////
 
void sendLocReq(char* name, int* argTypes, int &functionNameLength, int &numArgs, int &argLength){

//Connect to Binder 
	clientBinder = connectToBinder();
	
	//Calculate length of hostname
	functionNameLength =(strlen(name)) * sizeof(char) + 1;

	//Calculate length needed for total number of arguments	
	while(true) {
		if(argTypes[numArgs]==0) {
			break;
		}
		
		numArgs++;
	}
	
    numArgs++;
	
	argLength =(numArgs) * sizeof(int);
	
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

}

void sendExecReq(char* name, int* argTypes, void** args, int &functionNameLength, int &argLength, int &argsSizeToAllocate, int serverfd){
	
	cout << "Starting to send Exec request. FunctionNameLength is of size " << functionNameLength << " and argLength is of size " << argLength << " and argsSize is of size " << argsSizeToAllocate << endl; 
	//Calculate length needed for total number of arguments
	int numArgs = 0;
	
	while(true) {
		if(argTypes[numArgs]==0) {
			break;
		}
		
		numArgs++;
	}
	
	argsSizeToAllocate = (numArgs) * sizeof(void *);
	
	int msgLength = sizeof(int) + functionNameLength + sizeof(int) + argLength + sizeof(int) +  argsSizeToAllocate;
	
	
    RPC_MSG exec;
    exec.type = EXECUTE;
	exec.length = msgLength;
    
	write(serverfd, &exec, sizeof(exec));
	
	char params[msgLength];
    char *sendbuf2 = params;
	
	memcpy(sendbuf2, &functionNameLength, sizeof(int));
    sendbuf2+=sizeof(int);
    memcpy(sendbuf2, name, functionNameLength);
	sendbuf2+=functionNameLength;
	
	memcpy(sendbuf2, &argLength, sizeof(int));
	sendbuf2+=sizeof(int);
	memcpy(sendbuf2, argTypes, argLength);
	sendbuf2+=sizeof(argLength);
	
	char* arg_buffer = moveArgsToBuffer(calcArgsSizeToAllocate(argTypes), args, argTypes);	
	
	cout<<"Size sent is"<<msgLength;
	send(serverfd, params, msgLength, 0);
	send(serverfd, arg_buffer, calcArgsSizeToAllocate(argTypes), 0); 
}


//////////////////////////
//	core rpc functions  //
//////////////////////////

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
	gethostname(serverName, 1000);           /* who are we? */
	hp= gethostbyname(serverName);                  /* get our address info */
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

	listenerPort = ntohs(my_addr.sin_port);

	printServerInfo(serverName, listenerPort);

	serverBinder = connectToBinder();
	FD_SET(serverBinder, &master);

	return 0;
}

int rpcCall(char* name, int* argTypes, void** args){
	//Connect to Binder 
	
	int functionNameLength =0;
	int argLength=0;
	int numArgs=0;
	int argsSizeToAllocate = 0;
	
	sendLocReq(name, argTypes, functionNameLength, numArgs, argLength);

	RPC_MSG msg_response;
    read(clientBinder, &msg_response, sizeof(msg_response));
	
	if(msg_response.type == LOC_FAILURE) {
		return -100;
	}
	
	//Otherwise it returned LOC_SUCCESS
	
	//Found a server to execute the function - Process message to find out which one

	char recvbuf[msg_response.length];
	recv(clientBinder, recvbuf, msg_response.length, 0);
	
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
	
	//Connect to the server that the binder specified
	int serverfd = connectToServer(hostname, portNum);

	//Send Execute Request Call
	sendExecReq(name, argTypes, args, functionNameLength, argLength, argsSizeToAllocate, serverfd);
	
	//Parse Response
	RPC_MSG execResponse;
	read(serverfd, &execResponse, sizeof(execResponse));

	if(execResponse.type == EXECUTE_SUCCESS){
		cout<<"Server SUCCESSFULLY executed the function!"<<endl;
		
		char recvbuf[execResponse.length];
        recv(serverfd, recvbuf, execResponse.length, 0);
		char *msg = recvbuf;
		
		int responseFunctionNameLength =0;
		int responseArgLength=0;
		int responseArgsSizeToAllocate=0;
	
		//Extract the function name from the response
		memcpy(&responseFunctionNameLength, msg,  sizeof(int));
		msg+=sizeof(int);
		
		char *responseFunctionName = (char*) malloc(responseFunctionNameLength);
		
		memcpy(responseFunctionName, msg, responseFunctionNameLength);
		msg+=responseFunctionNameLength;
		
		cout<<"Name of method is "<<responseFunctionName<<endl;
		
		//Extract the argTypes from the response
		memcpy(&responseArgLength, msg,  sizeof(int));
		msg+=sizeof(int);
		
		int *responseArgTypes = (int *) malloc(responseArgLength);
		
		memcpy(responseArgTypes, msg, responseArgLength);
		msg+=responseArgLength;
		
		cout<<"Length of argTypes is "<<responseArgLength<<endl;

		//Extract the arg values from the response
		char* args_buffer = (char*) malloc(calcArgsSizeToAllocate(argTypes) * sizeof(char));
		recv(serverfd, args_buffer, calcArgsSizeToAllocate(argTypes), 0);
		extractArgsFromBuffer(args_buffer, args, argTypes);
	}else if(execResponse.type == EXECUTE_FAILURE) {
		cout<<"Server could not Execute requested function."<<endl;
		
		int reasonCode = 0;
		read(serverfd, &reasonCode, sizeof(int));
		if(reasonCode == -100) {
			cout<<"Server could not locate function"<<endl;
		}
		else {
			cout<<"Generic Error code";
		}
	
	}
	close(clientBinder);
}

int rpcCacheCall(char* name, int* argTypes, void** args){

}

int rpcRegister(char* funcName, int* argTypes, skeleton f){

	//Add to database
	
	ServerFunction newFunction;
	
	newFunction.name = funcName;
	newFunction.argTypes = argTypes;
	newFunction.skel = f;

	functionDB.insert(pair<char *,ServerFunction>(funcName, newFunction));

	//Calculate length of hostname
	int hostnameLength = (strlen(serverName))*sizeof(char);
	
	//Add 1 to account for the terminal char
	hostnameLength++;
	
	//Calculate length of hostname
	int portLength = listenerPort;
	
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
	memcpy(sendbuf, serverName, hostnameLength);
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
	
	cout<<"RPC Execute Called"<<endl;

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
					pthread_create(&t, NULL, &execute, threadData);
					
					serverThreads.push_back(t);
					
					FD_CLR(i, &master);
					
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END while loop--and you thought it would never end!

	cout << "We terminated the server " << endl;

	for(int i = 0; i < serverThreads.size(); i++) {
		pthread_join(serverThreads[i], NULL);
	}
	
	cout<<"Server is shutting down. Bye!"<<endl;

}

int rpcTerminate(){
	cout << "Terminating Message Received! " << endl;
	clientBinder = connectToBinder();
	RPC_MSG msg;
    msg.type = TERMINATE;
	
	//Send terminate message to the binder who will then inform the client
	write(clientBinder, &msg, sizeof(msg));
}
