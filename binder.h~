#ifndef _BINDER_H
#define _BINDER_H

#include <map>
#include <string>
#include "RequestMessage.h"

using namespace std;

class Binder{
	
		string machineName;		//machine name where the binder is executing
		int portNumber;				//port number that the binder is listening to
		std::map<string, string> binderDB;
//		void handleFunctionOverloading();		
		string callRoundRobinAlgorithm(string function);

	public:
		Binder();
		~Binder();
		void handleClientLookupRequests(ClientToBinderMsg& msg);
		void handleServerRegRequest(ServerRegRequest& serverRequest);
		void printBinderLocation();
   	void insertIntoDB(string procName, string serverLocation);
};

#endif
