#include "client.h"
#include <iostream>
#include <vector>

//using std::vector<unsigned char>
using namespace std;

#define ARG_CHAR 	1
#define ARG_SHORT 	2
#define ARG_INT 	3
#define ARG_LONG 	4
#define ARG_DOUBLE 	5
#define ARG_FLOAT 	6
#define ARG_INPUT 31
#define ARG_OUTPUT 30


Client::Client(){

}

Client::~Client(){

}

/*
 * From stack overflow:
 * http://stackoverflow.com/questions/5585532/c-int-to-byte-array
 *
 * paramInt - the integer we wish to convert to a byte
 *
 */
vector<unsigned char> intToBytes(int paramInt)
{
     vector<unsigned char> arrayOfByte(4);
     for (int i = 0; i < 4; i++)
    	 //bit shift the integer by 8 bits
         arrayOfByte[3 - i] = (paramInt >> (i * 8));
     return arrayOfByte;
}


/*
 * purpose: sends a location request message to the binder; to locate the server
 *
 */
int locateServer(){
	bool foundServer = false;

	if (foundServer){
		return true;
	}else{
		return false;
	}

	//sendLocationRequestMessageToBinder();
}

/*
 * Purpose: sends an remote procedure call to the server
 *
 * name - the name of the remote procedure to be executed
 * argTypes - types of arguments; whether the argument is "input to", "output from", or "input to, output from" the server
 *
 *
 */
int Client::rpcCall(char* name, int* argTypes, void ** args){
	/*
	 * send a location request message to the binder to locate the server for the procedure
	 *
	 */
	if (!locateServer()){
		//we could not locate the server
		cout << "alas, we have not found the server!\n";
		return -1;
	}else{
		//we found the server
		cout << "found the server!\n";
		//sendExecuteRequestMsgToServer();		//#TODO
		return 0;
	}
}


int main(){

	Client c;
	c.rpcCall("sum", NULL, NULL);
	return 0;

}






