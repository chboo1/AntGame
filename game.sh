#!/bin/bash

if [[ $# != 4 ]]; then
    echo "Must specify 4 client names as parameters (with .py extension)"
    exit 1
fi

for i in {1..4}; do
    if [[ ! -f AntGameModule/${!i} ]]; then
        echo "Client file AntGameModule/${!i} does not exist"
        exit 1
    fi
done

# make sure no server is running
killall AntGameServer 2>/dev/null

# start game server
bin/AntGameServer -l -s &
server_pid=$!
echo "Server PID: $server_pid"

# start all 4 in the background, gather the pids, and wait for them to finish
python3 AntGameModule/$1 &
pid1=$!
echo "Player 1 ($1) PID: $pid1"
python3 AntGameModule/$2 &
pid2=$!
echo "Player 2 ($2) PID: $pid2"
python3 AntGameModule/$3 &
pid3=$!
echo "Player 3 ($3) PID: $pid3"
python3 AntGameModule/$4 &
pid4=$!
echo "Player 4 ($4) PID: $pid4"

trap "echo -e '\nKilling processes...'; kill $server_pid $pid1 $pid2 $pid3 $pid4 2>/dev/null; exit" INT

wait $server_pid || exit 1
wait $pid1 2>/dev/null
wait $pid2 2>/dev/null
wait $pid3 2>/dev/null
wait $pid4 2>/dev/null

exit 0