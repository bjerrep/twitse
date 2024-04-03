#!/bin/bash

# build the 'twitse_server' and 'control' applications on the server and 
# the 'twitse_client' application on the clients.

source ./scripts/remote_server_and_clients.txt

if [ -n $SERVER ]; then
    echo -------------------- Launching server build --------------------------
    ssh -t $SERVER "cd ~/twitse/build ; nice make -j3 twitse_server" &
fi


# using 2 cores to avoid memory issues on 3A+ with 512kb

for s in $CLIENTS; do
    echo -------------------- Launching build on client $s --------------------------
    ssh -t $s "cd ~/twitse/build ; nice make -j2 twitse_client" &
done

wait < <(jobs -p)

exit
