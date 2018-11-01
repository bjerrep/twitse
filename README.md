[![Build Status](https://travis-ci.org/bjerrep/twitse.svg?branch=master)](https://travis-ci.org/bjerrep/twitse)

# twitse

## What is this
This is an experimental playground for getting a wireless raspberry pi client and a wired raspberry pi server in 'pretty good' time synchronization.

The following graph shows an hour of measurements on a plain 2.4GHz wifi. It is an ideal sunshine recording (given it seem to work) with unthrottled measurements. It can be interpreted in as many ways as one wants to do it, but the bottom line is that it give a graphical view of the standard deviation and response time for a measurement series. Given the awfull latencies of a wireless connection it doesn't look too bad to have a graph in the microsecond domain.

<p align="center"><img src="dataanalysis/data/1hour_throttle_off/server/plot.png"></p>

There are some more words

[here](doc/TLDR.md) (pure software implementation on unmodified raspberry pi) 

and 

[here](doc/VCTCXO.md) (VCTCXO mode, hardware modified raspberry pi where the processor xtal is replaced with a VCTCXO).

<br /><br /><br />

## Assorted links
PTPD on wireless raspberry pi 2. 50us precision back then so it might be even better now. This project was made before (!) spotting that discussion:
https://sourceforge.net/p/ptpd/discussion/469208/thread/74b512c5/

Frequency stability on raspberry 2 with some nice plots:
https://2n3904blog.com/raspberry-pi-2-frequency-stability/

Hackaday: GPS Disciplined xCXO. A nice writeup about voltage controlled oscillators and DACs.
https://hackaday.io/project/6872-gps-disciplined-xcxo

Talk about replacing the raspberry pi X1 crystal as done here in vctcxo mode.
https://raspberrypi.stackexchange.com/questions/74482/switch-out-the-x1-oscillator-on-a-rpi-2-3




[![Codacy Badge](https://api.codacy.com/project/badge/Grade/1f1ddf68d54641658fba20d23c885ad3)](https://www.codacy.com/app/bjerrep/twitse?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=bjerrep/twitse&amp;utm_campaign=Badge_Grade)