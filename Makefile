CXX = g++ # variables and initialization
CXXFLAGS = -pthread

OBJECT_CLIENT = client.o
OBJECT_BINDER = binder.o 
OBJECT_SERVER = server.o
OBJECT_SERVERFUNCS = server_functions.o server_function_skels.o
OBJECT_RPC = rpc.o

TARGETS = client binder server librpc.a
RPCLIBRARY = librpc.a

EXEC_CLIENT = client
EXEC_BINDER = binder
EXEC_SERVER = server

%.o: %.cpp 
	$(CXX) -c -o $@ $< $(CXXFLAGS)

all: $(TARGETS)

librpc.a: $(OBJECT_RPC)
	ar rc librpc.a rpc.o 
	
${EXEC_CLIENT} : $(OBJECT_CLIENT) $(RPCLIBRARY)
	${CXX} ${CXXFLAGS} ${OBJECT_CLIENT} -L. -lrpc -o ${EXEC_CLIENT}

${EXEC_BINDER} : $(OBJECT_BINDER)
	${CXX} ${CXXFLAGS} ${OBJECT_BINDER} -o ${EXEC_BINDER}
	
${EXEC_SERVER} : $(OBJECT_SERVER) $(RPCLIBRARY) $(OBJECT_SERVERFUNCS) 
	${CXX} ${CXXFLAGS} $(OBJECT_SERVER) $(OBJECT_SERVERFUNCS) -L. -lrpc -o ${EXEC_SERVER}

clean:
	rm -f *.o $(TARGETS)