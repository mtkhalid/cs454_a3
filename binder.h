#include <map>
#include <queue>

#include "rpc.h"

struct ServerNode{
	char *hostname;
	int port;
	int socket;
};

struct DBEntry{
	char* name;
	int* argTypes;
	std::queue<ServerNode> serverList;
};

struct cmp_str
{
   bool operator()(char const *a, char const *b)
   {
      return std::strcmp(a, b) < 0;
   }
};

std::map<char*, DBEntry, cmp_str> binderDB;
std::vector<ServerNode> serverNodes;
