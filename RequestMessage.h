/*
	Bismillah
*/
#include <iostream>
using namespace std;

/*
	a message sent from the client to the binder
	the client needs to send this message in order to locate the appropriate server
*/
struct ClientToBinderMsg{
	string LOC_REQUEST;
	char * name;
	int * argTypes;
};

/*
	a response (successful) from the binder to the client
*/
struct BinderToClientResponseSuccess{
	string LOC_SUCCESS;
	//string LOC_FAILURE;
	//int reasonCode;
	int server_identifier;
	int port;
};

/*
	a failure response from the binder to the client
*/
struct BinderToClientResponseFailure{
	string LOC_FAILURE;
	int reasonCode;
};

/*
	a response (generic) from the binder to the client
*/
struct BinderToClientResponseGeneric{
	string LOC_SUCCESS;	
	int server_identifier;
	int port;
	
	//failure case
	string LOC_FAILURE;
	int reasonCode;						//{reason code semantics: -1 = error; -2 = serverDown}
};

struct ClientServerMessage{
	string execute;
	string EXECUTE;
	string name;
	int * argTypes;
	void ** args;
};

struct TerminateMessage{
	string TERMINATE;
};


struct ServerRegRequest{
	string procSignature;		//name of the function; procedure signature residing on the server
	string location;					//server's ip address or host name
};
