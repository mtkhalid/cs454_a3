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

	//////////////////////////
	//	Connect to Binder 	//
	//////////////////////////

	char* binder_address = getenv("BINDER_ADDRESS");
	cout << "Binder Address is" << binder_address << endl;

	char* binder_port = getenv("BINDER_PORT");
	cout << "Binder Port is" << binder_port << endl;


	return(listener);
}


int rpcCall(char* name, int* argTypes, void** args){
	
	struct sockaddr_in sa;
	struct hostent *hp;
	int yes=1;	

	bool successful = false;
	bool warning = false;
	bool error = false;

/*
		the client calls the rpcCall function
		connects to the binder		
*/


	if (successful){
		return 0;
	}else if (warning){
		return 1;
	} else if (error){
		return -1;
	}

}


int rpcCacheCall(char* name, int* argTypes, void** args){
}

int rpcRegister(char* name, int* argTypes, skeleton f){

}

int rpcExecute(){

}

int rpcTerminate(){

}

int main(){


}
