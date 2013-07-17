#include <iostream>
#include <map>
#include <string>
#include <queue>
#include <vector>
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
#include <errno.h>
#include <pthread.h>

#include "binder.h"
#include "rpc.h"
#include "RequestMessage.h"

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
char   myname[1000];

/*
	handles server registration requests from each server;
	registers exactly ONE function

	DEPRECATED
	rpc.h -> register() takes care of this #kumar
	
*/
void handleServerRegRequest(int msgLength, int socket){
	/*
		binder receives a server registration request from the server
		binder registers the specified function in the DB
	*/
	//inser the server's appropriate procedure signature and location into the db

	//the key is the server location, the value is the function
	//this->insertIntoDB(serverRequest.location, serverRequest.procSignature);
	
	char recvbuf[msgLength];
	recv(socket, recvbuf, msgLength, 0);
	
	char *msg = recvbuf;	
	int hostnameLength, port, nameLength, argLength;	
	
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
	
	cout<<"Received arguments!"<<endl;

	cout<<"Inserting new server"<<endl;
	
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


//TODO: Still need to implement - Check the way I implemented handleServerRegRequest
void handleLocateServerRequest(int msgLength, int socket){
	/*
		workflow:
			binder receives a msg from the client; a LOC_REQUEST msg from the client
			matches the client request to functions registered at the binder
			callRoundRobinAlgorithm(functionName)
			the binder returns the appropriate location to the client
			client is subsequently able to invoke rpcCall
	*/

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
			
			//Pop the selected server and move it to the back of the queue
			d.serverList.pop();
			d.serverList.push(selectedServer);
			
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
			
			close(socket);
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

void handleTerminateServerRequest(RPC_MSG msg){

	//Send a TERMINATE message to all the servers 
	for(int i = 0; i < serverNodes.size();i++) {
		write(serverNodes[i].socket, &msg, sizeof(msg)); 
	} 
}

//TODO: Still need to implement
/*
	a function name is passed to the binder
	the binder subsequently calls the round robin algorithm

	returns - the string location of the server w/ the appropriate function
*/
string callRoundRobinAlgorithm(string function){

	string serverToPointTo;

	//first iterate through the first server (A) to see if it has the correct function
	/* previous algorithm

        for (it = binderDB.begin(); it != binderDB.end(); ++it){
            //function based
            if (it->second() == function){
                serverToPointTo = it->first;
                break;	//we found our function
            }else{
                //keep iterating
            }
        }
	*/

	//here is our vector of servers #test
	std::vector<string> servers;

	//for test purposes only
	servers.push_back("8080");
	servers.push_back("8090");
	servers.push_back("8010");

	//round robin server ("rrserver") points to the current server in the RR algorithm
	string rrServer = servers.front();

	//create an iterator to point to the first server in the list
	//rrIter points to the beginning server in the RR list
	//std::vector<string>::iterator rrIter = servers.begin();
	//	rrIter points to the beginning server in the RR list

	//iterator that iterates through the binderDB map; this iterator is of the same type as binderDB;
	//std::map<string, string>::iterator it;
	//we cannot iterate through a map :)

	
	//NOTE: I commented this out for now until we discuss the structure of the db 

	//iterator that iterates through the binderDB map
	//std::map<string, string>::iterator it;

	/*
		round robin algorithm
		
		first and foremost, check the rr server to see if it has the function
		if it does not, then check the remaining functions
	
	*/	
	
	/*
	for (it = binderDB.begin(); it != binderDB.end(); ++it){
		//check the rr server
		if (binderDB->first == 'rrServer'){
			//we first check to see if the rrServer has our appropriate function
			if (binderDB->second == function){
				//return this server's address
				return (binderDB->first);
			}else{
				//go to the next server in the rrIter list; check if it has the appropriate function
				rrIter++;
			}
		}
	}//for
	*/
	
	
}//callRoundRobinAlgorithm


void printBinderLocation(){
	std::cout << "BINDER_ADDRESS " << myname << "\n";
	std::cout << "BINDER_PORT " << listening_port << "\n";
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(){

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
						cout<<"REGISTRATION message received"<<endl;
						handleServerRegRequest(msg.length, i);
					}
					
					else if(msg.type == LOC_REQUEST) {
						cout<<"LOCATION message received"<<endl;
						handleLocateServerRequest(msg.length, i);
					}
					else if(msg.type == TERMINATE) {
						cout<<"TERMINATE message received"<<endl;
						handleTerminateServerRequest(msg);
						terminateBinder=true;
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
