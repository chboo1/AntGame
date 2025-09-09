all: client server
	# Stuff
	echo All


client: libs
	echo Client

server: libs
	echo Server

libs: source/lib/*.cpp
	g++ -c source/lib/*.cpp
	ar qs out/antgame.a
.PHONY: all
