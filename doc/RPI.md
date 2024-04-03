#  RPI

##  /boot/config.txt

Used on both client and server, both RPI 3

    initramfs initramfs-linux.img followkernel
    
    enable_uart=1
    dtparam=sd_overclock=100
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
    dtoverlay=pi3-disable-bt
    force_eeprom_read=0
    
    # client only with a hifiberry soundcard
    dtoverlay=hifiberry-dac


## client

Turn wifi power save off. The RPI is said not to support it but it seems to do something nice:

	iwconfig wlan0 power off

Make persistent as udev rule _/etc/udev/rules.d/70-wifi-powersave.rules_

    ACTION=="add", SUBSYSTEM=="net", KERNEL=="wlan*", RUN+="/usr/bin/iw dev %k set power_save off"

## server & client

Stop anything sounding like ntp or systemd-timesyncd or the like if running in software mode.

## Building - native

Building natively (as opposed to crosscompile) on the Pi is super slow but a few scripts will make life much easier for build-deploy cycles when developing. As the first thing copy the development PC public keys to the server and clients (see ssh-copy-id) to avoid having to login. Then as a first manually login to the Pi's and  run cmake to setup the correct build targets and build the relevant target(s) to catch any missing dependencies. If there is no ntp (e.g. systemd-timesyncd) client running on the target then start one before building to avoid confusing make.

Make a *remote_server_and_clients.txt* file in ./scripts with the server and/or client that should be used.

Now there is a scripts/export.sh and a scripts/build.sh that do what they say.

So in twitse root its now

`$ scripts/export.sh && scripts/build.sh && bin/control --kill all`

Things started with systemd will now restart (given that Restart=always in the systemd scripts).

Otherwise processes will just exit and be ready to get restarted.

## systemd

Systemd log file maintenance seems to have a negative impact on the time measurements, especially when processes are running with a lot of debug output. If its ok to loose systemd logs at power off:

    Storage=volatile in /etc/systemd/journald.conf
    journalctl --rotate
    journalctl --vacuum-time=1s
    systemctl daemon-reexec

Issue a journalctl --disk-usage to check that the disk logs doesn't grow anymore.



# SD card backup and restore

If fed up with binary copies of entire SD cards that for a start are problematic when SD cards are (never) the same size and manufacturer then just copy the files present and relevant on the SD card from the mounted /boot and / partitions.

### Backup

Make a copy of the /boot and /root partitions as verbatim filesystem copies with uncompressed files. (last exclude matches Arch package cache)

```
#	rsync -av <src> <dest> --exclude 'usr/share/doc/' --exclude 'var/cache/pacman/pkg/'
```

Note: Make sure that the destination drive is also formatted as Ext4 so symlinks etc. can be preserved.

There is a number of root directories that it makes no sense to backup (/proc /dev etc) but for some reason they seem to get ignored by the rsync above.

### Restore

Restore to a mount point. Observe the / on the source in order not to make the last source directory on destination as the root for the restore.

#rsync -av <src>/ <dest>



Links:

https://help.ubuntu.com/community/BackupYourSystem/TAR

# Prerequisites

Optionals: usbutils, lshw, ethtool



