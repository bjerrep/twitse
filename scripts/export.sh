#!/bin/bash

# update the server and clients directly via rsync (and not though git)

source ./scripts/remote_server_and_clients.txt

if [ -n $SERVER ]; then
    USER=${SERVER%@*}
    rsync --exclude ".git" --exclude "build*" --exclude "bin"  -av -e ssh ${PWD} ${SERVER}:/home/${USER}
fi


for s in $CLIENTS; do
    USER=${s%@*}
    rsync --exclude ".git" --exclude "build*" --exclude "bin"  -av -e ssh ${PWD} ${s}:/home/${USER}
done
