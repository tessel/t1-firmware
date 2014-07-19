// Copyright 2014 Technical Machine, Inc. See the COPYRIGHT
// file at the top-level directory of this distribution.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#include <tm.h>

#include "hw.h"
#include "tm.h"
#include "host_spi.h"
#include "utility/nvmem.h"
#include "utility/wlan.h"
#include "utility/hci.h"
#include "utility/security.h"
#include "utility/netapp.h"
#include "utility/evnt_handler.h"


int hw_net__inuse_start ();
int hw_net__inuse_stop ();

#define CC3000_START hw_net__inuse_start();
#define CC3000_END hw_net__inuse_stop();

uint32_t tm_hostname_lookup (const uint8_t *hostname)
{
	if (!hw_net_online_status()) return 0;
	CC3000_START;

	unsigned long out_ip_addr = 0;

	/* get the host info */
	int err = 0;
	if ((err = gethostbyname((char *) hostname, strlen((char *) hostname), &out_ip_addr)) < 0) {
		TM_DEBUG("gethostbyname(): error %d", err);
		return 0;
	}
	CC3000_END;
	return (uint32_t) out_ip_addr;
}

tm_socket_t tm_udp_open ()
{
	if (!hw_net_online_status()) return -1;
	CC3000_START;

	int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	CC3000_END;
	return (tm_socket_t) sock;
}

int tm_udp_close (int ulSocket)
{
	if (!hw_net_online_status()) return -1;
	CC3000_START;

	closesocket(ulSocket);
	CC3000_END;
	return 0xFFFFFFFF;
}

int tm_udp_listen (int ulSocket, int port)
{
	if (!hw_net_online_status()) return 1;

	sockaddr localSocketAddr;
	localSocketAddr.sa_family = AF_INET;
	localSocketAddr.sa_data[0] = (port & 0xFF00) >> 8;
	localSocketAddr.sa_data[1] = (port & 0x00FF);
	localSocketAddr.sa_data[2] = 0;
	localSocketAddr.sa_data[3] = 0;
	localSocketAddr.sa_data[4] = 0;
	localSocketAddr.sa_data[5] = 0;

	// Bind socket
	CC3000_START;
	int sockStatus;
	if ( (sockStatus = bind(ulSocket, &localSocketAddr, sizeof(sockaddr)) ) != 0) {
		TM_ERR("Binding UDP socket failed: %d", sockStatus);
	}
	CC3000_END;
	return sockStatus;
}

int tm_udp_receive (int ulSocket, uint8_t *buf, unsigned long buf_len, uint32_t *ip)
{
	if (!hw_net_online_status()) return 0;
	CC3000_START;

	sockaddr from;
	socklen_t from_len;
	signed long ret = recvfrom(ulSocket, buf, buf_len, 0, &from, &from_len);
	char* aliasable_ip = (char*) &(from.sa_data[2]);
	*ip = *((uint32_t *) aliasable_ip);
	CC3000_END;
	if (ret <= 0) {
		TM_DEBUG("No data recieved");
	} else {
		TM_DEBUG("Recieved %d UDP bytes", (unsigned int) ret);
	}
	return ret;
}

int tm_udp_readable (tm_socket_t sock)
{
	if (!hw_net_online_status()) return 0;
    CC3000_START;

    fd_set readSet;        // Socket file descriptors we want to wake up for, using select()
    FD_ZERO(&readSet);
    FD_SET(sock, &readSet);
    struct timeval timeout;

    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    int rcount = select(sock+1, &readSet, (fd_set *) 0, (fd_set *) 0, &timeout );
    int flag = FD_ISSET(sock, &readSet);
    CC3000_END;

    (void) rcount;
    return flag;
}

int tm_udp_send (int ulSocket, uint8_t ip0, uint8_t ip1, uint8_t ip2, uint8_t ip3, int port, const uint8_t *buf, unsigned long buf_len)
{
	if (!hw_net_online_status()) return 0;

	sockaddr tSocketAddr;

	tSocketAddr.sa_family = AF_INET;
	tSocketAddr.sa_data[0] = (port & 0xFF00) >> 8;
	tSocketAddr.sa_data[1] = (port & 0x00FF);
	tSocketAddr.sa_data[2] = ip0;
	tSocketAddr.sa_data[3] = ip1;
	tSocketAddr.sa_data[4] = ip2;
	tSocketAddr.sa_data[5] = ip3;

	CC3000_START;
	int sent = sendto(ulSocket, buf, buf_len, 0, &tSocketAddr, sizeof(sockaddr));
//	TM_DEBUG("Sent udp packet %d to %d.%d.%d.%d:%d", sent, ip0, ip1, ip2, ip3, port);
	CC3000_END;
	return sent;
}

tm_socket_t tm_tcp_open ()
{
	if (!hw_net_online_status()) return -1;
	CC3000_START;

	int ulSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	uint16_t wAccept = SOCK_ON;
	setsockopt(ulSocket, SOL_SOCKET, SOCKOPT_ACCEPT_NONBLOCK, &wAccept, sizeof(wAccept)); // TODO this is duplicated in tm_tcp_listen
	unsigned long wRecvTimeout = 10;
	if (setsockopt(ulSocket, SOL_SOCKET, SOCKOPT_RECV_TIMEOUT, &wRecvTimeout, sizeof(wRecvTimeout)) != 0) {
		TM_LOG("setting recv_timeout failed.");
	}
	CC3000_END;
	return ulSocket;
}

int tm_tcp_close (tm_socket_t sock)
{
	if (!hw_net_online_status()) return -1;
	CC3000_START;

	closesocket(sock);
	CC3000_END;
	return 0;
}

int tm_tcp_connect (tm_socket_t sock, uint8_t ip0, uint8_t ip1, uint8_t ip2, uint8_t ip3, uint16_t port)
{
	if (!hw_net_online_status()) return 1;
	CC3000_START;

	// the family is always AF_INET
	sockaddr remoteSocketAddr;
	remoteSocketAddr.sa_family = AF_INET;
	remoteSocketAddr.sa_data[0] = (port & 0xFF00) >> 8;
	remoteSocketAddr.sa_data[1] = (port & 0x00FF);
	remoteSocketAddr.sa_data[2] = ip0;
	remoteSocketAddr.sa_data[3] = ip1;
	remoteSocketAddr.sa_data[4] = ip2;
	remoteSocketAddr.sa_data[5] = ip3;

	int lerr = connect(sock, &remoteSocketAddr, sizeof(sockaddr));
	if (lerr != ESUCCESS) {
		TM_DEBUG("Error connecting to TCP socket.");
	}
	CC3000_END;
	return lerr;
}

// http://publib.boulder.ibm.com/infocenter/iseries/v5r3/index.jsp?topic=%2Frzab6%2Frzab6xnonblock.htm

int tm_tcp_write (tm_socket_t sock, const uint8_t *buf, size_t buflen)
{
	if (!hw_net_online_status()) return 0;
	CC3000_START;

	int sentLen = send(sock, buf, buflen, 0);
	// TM_DEBUG("Wrote %d bytes to TCP socket.", sentLen);
	CC3000_END;
	return sentLen;
}

int tm_tcp_read (tm_socket_t sock, uint8_t *buf, size_t buflen)
{
	if (!hw_net_online_status()) return 0;

	// Limit buffer limit to 512 bytes to be reliable.
	if (buflen > 512) {
		buflen = 512;
	}
	
	CC3000_START;
	int read = recv(sock, buf, buflen, 0);
	CC3000_END;
	return read;
}

int tm_tcp_readable (tm_socket_t sock)
{
	if (!hw_net_online_status()) return 0;
    CC3000_START;

    // Socket file descriptors we want to wake up for, using select()
    fd_set readSet, errSet;
    FD_ZERO(&readSet);
    FD_ZERO(&errSet);
    FD_SET(sock, &readSet);
    FD_SET(sock, &errSet);
    struct timeval timeout;

    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    int rcount = select(sock+1, &readSet, (fd_set *) 0, &errSet, &timeout );
    int errflag = FD_ISSET(sock, &errSet);
    int flag = FD_ISSET(sock, &readSet);
    CC3000_END;

    (void) rcount;
    return errflag ? -1 : flag;
}

int tm_tcp_listen (tm_socket_t sock, uint16_t port)
{
	if (!hw_net_online_status()) return -1;
	CC3000_START;

	sockaddr localSocketAddr;
	localSocketAddr.sa_family = AF_INET;
	localSocketAddr.sa_data[0] = (port & 0xFF00) >> 8; //ascii_to_char(0x01, 0x01);
	localSocketAddr.sa_data[1] = (port & 0x00FF); //ascii_to_char(0x05, 0x0c);
	localSocketAddr.sa_data[2] = 0;
	localSocketAddr.sa_data[3] = 0;
	localSocketAddr.sa_data[4] = 0;
	localSocketAddr.sa_data[5] = 0;

	// Bind socket
	TM_DEBUG("Binding local socket...");
	int sockStatus;
	if ((sockStatus = bind(sock, &localSocketAddr, sizeof(sockaddr))) != 0) {
		TM_DEBUG("binding failed: %d", sockStatus);
		CC3000_END;
		return -1;
	}

	TM_DEBUG("Listening on local socket...");
	int listenStatus = listen(sock, 1);
	if (listenStatus != 0) {
		TM_DEBUG("cannot listen to socket: %d", listenStatus);
		CC3000_END;
		return -1;
	}

	char arg = SOCK_ON;
	if (setsockopt(sock, SOL_SOCKET, SOCKOPT_ACCEPT_NONBLOCK, &arg, sizeof(arg)) < 0) {
		TM_DEBUG("Couldn't set socket as non-blocking!");
		CC3000_END;
		return -1;
	}

	CC3000_END;
	return 0;
}

// Returns -1 on error or no socket.
// Returns -2 on pending connection.
// Returns >= 0 for socket descriptor.
tm_socket_t tm_tcp_accept (tm_socket_t sock, uint32_t *ip)
{
	if (!hw_net_online_status()) return -1;

	// the family is always AF_INET
	sockaddr addrClient;
	socklen_t addrlen;
	CC3000_START;
	int res = accept(sock, &addrClient, &addrlen);
	CC3000_END;

	char *aliasable_socket = (char*) &(addrClient.sa_data[2]);
	*ip = *((uint32_t *) aliasable_socket);
	return res;
}
