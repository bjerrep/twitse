





[start page](../README.md)

# Twitse - Network



## Multicast

Multicast is used for the automatic connecting of clients and the server. Once started the clients broadcast a connection request on multicast and the server will respond with its ip address and a port to connect to with normal sockets.

Time and again multicast is a source of agony. So if a client cannot connnect to the server then kill all and everything related to twitse processes on server and client and install multicast-mtools on both. On arch this is via aur and ignore all complaints about architectures not matching.

On the server start the mtools multicast receiver (using the twitse broadcast address and port):

`mreceive -g 224.0.0.234 -p 45654`

Now still on the server try to start a sender in parallel (in a second terminal):

`msend -g 224.0.0.234 -p 45654 -t 8 -text "Broadcast on twitse address"`

And it should work nicely.

Next try to launch the sender on the client. Once this setup is working then twitse should be able to connect as well.



Things that might be worth trying:

* Update the router firmware, also make sure multicast is enabled (also on wireless)

* Make sure that there are only one network interface up on client and server.

* If there are firewalls then they might need rules adjustments.





# Wireless LAN

This is an attempt to preserve some kind of the observed wifi performances with regard to twitse time measurements and different means of wlan access. Since the observed performance at any given day can easily be attributed to just about anything else than the wifi adapter itself then consider this section a better-than-nothing starting point for further observations.

So far the general consensus seems to be that Realtek chipsets are the best.




| Chipset                                                      | Rating |
| ------------------------------------------------------------ | ------ |
| Onboard<br /> *Model 3 Model B - Revision: a02082*           | Usable |
|                                                              |        |
| CEPTER CCW11 AC600 (on raspberry 3A+)<br /> *0bda:1a2b Realtek Semiconductor Corp. RTL8188GU 802.11n WLAN Adapter (Driver CDROM Mode)* | Good   |



#### Getting the onboard raspberry pi Broadcom wifi

```
# getting the rpi version, from https://elinux.org/RPi_HardwareHistory
rev=$(awk '/^Revision/ { print $3 }' /proc/cpuinfo) && curl -L perturb.org/rpi?rev=$rev
```
