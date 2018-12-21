#  RPI

##  /boot/config.txt

Used on both client and server, both RPI 3

    enable_uart=1
    dtparam=sd_overclock=100
    dtparam=spi=on
    gpu_mem=16
    force_turbo=1
    arm_freq=1300
    core_freq=500
    sdram_freq=500
    over_voltage=2
    over_voltage_sdram=2
    
    # i2c and spi if wanted
    dtparam=i2c1=on
    dtparam=i2c_arm=on
    dtparam=spi=on


## client

Turn wifi power save off. The RPI is said not to support it but it seems to do something nice:

	iwconfig wlan0 power off

Make persistent as udev rule _/etc/udev/rules.d/70-wifi-powersave.rules_

    ACTION=="add", SUBSYSTEM=="net", KERNEL=="wlan*", RUN+="/usr/bin/iw dev %k set power_save off"

## server & client

Stop anything sounding like ntp or systemd-timesyncd or the like if running in software mode.

## building - native

Building on the Pi is super slow. A few scripts will make life much easier for testing. As the first thing copy the development PC public keys to the server and clients (see ssh-copy-id) to avoid having to login. Then as a first login to the Pi's and manually run cmake and make to verify that everything is ready.

The first script uses rsync for beaming the latest and greatest source files to the Raspberry Pi computers. Here there are two clients. Adapt the number of clients and their usernames and hostnames.

    export.sh

    #!/bin/bash
    # Execute from project root. Adapt usernames and hostnames. 
    # Also adapt the destination path if the project isnt located directly in the remote home dir.
    rsync --exclude ".git" --exclude "build*" --exclude "bin"  -av -e ssh ${PWD} ludit@server:/home/ludit
    rsync --exclude ".git" --exclude "build*" --exclude "bin"  -av -e ssh ${PWD} ludit@right:/home/ludit
    rsync --exclude ".git" --exclude "build*" --exclude "bin"  -av -e ssh ${PWD} ludit@left:/home/ludit

The second script compiles the server and the client(s) remotely via ssh:

    remote_build.sh

    #!/bin/bash
    ssh -t ludit@server "cd ~/twitse/build ; make -j2 twitse_server control" &
    ssh -t ludit@left "cd ~/twitse/build ; make -j2 twitse_client" &
    ssh -t ludit@right "cd ~/twitse/build ; make -j2 twitse_client" &
    wait < <(jobs -p)
    exit

If all this was in the project root on the developer PC then the following line will export the source files, build the server and clients and then restart them all:

    /export.sh && ./remote_build.sh && bin/control --kill all
    
This relies on the line Restart=always in the systemd scripts. 

## systemd

Systemd log file maintenance seems to have a negative impact on the time measurements, especially when processes are running with a lot of debug output. If its ok to loose systemd logs at power off:

    Storage=volatile in /etc/systemd/journald.conf
    journalctl --rotate
    journalctl --vacuum-time=1s
    systemctl daemon-reexec

Issue a journalctl --disk-usage to check that the disk logs doesn't grow anymore.

## other

backup a sd card

dd bs=4M if=/dev/mmcblk0 status=progress | <name>_`date +%Y%m%d.gz`

restore

gzip -dc <name.gz> | dd bs=4M of=/dev/mmcblk0
