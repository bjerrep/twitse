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

## client

Turn wifi power save off. The RPI is said not to support it but it seems to do something nice:

	iwconfig wlan0 power off

## server & client

Stop anything sounding like ntp or systemd-timesyncd or the like.


## note

backup a sd card

dd bs=4M if=/dev/mmcblk0 | gzip > ludit_arch64_`date +%d%m%y`.gz

restore

gzip -dc <name.gz> | dd bs=4M of=/dev/mmcblk0
