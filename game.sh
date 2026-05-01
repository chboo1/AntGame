#!/bin/bash

GAME_NAME=""
PORT=""
while [[ "$1" == -* ]]; do
    case "$1" in
        -n) GAME_NAME="$2"; shift 2 ;;
        -p) PORT="$2"; shift 2 ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
done

if [[ $# != 4 ]]; then
    echo "Usage: game.sh [-n name] [-p port] client1.py client2.py client3.py client4.py"
    exit 1
fi

for i in {1..4}; do
    if [[ ! -f PyCode/${!i} ]]; then
        echo "Client file PyCode/${!i} does not exist"
        exit 1
    fi
done

# make sure no server is running
killall AntGameServer 2>/dev/null

# start game server
SERVER_ARGS="-l -s"
[[ -n "$GAME_NAME" ]] && SERVER_ARGS="$SERVER_ARGS -n $GAME_NAME"
[[ -n "$PORT" ]] && SERVER_ARGS="$SERVER_ARGS -p $PORT"
set -x
bin/AntGameServer $SERVER_ARGS &
set +x
server_pid=$!
echo "Server PID: $server_pid"

# Port arg for python clients (empty string if not set, so no arg passed)
CLIENT_ARGS=""
[[ -n "$PORT" ]] && CLIENT_ARGS="$PORT"

# start all 4 in the background, gather the pids, and wait for them to finish
python3 PyCode/$1 $CLIENT_ARGS &
pid1=$!
echo "Player 1 ($1) PID: $pid1"
python3 PyCode/$2 $CLIENT_ARGS &
pid2=$!
echo "Player 2 ($2) PID: $pid2"
python3 PyCode/$3 $CLIENT_ARGS &
pid3=$!
echo "Player 3 ($3) PID: $pid3"
python3 PyCode/$4 $CLIENT_ARGS &
pid4=$!
echo "Player 4 ($4) PID: $pid4"

trap "echo -e '\nKilling processes...'; kill $server_pid $pid1 $pid2 $pid3 $pid4 2>/dev/null; exit" INT

wait $server_pid || exit 1
wait $pid1 2>/dev/null
wait $pid2 2>/dev/null
wait $pid3 2>/dev/null
wait $pid4 2>/dev/null

exit 0
