LIBRARIES := $(wildcard source/lib/*.cpp)
all: AntGameServer tests res dummyClient module
	echo All

libs: out/*.o


tests: out/*.o tests.cpp
ifeq ($(OS),Windows_NT)
	g++ out/*.o tests.cpp -lws2_32 -Isource/headers -o tests
else
	g++ out/*.o tests.cpp -Isource/headers -o tests
endif


dummyClient: dummyClient.cpp out/*.o
ifeq ($(OS),Windows_NT)
	g++ out/*.o dummyClient.cpp -lws2_32 -Isource/headers -o dummyClient
else
	g++ out/*.o dummyClient.cpp -Isource/headers -o dummyClient
endif


AntGameServer: server.cpp out/*.o
ifeq ($(OS),Windows_NT)
	g++ out/*.o server.cpp -lws2_32 -Isource/headers -o AntGameServer
else
	g++ out/*.o server.cpp -Isource/headers -o AntGameServer
endif


out/*.o: source/lib/*.cpp out
ifeq ($(OS),Windows_NT)
	cd source/lib && g++ -c *.cpp -I../headers
	move /y source\\lib\\*.o out\\
else
	cd source/lib; g++ -c *.cpp -I../headers
	mv source/lib/*.o out
endif


out:
	mkdir out


res: resources/mapMaker


ifeq ($(OS), Windows_NT)
module: AntGameModule/AntGame.pyd libs

AntGameModule/AntGame.pyd: source/PythonFiles/AntGamemodule.cpp
	g++ -fpic -c source\\PythonFiles\\AntGameModule.cpp -IAntGameModule\\include -o out\\AntGamemodule.o
	g++ -shared out\\AntGamemodule.o -o AntGameModule\\AntGame.pyd -LAntGameModule\\libs -lpython314
else
endif


resources/mapMaker: resources/mapMaker.cpp
	g++ resources/mapMaker.cpp -o resources/mapMaker

.PHONY: all libs client res module
