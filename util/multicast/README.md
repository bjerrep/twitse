
A multicast transmitter and receiver intended to run on a server and client with twitse running in the background. If twitse is running as it should the receiver will print the latency from server to a (wireless) client. (it is not measuring the ping time, its forward latency only). This directory is not built by default.

Before starting, NTP should be off on the server and twitse restarted.

A typical run (in ns):

	00:02:01:486194 100002993
	00:02:01:691004 79533203
	00:02:01:895837 60056541
	00:02:02:103668 42552272
	00:02:02:308477 22072483
	00:02:02:718096 207402591
	00:02:02:919806 183814989
	00:02:03:124607 163339887
	00:02:03:329421 143859475
	00:02:03:534229 123363280
	00:02:03:739014 102859376
	00:02:03:944995 83547243
	00:02:04:148621 62877664
	00:02:04:353438 42401103
	00:02:04:558224 21890013
	00:02:04:967845 207221735
	00:02:05:172633 186726321
	00:02:05:377438 166245126

The periodicity looks line an interference pattern with the beacon/DTIM timing on the AP where all sorts of wireless receivers are running with power save (see links below). Values like these in the range 20-200 ms are obviously useless. It will probably require a tuned AP with only non power saving devices attached to get some sane values. And even that might not really be much better than plain unicast.

An ordinary ping can test multicast as well. On a client run

	# sysctl net.ipv4.icmp_echo_ignore_broadcasts=0

then the client will respond to multicast pings. On the server run e.g.

	ping 225.168.1.102
	
which for the same setup will once more yield measurements in the 20-200 ms range.

http://www.wireless-nets.com/resources/tutorials/802.11_multicasting.html
https://networkengineering.stackexchange.com/questions/36450/can-i-truly-multicast-over-wifi
https://dev.archive.openwrt.org/ticket/10271
