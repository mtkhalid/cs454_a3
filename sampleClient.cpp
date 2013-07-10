//result = sum(vector)
#define PARAMETER_COUNT 2		//number of RPC arguments
#define LENGTH 23				//vector length

int argTypes[PARAMETER_COUNT+1]		//array of parameters
void **args = (void **)malloc(PARAMETER_COUNT * sizeof(void *));

argTypes[0] = (1 << ARG_OUTPUT) | (ARG_INT << 16);				//result
argTypes[1] = (1 << ARG_INPUT) | (ARG_INT << 16) | LENGTH;		//vector
argTypes[2] = 0;

args[0] = (void *)&result;
args[1] = (void*)vector;

rpcCall("sum", argTypes, args);
