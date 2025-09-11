LIBRARIES := $(wildcard source/lib/*.cpp)
all: client server
	echo All

libs: out/*.o


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
