CXX = g++ # variables and initialization
CXXFLAGS = -g -Wall -MMD # builds dependency lists in .d ?les
OBJECTS = client1.o rpc.o
DEPENDS = ${OBJECTS:.o=.d} # substitute ".o" with ".d"
EXEC = client

${EXEC} : ${OBJECTS}
	${CXX} ${CXXFLAGS} ${OBJECTS} -o ${EXEC}

clean : # separate target; cleans directory
	rm -rf ${DEPENDS} ${OBJECTS} ${EXEC}

-include ${DEPENDS} # reads the .d ?les and reruns 
						  # dependencies
