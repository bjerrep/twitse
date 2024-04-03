#!/bin/bash

# the big hammer to check if everything snaps into place when coldstarting. 
# Requires a passwordless reboot, see readme.

source ./scripts/remote_server_and_clients.txt

ssh -t $SERVER "sudo reboot"

for s in $CLIENTS; do
    ssh -t $s "sudo reboot"
done
