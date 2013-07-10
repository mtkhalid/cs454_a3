#ifndef _CLIENT_H
#define _CLIENT_H

class Client{
	public:
		Client();
		~Client();
		int rpcCall(char* name, int* argTypes, void ** args);
};

#endif
