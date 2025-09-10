LIBRARIES := $(wildcard source/lib/*.cpp)
all: client server
	echo All

libs: out/*.o out/antgame.a


client: libs
	echo Client >> client

server: libs
	echo Server >> server

out/antgame.a: out/*.o out
	ar -qs out/antgame.a out/*.o

out/*.o: source/lib/*.cpp out
	cd source/lib; g++ -c *.cpp -I../headers
	mv source/lib/*.o out

out:
	mkdir out

.PHONY: all libs client
