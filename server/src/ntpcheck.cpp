/*
 *
 * (C) 2014 David Lettier.
 *
 * http://www.lettier.com/
 *
 * NTP client.
 *
 * Compiled with gcc version 4.7.2 20121109 (Red Hat 4.7.2-8) (GCC).
 *
 * Tested on Linux 3.8.11-200.fc18.x86_64 #1 SMP Wed May 1 19:44:27 UTC 2013 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc main.c -o ntpClient.out
 *
 * Usage: $ ./ntpClient.out
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "ntpcheck.h"
#include "systemtime.h"
#include "log.h"
#include "globals.h"
#include "system.h"

#define NTP_TIMESTAMP_DELTA 2208988800ull

#define LI(packet)   (uint8_t) ((packet.li_vn_mode & 0xC0) >> 6) // (li   & 11 000 000) >> 6
#define VN(packet)   (uint8_t) ((packet.li_vn_mode & 0x38) >> 3) // (vn   & 00 111 000) >> 3
#define MODE(packet) (uint8_t) ((packet.li_vn_mode & 0x07) >> 0) // (mode & 00 000 111) >> 0


uint64_t ntp_time_ns( const char* ntp_server )
{
    int sockfd, n; // Socket file descriptor and the n return result from writing/reading from the socket.

    int portno = 123; // NTP UDP port number.

    // Structure that defines the 48 byte NTP packet protocol.

    typedef struct
    {

        uint8_t li_vn_mode;      // Eight bits. li, vn, and mode.
            // li.   Two bits.   Leap indicator.
            // vn.   Three bits. Version number of the protocol.
            // mode. Three bits. Client will pick mode 3 for client.

        uint8_t stratum;         // Eight bits. Stratum level of the local clock.
        uint8_t poll;            // Eight bits. Maximum interval between successive messages.
        uint8_t precision;       // Eight bits. Precision of the local clock.

        uint32_t rootDelay;      // 32 bits. Total round trip delay time.
        uint32_t rootDispersion; // 32 bits. Max error aloud from primary clock source.
        uint32_t refId;          // 32 bits. Reference clock identifier.

        uint32_t refTm_s;        // 32 bits. Reference time-stamp seconds.
        uint32_t refTm_f;        // 32 bits. Reference time-stamp fraction of a second.

        uint32_t origTm_s;       // 32 bits. Originate time-stamp seconds.
        uint32_t origTm_f;       // 32 bits. Originate time-stamp fraction of a second.

        uint32_t rxTm_s;         // 32 bits. Received time-stamp seconds.
        uint32_t rxTm_f;         // 32 bits. Received time-stamp fraction of a second.

        uint32_t txTm_s;         // 32 bits and the most important field the client cares about. Transmit time-stamp seconds.
        uint32_t txTm_f;         // 32 bits. Transmit time-stamp fraction of a second.

    } ntp_packet;              // Total: 384 bits or 48 bytes.

    // Create and zero out the packet. All 48 bytes worth.

    ntp_packet packet = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    memset( &packet, 0, sizeof( ntp_packet ) );

    // Set the first byte's bits to 00,011,011 for li = 0, vn = 3, and mode = 3. The rest will be left set to zero.

    *( ( char * ) &packet + 0 ) = 0x1b; // Represents 27 in base 10 or 00011011 in base 2.

    // Create a UDP socket, convert the host-name to an IP address, set the port number,
    // connect to the server, send the packet, and then read in the return packet.

    struct sockaddr_in serv_addr; // Server address data structure.
    struct hostent *server;      // Server data structure.

    sockfd = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP ); // Create a UDP socket.

    if ( sockfd < 0 )
        return 0;

    server = gethostbyname( ntp_server ); // Convert URL to IP.

    if ( server == NULL )
        return 0;

    // Zero out the server address structure.

    bzero( ( char* ) &serv_addr, sizeof( serv_addr ) );

    serv_addr.sin_family = AF_INET;

    // Copy the server's IP address to the server address structure.

    bcopy( ( char* )server->h_addr, ( char* ) &serv_addr.sin_addr.s_addr, server->h_length );

    // Convert the port number integer to network big-endian style and save it to the server address structure.

    serv_addr.sin_port = htons( portno );

    // Call up the server using its IP address and port number.

    if ( connect( sockfd, ( struct sockaddr * ) &serv_addr, sizeof( serv_addr) ) < 0 )
        return 0;

    // Send it the NTP packet it wants. If n == -1, it failed.

    n = write( sockfd, ( char* ) &packet, sizeof( ntp_packet ) );

    if ( n < 0 )
        return 0;

    // Wait and receive the packet back from the server. If n == -1, it failed.

    n = read( sockfd, ( char* ) &packet, sizeof( ntp_packet ) );

    if ( n < 0 )
        return 0;

    // These two fields contain the time-stamp seconds as the packet left the NTP server.
    // The number of seconds correspond to the seconds passed since 1900.
    // ntohl() converts the bit/byte order from the network's to host's "endianness".

    packet.txTm_s = ntohl( packet.txTm_s ); // Time-stamp seconds.
    packet.txTm_f = ntohl( packet.txTm_f ); // Time-stamp fraction of a second.

    // Extract the 32 bits that represent the time-stamp seconds (since NTP epoch) from when the packet left the server.
    // Subtract 70 years worth of seconds from the seconds since 1900.
    // This leaves the seconds since the UNIX epoch of 1970.
    // (1900)------------------(1970)**************************************(Time Packet Left the Server)

    time_t txTm = ( time_t ) ( packet.txTm_s - NTP_TIMESTAMP_DELTA );

    // for the fractional part see https://tickelton.gitlab.io/articles/ntp-timestamps/
    return txTm * NS_IN_SEC + (uint32_t)((double)packet.txTm_f * 1.0e9 / (double)(1LL << 32));
}


// Wait for the local ntp client to get synced if it isn't.
// Note that his might include the time needed for the network to even get properly started.
//
bool wait_for_local_ntp_client_started()
{
    for (int i = 1; i <= 60; i++)
    {
        if (!System::ntpSynced())
        {
            if (i % 5 == 0)
            {
                trace->info("waiting for ntp client sync - {} sec", i);
            }
        }
        else
        {
            trace->info("ntp client is ready");
            return true;
        }
        sleep(1);
    }
    return false;
}


bool wait_for_ntp_in_lock(const char* ntp_server, int timeout_sec)
{
    for(int secs = 0; secs < timeout_sec; secs++)
    {
        uint64_t ntp_ns = ntp_time_ns(ntp_server);

        if (ntp_ns)
        {
            double local_deviation_sec = ((int64_t) (ntp_ns - SystemTime::getWallClock_ns())) / NS_IN_SEC_F;

            if (fabs(local_deviation_sec) < 0.5)
            {
                trace->info("ntp vs local ntp client time deviation = {:.3f} sec - ok", local_deviation_sec);
                return true;
            }
            trace->info("ntp vs local ntp client time deviation = {:.3f} sec - retrying", local_deviation_sec);
            sleep(1);
        }
    }
    return false;
}


bool ntp_check(const char* ntp_server, int timeout_sec)
{
    if (!wait_for_local_ntp_client_started())
    {
        trace->error("giving up waiting for local ntp client sync");
        return false;
    }

    if (!wait_for_ntp_in_lock(ntp_server, timeout_sec))
    {
        trace->error("ntp client didn't get in lock");
        return false;
    }
    return true;
}
