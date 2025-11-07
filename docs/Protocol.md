# AntGame Network Protocol

## Basic information

The AntGame Network Protocol (AGNP) works directly over the TCP protocol in the default server implementation, but could technically work on any type of connection-based protocol. It runs using a central server which communicates with clients. By default, the server listens on port 55521, but it could run on any port, chosen by the server's configuration.

## Overview

### Requests

To communicate with an AGNP server, the client must connect and make a JOIN request, which will allow the server to verify you are a player. Afterwards, requests are made by the client in real time to both control ants and request information. To facilitate making requests quickly, a client may send multiple requests in one message with the following syntax:

- Message header ( 8 bytes )
	+ Message size ( 4 bytes )
		An unsigned integer. Dictates the size of the entire message including the message header. It consitutes an error if with this amount of data, the message is incomplete. In all cases, the server will wait until this many bytes are available, then read as many bytes as this field states and will assume the next thing to come is a new message. Any malformed or bisected requests are ignored.
	+ Request count ( 4 bytes )
		An unsigned integer. States the amount of requests to be expected in this message. If the message's size is not met when this many requests are read, the leftover bytes are discarded.

Per request:
- Request identifier ( 1 byte )
	A unique byte identifying the command that will be executed. See the [list of request types](#request-ids).
- \[Request argument\] (X bytes)
	This field is optional and is included in certain IDs which require the client to pass data or arguments. It is optional and its length is defined by the request ID. It can also be dynamic, which means it will specify its length as its first argument.

This makes for a message with a length of 8 bytes + 1 or more per request. The only exception to this system is that the very first message of a client must have exactly one request that is a JOIN request, which will make the very first message always be `"\x00\x00\x00\x09\x00\x00\x00\x01\x00"` or the unsigned ints 9 and 1, then 1 null byte (the code for JOIN).
Note that in all unsigned integer fields, the first byte is the most significant byte (Big-Endian byte ordering.)

### Responses

For each message a client sends, the server will make a response either giving the client the requested info or a status. Since each message can contain multiple requests, the server's response will be a list of responses to each request. They will always be in the same order as requests were sent. The responses follow the following syntax:

- Message header ( 8 bytes )
	+ Message size ( 4 bytes )
		An unsigned integer. Dictates the size of the entire message including the message header.
	+ Response count ( 4 bytes )
		An unsigned integer. States the amount of requests addressed by this message. If the message's size is not met when this many requests are read, the leftover bytes should be discarded.

For each response:
- Request identifier echo ( 1 byte )
	This is a protection against byte loss and other errors. It permits the client to confirm that this response is to the right request.
- Response identifier ( 1 byte )
	This byte identifies the type of response given by the server. It is usually some variation of OK or ERROR. See the [list of response types](#response-ids)
- \[Response argument size\] ( 4 bytes )
	This is an optional argument that defines the length of the response argument. This field and the next's presences are determined using the response identifier.
- \[Response argument\] ( X bytes )
	This is an optional argument that defines data sent over with the response. Its size is defined by the response argument size field that precedes it. Both fields' presences are determined by checkihg the response identifier.

### Management

There are a total of three management requests and two management responses that allow the client and server to communicate and verify their status of communication. These are JOIN, PING and BYE for the client and PING and BYE for the server. The behaviour of each is simple. A JOIN request is to be made once at the beginning by a client seeking to join a game, which will be responded with a standard OK or DENY. A PING request can be made at any time by both the client and the server (using the NONE req echo), to which the other party must respond with another PING. This serves as a way to verify whether the other party is still connected/operational. By default, the server pings clients after 1 second of inactivity. A BYE request is simply a courtesy to the other client, indicating that the connection will be imminently closed (for the default server i.e. after any response or one second). The peer may optionally reply with their own BYE, after finishing their last messages, or simply close the connection. Note that while all of these (except JOIN) are not strictly required for function, a scenario may arise where a PING request needs to be interpreted lest the connection be prematurely closed.

### Request IDs

|ID|Meaning|Arg1|Arg2|Arg3|Arg4|Response data|
|---|---|---|---|---|---|---|
|0x00|JOIN - The client requests to join this game| \- | \- | \- | \- | \- |
|0x00|NONE - Specifically when used as a request ID echo, this could mean the server is sending an unsolicited message.| \- | \- | \- | \- | \- |
|0x01|PING - The client is either making sure this server is alive or is answering another ping.| \- | \- | \- | \- | \- |
|0x02|BYE - The client is either acknowledging a server BYE or letting the server know it's disconnecting.| \- | \- | \- | \- | \- |
|0x03|NAME - The client wants to define its name.| Size of string ( 4 bytes ) | Name string (variable length, indicated by the size of string field)| \- | \- | \- |
|0x04|WALK - The client wants an ant to walk to a specified position.|Ant ID (4 bytes)|X position (4 bytes)|Y position (4 bytes)| \- | \- |
|0x05|SETTINGS - The client is asking for the game's settings.| \- | \- | \- | \- |The size of the file, then a configuration file, describing the current round's settings. It is valid to send the configuration file that initialized this round as is or to send nothing, meaning default settings.|
|0x06|TINTERACT - The client wants an ant to interact with a tile at a specified position.|Ant ID (4 bytes)|X position (4 bytes)|Y position (4 bytes)| \- | \- |
|0x07|AINTERACT - The client wants an ant to interact with another ant at the specified nest and ant IDs.|Self Ant ID (4 bytes)|Target Ant ID (4 bytes)| \- | \- | \- |
|0x08|NEWANT - The client wishes to make a new ant at their nest.|Ant type (1 byte)| \- | \- | \- | \- |

### Response IDs

|ID|Meaning|Type of data|Unsolicited?|
|---|---|---|---|
|0x00|OK - The server accepts the request.| \- |No|
|0x01|DENY - The server rejects the request.| \- |No|
|0x02|PING - The server is either answering a ping or making sure the client is alive.| \- |Either|
|0x03|BYE - The server is either acknowledging a client disconnect or telling the client to leave.| \- |Either|
|0x04|START - The server is starting the game now!| \- |Yes|
|0x05|OK, DATA - The server accepts the request and is carrying the requested data. The type of data will depend on the request.| Any |No|
|0x06|FAILURE - The server cannot fulfill the request because of its own error.| \- |No|
|0x07|CMDSUCCESS - The server is indicating that a command has succeeded|Command data (1 byte cmd ID, 4 bytes ant ID, 1-8 bytes argument, depending on cmd ID)|Yes|
|0x08|CMDFAIL - The server is indicating that a command has failed|Command data (1 byte cmd ID, 4 bytes ant ID, 1-8 bytes argument, depending on cmd ID)|Yes|

### Placeholder
I haven't made the part of the document whatever you clicked on should link to.
