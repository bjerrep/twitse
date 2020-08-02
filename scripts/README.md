# Helper scripts for updating remote server and client computers

The scripts here simplifies the operations of synchronizing source files and building new binaries on the individual target computers when developing. The computers in question will typically be a server and a couple of clients running on raspberry pi.

The (faster) alternative for producing new binaries for the targets would be to use a crosscompiler but that might be another day.

First add the developer pc SSH key to the remotes list of authorized keys (i.e. with the tool ssh-copy-id) to get rid of manual SSH login.

Next enter the SSH address (user@hostname) of the server and the client(s) in a copy of the remote_server_and_clients.txt.template file with the .template extension stripped. Server and clients as one are referred to as remotes below. All scripts uses this file to define where the remotes can be found.



NOTE: All scripts expects the current working directory (PWD) to be the twitse root when executed.



## ./scripts/remote_export.sh

rsync all source files to remotes



## ./scripts/remote_cmake_vctcxo.sh

configure all remotes (first time) or reconfigure all remotes in case of project structure changes. Targets will be release builds in vctcxo mode.



## ./scripts/remote_build.sh

build on all remotes (in parallel)



## One liner

So if there are just changes in one or more source files then the fire and forget will be:

./scripts/remote_export.sh && ./scripts/remote_build.sh && ./bin/control --kill all

The control application (part of twitse) is here used to make all the remote applications terminate and the default configuration of the twitse systemd scipts will then make systemd restart them automatically.

