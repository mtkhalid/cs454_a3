/*
	Bismillah
*/
#include <iostream>
using namespace std;

/*
	a message sent from the client to the binder
	the client needs to send this message in order to locate the appropriate server
*/
struct clientToBinderMsg{
	string LOC_REQUEST;
	char * name;
	int * argTypes;
};

/*
	a response (successful) from the binder to the client
*/
struct binderToClientResponseSuccess{
	string LOC_SUCCESS;
	//string LOC_FAILURE;
	//int reasonCode;
	int server_identifier;
	int port;
};

/*
	a failure response from the binder to the client
*/
struct binderToClientResponseFailure{
	string LOC_FAILURE;
	int reasonCode;
};

/*
	a response (generic) from the binder to the client
*/
struct binderToClientResponseGeneric{
	string LOC_SUCCESS;	
	int server_identifier;
	int port;
	
	//failure case
	string LOC_FAILURE;
	int reasonCode;						//{reason code semantics: -1 = error; -2 = serverDown}
};

struct clientServerMessage{
	string execute;
	string EXECUTE;
	string name;
	int * argTypes;
	void ** args;
};

struct terminateMessage{
	string TERMINATE;
};

int main(){
	return 0;
}
