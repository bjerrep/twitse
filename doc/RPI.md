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

Stop anything sounding like ntp or systemd-timesyncd or the like.


## other

backup a sd card

dd bs=4M if=/dev/mmcblk0 status=progress | <name>_`date +%Y%m%d.gz`

restore

gzip -dc <name.gz> | dd bs=4M of=/dev/mmcblk0
