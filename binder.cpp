#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
#include <pthread.h>
#include <map>
#include <queue>

#include "rpc.h"
#include "binder.h"
#include "RequestMessage.h"

#define MAXHOSTNAME 50

using namespace std;

struct sockaddr_in sa, my_addr;
socklen_t sl =  sizeof(my_addr);
socklen_t addrlen;

struct hostent *hp;
int yes=1;
int sockfd, numbytes;
struct addrinfo hints, *servinfo, *p;
int rv;
char s[INET6_ADDRSTRLEN];

fd_set master;    // master file descriptor list
fd_set read_fds;  // temp file descriptor list for select()
int fdmax;        // maximum file descriptor number

int binderfd; 
int binderfd_client;
int listener;
int listening_port;
char myname[1000];

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void printBinderLocation(){
	std::cout << "BINDER_ADDRESS " << myname << "\n";
	std::cout << "BINDER_PORT " << listening_port << "\n";
}

void handleServerRegRequest (int msgLength, int socket) {
	
	char recvbuf[msgLength + 1];
	recv(socket, recvbuf, 1000, 0);
	char *msg = recvbuf;	
	
	int hostnameLength=0;
	int port=0;
	int nameLength =0;
	int argLength=0;	
	
	//Extract the hostname length
	memcpy(&hostnameLength, msg , sizeof(int));
	msg+=sizeof(int);
	
	//Alloacte enough memory to store the hostname
	char *hostname = (char*) malloc(hostnameLength);
	
	//Extract the hostname
	memcpy(hostname, msg , hostnameLength);
	msg+=hostnameLength;	
	
	cout<<"Hostname is "<<hostname<<endl;
	
	//Extract the port
	memcpy(&port, msg, sizeof(int));
	msg+=sizeof(int);
	cout<<"Port is "<<port<<endl;
	
	//Extract the function name length
	memcpy(&nameLength, msg,  sizeof(int));
	msg+=sizeof(int);
	
	//Alloacte enough memory to store the function name
	char *funcName = (char*) malloc(nameLength); 
	
	//Extract the function name
	memcpy(funcName, msg, nameLength);
	msg+=nameLength;
	
	cout<<"Name of method is "<<funcName<<endl;
	
	//Extract the function name length
	memcpy(&argLength, msg,  sizeof(int));
	msg+=sizeof(int);
	
	//Alloacte enough memory to store the function name
	int *argTypes = (int *) malloc(argLength);
	
	//Extract the function name
	memcpy(argTypes, msg, argLength);
	msg+=argLength;
	
	ServerNode newServer;
	newServer.hostname = hostname;
	newServer.port = port;
	newServer.socket = socket;
	
	//Add newServer to the list of serverNodes
	serverNodes.push_back(newServer);	
	
	map<char *,DBEntry>::iterator it;
	
	cout << "LOOKING FOR " << funcName << " IN A DB OF SIZE " << binderDB.size();
	it=binderDB.find(funcName);
	
	//TODO: Locate the proper function
	bool found = (it != binderDB.end());
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
			cout<<"Function already Exists"<<endl;
			
			(it->second).serverList.push(newServer);
		}else{
		
			cout<<"Inserting new signature"<<endl;
		
			DBEntry newSig;	
			newSig.name = funcName;
			newSig.argTypes = argTypes;
			newSig.serverList.push(newServer);
			
			//Add newSig to the list of DBEntrys
			binderDB.insert(pair<char *,DBEntry>(funcName, newSig));
		}
	}else{	
		cout<<"Inserting new signature"<<endl;
		
		DBEntry newSig;	
		newSig.name = funcName;
		newSig.argTypes = argTypes;
		newSig.serverList.push(newServer);
		
		//Add newSig to the list of DBEntrys
		binderDB.insert(pair<char *,DBEntry>(funcName, newSig));
	}
	
}

void handleLocateServerRequest(int msgLength, int socket) {
	char recvbuf[msgLength];
	recv(socket, recvbuf, msgLength, 0);
	
	char *msg = recvbuf;	
	int nameLength, argLength;	
	
	//Extract the function name length
	memcpy(&nameLength, msg,  sizeof(int));
	msg+=sizeof(int);
	
	//Alloacte enough memory to store the function name
	char *name = (char*) malloc(nameLength); 
	
	//Extract the function name
	memcpy(name, msg, nameLength);
	msg+=nameLength;
	
	cout<<"Name of method is "<<name<<endl;
	
	//Extract the function name length
	memcpy(&argLength, msg,  sizeof(int));
	msg+=sizeof(int);
	
	//Alloacte enough memory to store the function name
	int *argTypes = (int *) malloc(argLength);
	
	//Extract the function name
	memcpy(argTypes, msg, argLength);
	msg+=argLength;
	
	cout<<"Received arguments!"<<endl;
	
	//Searching for Server

	map<char *,DBEntry>::iterator it;
	it=binderDB.find(name);
	
	bool found = (it != binderDB.end());
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
			cout<<"Found correct function"<<endl;
			
			DBEntry d = (it->second);
			
			ServerNode selectedServer = d.serverList.front();
			
			cout << "The binder has selected server: "<< selectedServer.hostname << " to exectue the function request" << endl;
			cout << "The selected server's Port No. is " << selectedServer.port << endl;

			char* hostname = selectedServer.hostname;
			int portNum = selectedServer.port;
			
			int hostnameLength = strlen(hostname);
			
			//Add one to account for the terminal char
			hostnameLength++;
			
			int msgLength = sizeof(int) + hostnameLength + sizeof(int);
			
			RPC_MSG msg;
			msg.type = LOC_SUCCESS;
			msg.length = msgLength;

			write(socket, &msg, sizeof(msg));
			
			char header[msgLength];
			char* locMsg = header;	

			memcpy(locMsg, &hostnameLength, sizeof(int));
			locMsg+=sizeof(int);
			memcpy(locMsg, hostname, hostnameLength);
			locMsg+=hostnameLength;
			
			memcpy(locMsg, &portNum, sizeof(int));
			locMsg+=sizeof(int);
			
			send(socket, &header, msgLength, 0);
			
			FD_CLR(socket, &master);
			
			//close(socket);
		}else{
		
			cout<<"The function signature cannot be found in the db"<<endl;
			RPC_MSG msg;
			msg.type = LOC_FAILURE;
			write(socket, &msg, sizeof(msg));
			return;	
		}
	}else{	
		cout<<"The function signature cannot be found in the db"<<endl;
		RPC_MSG msg;
        msg.type = LOC_FAILURE;
        write(socket, &msg, sizeof(msg));
		return;	
	}

}

void handleTerminateServerRequest(RPC_MSG msg) {
	
	//Send a TERMINATE message to all the servers 
	for(int i = 0; i < serverNodes.size();i++) {
		write(serverNodes[i].socket, &msg, sizeof(msg)); 
	} 
}

int main () {

	//////////////////////////
	//	Create Listener 	//
	//////////////////////////

    FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);

    // get us a socket and bind it

    memset(&sa, 0, sizeof(struct sockaddr_in)); /* clear our address */
	gethostname(myname, 1000);           /* who are we? */
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

	printBinderLocation();

	//////////////////////////
	//	Handle Connections 	//
	//////////////////////////
	
	FD_SET(listener, &master);
	struct sockaddr_storage remoteaddr; // client address
	char remoteIP[INET6_ADDRSTRLEN];
	int newfd, i;
	//int stopbinder = 0;
	bool terminateBinder = false;

	fdmax = listener; // so far, it's this one

	
	// main loop
    while (!terminateBinder) {
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
                } else {	
					RPC_MSG msg;
					read(i, &msg, sizeof(msg));
					
					if(msg.type == REGISTER) {
						cout<<"Registration Message Received"<< (int) msg.type<<endl;
						cout<<"-----------------------------"<<endl;
						handleServerRegRequest(msg.length, i);
					}
					else if(msg.type == LOC_REQUEST) {
						cout<<"Location Request Received"<<endl;
						cout<<"-------------------------"<<endl;
						handleLocateServerRequest(msg.length, i);
					}
					else if(msg.type == TERMINATE) {
						cout<<"Terminate Message Received"<<endl;
						handleTerminateServerRequest(msg);
						terminateBinder=true;
						return 1;
					}
					
					
					if(terminateBinder){
						break;
					}
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END while loop--and you thought it would never end!
	
	return 0;
}
