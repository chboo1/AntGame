LIBRARIES := $(wildcard source/lib/*.cpp)
all: bin/AntGameServer bin/tests res bin/dummyClient module


bin/tests: out/*.o source/tests.cpp
ifeq ($(OS),Windows_NT)
	g++ out/*.o source/tests.cpp -lws2_32 -Isource/headers -o bin/tests
else
	g++ out/*.o source/tests.cpp -Isource/headers -o bin/tests
endif


bin/dummyClient: source/dummyClient.cpp out/*.o
ifeq ($(OS),Windows_NT)
	g++ out/*.o source/dummyClient.cpp -lws2_32 -Isource/headers -o bin/dummyClient
else
	g++ out/*.o source/dummyClient.cpp -Isource/headers -o bin/dummyClient
endif


bin/AntGameServer: source/server.cpp out/*.o
ifeq ($(OS),Windows_NT)
	g++ out/*.o source/server.cpp -lws2_32 -Isource/headers -o bin/AntGameServer
else
	g++ out/*.o source/server.cpp -Isource/headers -o bin/AntGameServer
endif


out/*.o: source/lib/*.cpp
ifeq ($(OS),Windows_NT)
	cd source/lib && g++ -c *.cpp -I../headers
	move /y source\\lib\\*.o out\\
else
	cd source/lib; g++ -c *.cpp -I../headers
	mv source/lib/*.o out
endif


ifeq ($(OS), Windows_NT)
module: libs AntGameModule/AntGame.pyd

AntGameModule/AntGame.pyd: source/AntGamemodule.cpp
	g++ -fpic -c source\\AntGameModule.cpp -IAntGameModule\\include -o AntGameModule\\AntGamemodule.o
	g++ -shared AntGameModule\\AntGamemodule.o -lws2_32 -o AntGameModule\\AntGame.pyd -LAntGameModule\\libs -lpython314
else
module: libs AntGameModule/AntGame.so

AntGameModule/AntGame.so: source/AntGamemodule.cpp
	g++ -fpic -c source/AntGameModule.cpp -IAntGameModule/include -o AntGameModule/AntGamemodule.o
	g++ -shared AntGameModule/AntGamemodule.o out/*.o -o AntGameModule/AntGame.so -LAntGameModule/libs -lpython3.14
endif


bin/mapMaker: source/resources/mapMaker.cpp
	g++ source/resources/mapMaker.cpp -o bin/mapMaker

bin/mapMakerSmall: source/resources/mapMakerSmall.cpp
	g++ source/resources/mapMakerSmall.cpp -o bin/mapMakerSmall

libs: out/*.o

res: bin/mapMaker bin/mapMakerSmall

server: bin/AntGameServer

client: bin/dummyClient

.PHONY: all libs res module server client
