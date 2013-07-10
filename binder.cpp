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
			client has location
	*/

		//the binder receives a request from the client to locate the server with the appropriate function
		//string LOC_REQUEST = msg.LOC_REQUEST;
		
		string functionName = msg.name;

		//now we have the function name, we need to see which server has this function according
		//to the round robin algorithm

		//serverName = the server w/ the appropriate function
		string serverName = callRoundRobinAlgorithm(functionName);

}


/*
	handles server registration requests from each server;
	registers exactly ONE function
*/
void Binder::handleServerRegRequest(ServerRegRequest& serverRequest){
	/*
		binder receives a server registration request from the server
		binder registers the function in the DB
	*/
	//inser the server's appropriate procedure signature and location into the db

	//the key is the server location, the value is the function
	//this->insertIntoDB(serverRequest.location, serverRequest.procSignature);

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

	std::vector<string> servers;

	//for test purposes only
	servers.push_back("8080");
	servers.push_back("8090");
	servers.push_back("8010");

	string rrServer = servers.front();

	//create an iterator to point to the first server in the list
	std::vector<string>::iterator rrIter = servers.begin();
	//	rrIter points to the beginning server in the RR list

	//iterator that iterates through the binderDB map; this iterator is of the same type as binderDB;
	//	std::map<string, string>::iterator it;
	//we cannot iterate through a map :)


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
