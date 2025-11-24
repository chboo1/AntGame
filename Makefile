LIBRARIES := $(wildcard source/lib/*.cpp)
all: AntGameServer tests res dummyClient
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


resources/mapMaker: resources/mapMaker.cpp
	g++ resources/mapMaker.cpp -o resources/mapMaker

.PHONY: all libs client res
