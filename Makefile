LIBRARIES := $(wildcard source/lib/*.cpp)
all: client server tests
	echo All

libs: out/*.o


tests: libs tests.cpp
ifeq ($(OS),Windows_NT)
	g++ out/*.o tests.cpp -lws2_32 -Isource/headers -o tests
else
	g++ out/*.o tests.cpp -Isource/headers -o tests
endif


client: libs
	echo Client >> client

server: libs
	echo Server >> server

out/*.o: source/lib/*.cpp out
ifeq ($(OS),Windows_NT)
	cd source/lib & g++ -c *.cpp -I../headers
	move /y source\\lib\\*.o out\\
else
	cd source/lib; g++ -c *.cpp -I../headers
	mv source/lib/*.o out
endif

out:
	mkdir out

.PHONY: all libs client
