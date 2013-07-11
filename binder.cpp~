#include <iostream>
#include <map>
#include "binder.h"
#include <string>
#include <vector>

using namespace std;

Binder::Binder(){

}

Binder::~Binder(){
	//nothing needed here; no vars are on the stack
}

void Binder::handleClientLookupRequests(ClientToBinderMsg& msg){
	/*
		workflow:
			binder receives a msg from the client; a LOC_REQUEST msg from the client
			matches the client request to functions registered at the binder
			callRoundRobinAlgorithm(functionName)
			the binder returns the appropriate location to the client
			client is subsequently able to invoke rpcCall
	*/

		//the binder receives a request from the client to locate the server with the appropriate function
		//get the function name itself, stored in the client-binder message
		string functionName = msg.name;

		//given the function name, we need to locate the appropriate server
		//call the round robin algorithm to locate the appropriate server
		string serverName = callRoundRobinAlgorithm(functionName);
}


/*
	handles server registration requests from each server;
	registers exactly ONE function

	DEPRECATED
	rpc.h -> register() takes care of this #kumar
	
*/
void Binder::handleServerRegRequest(ServerRegRequest& serverRequest){
	/*
		binder receives a server registration request from the server
		binder registers the function in the DB
	*/
	//insert the server's appropriate procedure signature and location into the db

	//the key is the server location, the value is the function
	this->insertIntoDB(serverRequest.procSignature, serverRequest.location);
}


void Binder::printBinderLocation(){
	std::cout << "BINDER_ADDRESS " << machineName << "\n";
	std::cout << "BINDER_PORT " << portNumber << "\n";
}

/*
	inserts procedure signatures and locations into the map db
	map<string, string> binderDB;
*/
void Binder::insertIntoDB(string procName, string serverLocation){
	//binderDB.insert( procName, serverLocation );			//errors at this location
}


/*
	a function name is passed to the binder
	the binder subsequently calls the round robin algorithm

	returns - the string location of the server w/ the appropriate function
*/
string Binder::callRoundRobinAlgorithm(string function){

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
	std::vector<string>::iterator rrIter = servers.begin();

	//iterator that iterates through the binderDB map
	std::map<string, string>::iterator it;

	/*
		round robin algorithm
		
		first and foremost, check the rr server to see if it has the function
		if it does not, then check the remaining functions
	
	*/	
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

}//callRoundRobinAlgorithm

int main(){

}
