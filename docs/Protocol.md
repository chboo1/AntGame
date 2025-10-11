# AntGame Network Protocol

## Basic information

The AntGame Network Protocol (AGNP) works directly over the TCP protocol in the default server implementation, but could technically work on any type of connection-based protocol. It runs using a central server which communicates with clients. By default, the server listens on port 55521, but it could run on any port, chosen by the server's configuration.

## Overview

### Requests

To communicate with an AGNP server, the client must connect and make a JOIN request, which will allow the server to verify you are a player. Afterwards, requests are made by the client in real time to both control ants and request information. To facilitate making requests quickly, a client may send multiple requests in one message with the following syntax:

- Message header ( 8 bytes )
	+ Message size ( 4 bytes )
		An unsigned integer. Dictates the size of the entire message including the message header. It consitutes an error if less data is available than this field states or if with this amount of data, the message is incomplete. In all cases, the server will read as many bytes as this field states, or as many as are available and will assume the next thing to come is a new message. Any malformed or bisected requests are ignored.
	+ Request count ( 4 bytes )
		An unsigned integer. States the amount of requests to be expected in this message. If the message's size is not met when this many requests are read, the leftover bytes are discarded.

Per request:
- Request identifier ( 1 byte )
	A unique byte identifying the command that will be executed. See the [list of request types](#request-ids).
- Request argument ( 8 bytes )
	This argument does different things depending on the ID, but it's usually either and unsigned long or two unsigned ints/a position. If the request ID takes no argument, then all of these bytes are ignored (but should still be there!)
- \[Request argument continuation\] (X bytes)
	This is an optional part for specific request IDs that require longer arguments. In these cases, the first four bytes of the request argument will define the length of this field. The other four bytes of the request argument may or may not be used, depending on the request ID.


This makes for a message with a length of 8+9*n bytes, where n is the amount of requests. The only exception to this system is that the very first message of a client must have exactly one request that is a JOIN request, which will make the very first message always be `"\x00\x00\x00\x11\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00"` or the unsigned ints 17 and 1, then 9 null bytes (the first of which is the code for JOIN and the others being the empty argument of that command). The only variation in the first message can lie in the eight last bytes (request argument) as they are ignored.
Note that in all unsigned integer fields, the first byte is the most significant byte (Big-Endian byte ordering.)

### Responses

For each message a client sends, the server will make a response either giving the client the requested info or a status. Since each message can contain multiple requests, the server's response will be a list of responses to each request. They will always be in the same order as requests were sent. The responses follow the following syntax:

- Message header ( 8 bytes )
	+ Message size ( 4 bytes )
		An unsigned integer. Dictates the size of the entire message including the message header. It consitutes an error if less data is available than this field states or if with this amount of data, the message is incomplete. In all cases, the server will read as many bytes as this field states, or as many as are available and will assume the next thing to come is a new message. Any malformed or bisected requests are ignored.
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

### Request IDs

|ID|Meaning|Arg1|Arg2|
|---|---|---|---|
|0x00|JOIN - The client requests to join this game| \- | \- |

### Response IDs

|ID|Meaning|
|---|---|
|0x00|OK - The server accepts the request.|

### Placeholder
I haven't made the part of the document whatever you clicked on should link to.
