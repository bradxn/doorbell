
// ssdp.cpp : Defines the entry point for the console application.
//


#ifdef _WIN32
#include "stdafx.h"
#include <stdio.h>
#else
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#define TCHAR char
#define DWORD unsigned long
#define WORD unsigned short
#define ULONG unsigned long
#define UINT unsigned int
#define LPVOID void*
#define BOOL bool
#define TRUE true
#define SOCKET int
#define SOCKET_ERROR -1
#define INVALID_SOCKET -1
#define closesocket close

#define WSAGetLastError() (errno)
#define ZeroMemory(pb,cb) memset(pb, 0, cb)
#endif

#ifdef _WIN32
TCHAR szLocalHost [256];
DWORD ipLocalHost;
#endif

bool g_bNotifications = false;

/*
class CHttpMessage
{
public:
	CString m_strRequestOrStatusLine;
	CMapStringToString m_headers;
};

class CHttpRequest : public CHttpMessage
{
public:
	CString m_strAddress; // the IP address of the client
};
*/

#define	SSDP_BUFSIZE 2500
#define SSDP_ADDRESS "239.255.255.250"
#define SSDP_PORT 1900 // This is the standard SSDP port

UINT SsdpServerProc(LPVOID lpParam)
{
	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == SOCKET_ERROR)
	{
		fprintf(stderr, "Error %d creating the socket\n", WSAGetLastError());
		return 1;
	}

	{
		int bTrue = 1;
		if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&bTrue, sizeof (bTrue)) == SOCKET_ERROR)
		{
			fprintf(stderr, "Error %d setting reuse address flag\n", WSAGetLastError());
			return 1;
		}
	}

	{
		struct sockaddr_in sock_addr;
		ZeroMemory(&sock_addr, sizeof (sock_addr));
		sock_addr.sin_family = AF_INET;
		sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);//inet_addr(SSDP_ADDRESS);
		sock_addr.sin_port = htons(SSDP_PORT);

		// Binding the socket to the address
		if (bind(sock, (struct sockaddr*)&sock_addr, sizeof (sock_addr)) == SOCKET_ERROR)
		{
			fprintf(stderr, "Error %d binding the socket\n", WSAGetLastError());
			return 1;
		}
	}

	// Joining a multicast group
	{
		struct ip_mreq mreq;
		ZeroMemory(&mreq, sizeof (mreq));
		mreq.imr_multiaddr.s_addr = inet_addr(SSDP_ADDRESS);
		mreq.imr_interface.s_addr = htonl(INADDR_ANY);

		if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof (struct ip_mreq)) == SOCKET_ERROR)
		{
			fprintf(stderr, "Error %d joining Multicast Group\n", WSAGetLastError());
			return 1;
		}
	}

	{
		DWORD bTrue = 1;
		if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, (char*)&bTrue, sizeof (bTrue)) == SOCKET_ERROR)
		{
			fprintf(stderr, "Warning: Could not set loopback - %d\n", WSAGetLastError());
		}
	}

#ifdef _WIN32
	{
		DWORD bTrue = 1;
		WSAIoctl(sock, SIO_MULTIPOINT_LOOPBACK, NULL, 0, &bTrue, sizeof (bTrue), NULL, NULL, NULL);
	}
#endif

	{
		char ttl = 4;
		if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof (ttl)) == SOCKET_ERROR)
		{
			fprintf(stderr, "Error %d setting Multicast TTL\n", WSAGetLastError());
			return 1;
		}
	}

	for (;;)
	{
		char buf [SSDP_BUFSIZE];

//		printf("Waiting for a packet...\n");

		struct sockaddr_in new_sock_addr;
		socklen_t new_sock_addr_len = sizeof (new_sock_addr);

		socklen_t bytes = recvfrom(sock, buf, SSDP_BUFSIZE, 0, (struct sockaddr*)&new_sock_addr, &new_sock_addr_len);
		if (bytes == INVALID_SOCKET)
		{
			fprintf(stderr, "Error in receiving data - %d\n", WSAGetLastError());
			continue;
		}

		buf[bytes] = '\0';
//		printf("Received from %s:\n%s\n", inet_ntoa(new_sock_addr.sin_addr), buf);

		char* pch = buf;
		while (*pch != '\0')
		{
			char* pch2 = strchr(pch, '\r');
			if (pch2 == NULL)
			{
				pch2 = strchr(pch, '\n');
				if (pch2 == NULL)
					pch2 = pch + strlen(pch);
				else
					*pch2++ = '\0';
			}
			else
			{
				*pch2++ = '\0';
				if (*pch2 == '\n')
					pch2 += 1;
			}
/*			
            if (strncasecmp(pch, "NTS:", 4) == 0)
            {
                pch += 4;
                while (*pch == ' ')
                    pch += 1;
                if (strcmp(pch, "doorbell:ring") == 0)
                {
                    printf("ding dong!\n");
                    system("mplayer Doorbell.mp4a");
                }
            }
*/
            if (strncasecmp(pch, "NT:", 3) == 0)
            {
                pch += 3;
                while (*pch == ' ')
                    pch += 1;
                if (strcmp(pch, "house:ring-doorbell") == 0)
                {
                    printf("ding dong!\n");
                    system("mplayer Doorbell.m4a");
					break;
                }
            }

			pch = pch2;
		}

/*
		char szResponse [] = 
			"HTTP/1.1 200 OK\r\n"
			"Date: \r\n"
			"Server: Unknown/0.0 UPnP/1.0 RosePointNav-CE/1.1\r\n"
			"EXT: \r\n"
			"LOCATION: http://rosepointnav.com/default.htm\r\n"
			"CACHE-CONTROL: max-age=1830\r\n"
			"ST: urn:coastalexplorer-net:service:CoastalExplorer:1\r\n"
			"USN: \r\n"
			"\r\n";

		sendto(sock, szResponse, strlen(szResponse), MSG_DONTROUTE, (struct sockaddr*)&new_sock_addr, sizeof (new_sock_addr));
*/
	}

	return 0;
}

SOCKET SsdpCreateSimpleSocket()
{
	SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (sock == INVALID_SOCKET)
	{
		fprintf(stderr, "Error creating the socket - %d\n", errno);
		return INVALID_SOCKET;
	}
	
	{
		unsigned char ttl = 4;
		if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof (ttl)) == SOCKET_ERROR)
		{
			fprintf(stderr, "Warning: Could not set ttl - %d\n", errno);
		}
	}
	
	{
		unsigned char bTrue = 1;
		if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &bTrue, sizeof (bTrue)) == SOCKET_ERROR)
		{
			fprintf(stderr, "Warning: Could not set loopback - %d\n", errno);
		}
	}
	
	return sock;
}

void SsdpInitAddr(struct sockaddr_in* paddr)
{
	bzero(paddr, sizeof (struct sockaddr_in));
	paddr->sin_family = PF_INET;
	paddr->sin_addr.s_addr = inet_addr(SSDP_ADDRESS);
	paddr->sin_port = htons(SSDP_PORT);
}

static bool SsdpSend(char* pchPacket, int cchPacket)
{
	SOCKET sock = SsdpCreateSimpleSocket();
	if (sock == SOCKET_ERROR)
	{
		fprintf(stderr, "Error creating the socket - %d\n", WSAGetLastError());
		return false;
	}
    
/*
#ifndef _WIN32
    {
        struct sockaddr_in saddr;
        ZeroMemory(&saddr, sizeof(saddr));
        saddr.sin_family = AF_INET;
        saddr.sin_port = htons(0);
        saddr.sin_addr.s_addr = htonl(INADDR_ANY);
        if (bind(sock, (struct sockaddr*)&saddr, sizeof(saddr)) < 0)
            fprintf(stderr, "Bind failed %d\n", WSAGetLastError());
    }
#endif
*/
/*
    {
        DWORD addr = INADDR_ANY;//ipLocalHost;
        if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, (char*)&addr, sizeof (addr)) == SOCKET_ERROR)
        {
            fprintf(stderr, "Warning: Could not set IP_MULTICAST_IF - %d\n", WSAGetLastError());
        }
    }
*/
    
	{
		struct sockaddr_in dst_sock_addr;
        SsdpInitAddr(&dst_sock_addr);

//		for (int i = 0; i < 3; i += 1)
		{
			if (sendto(sock, pchPacket, cchPacket, 0, (struct sockaddr*)&dst_sock_addr, sizeof (dst_sock_addr)) == SOCKET_ERROR)
			{
				fprintf(stderr, "Error in Sending data on the socket - %d\n", WSAGetLastError());
			}

/*
			for (;;)
			{
				fd_set reading;
				FD_ZERO(&reading);
				FD_SET(sock, &reading);

				timeval tv;
				tv.tv_sec = 5;
				tv.tv_usec = 0;

				int s = select((int)sock + 1, &reading, NULL, NULL, &tv);
				if (s == SOCKET_ERROR)
				{
					fprintf(stderr, "Select failed!\n");
					break;
				}
				else if (s == 0)
				{
					// timeout
					fprintf(stderr, "Timeout!\n");
					break;
				}
				else
				{
					char buf [1000];
					int cch = recv(sock, buf, sizeof (buf), 0);
					buf[cch] = 0;
					printf("recv'd: %s\n", buf);
				}
			}
*/
		}
	}

	closesocket(sock);

	return true;
}

bool SsdpStartSearch(const char* szType, int mx/*=5*/)
{
	char buf [SSDP_BUFSIZE];
	char* pch = buf + sprintf(buf, "M-SEARCH * HTTP/1.1\r\n");
	pch += sprintf(pch, "HOST: %s:%d\r\n", SSDP_ADDRESS, SSDP_PORT);
	pch += sprintf(pch, "MAN: \"ssdp:discover\"\r\n");
	pch += sprintf(pch, "ST: %s\r\n", szType);
	if (mx > 0)
		pch += sprintf(pch, "MX: %d\r\n", mx);
	pch += sprintf(pch, "\r\n");

	printf("sending:\n%s\n", buf);

	return SsdpSend(buf, int(pch - buf));
}

static bool SsdpNotify(const char* szType, const char* szSubType)
{
	char buf [SSDP_BUFSIZE];
	char* pch = buf + sprintf(buf, "NOTIFY * HTTP/1.1\r\n");
	pch += sprintf(pch, "HOST: %s:%d\r\n", SSDP_ADDRESS, SSDP_PORT);
	pch += sprintf(pch, "NT: %s\r\n", szType);
    if (szSubType != NULL)
    	pch += sprintf(pch, "NTS: %s\r\n", szSubType);
	pch += sprintf(pch, "\r\n");

	return SsdpSend(buf, int(pch - buf));
}


static bool SsdpAnnounce(const char* szType, const char* szName, const char* szLocation)
{
	char buf [SSDP_BUFSIZE];
	char* pch = buf + sprintf(buf, "NOTIFY * HTTP/1.1\r\n");
	pch += sprintf(pch, "HOST: %s:%d\r\n", SSDP_ADDRESS, SSDP_PORT);
	pch += sprintf(pch, "NTS: \"ssdp:alive\"\r\n");
	pch += sprintf(pch, "NT: %s\r\n", szType);
	pch += sprintf(pch, "USN: %s\r\n", szName);
	pch += sprintf(pch, "AL: %s\r\n", szLocation);
	// TODO: Cache-Control header...
	pch += sprintf(pch, "\r\n");

	printf("SsdpAnnounce:\n%s", buf);
	return SsdpSend(buf, int(pch - buf));
}


int main(int argc, char* argv[])
{
#ifdef _WIN32
	WORD wVersionRequested = MAKEWORD(2, 0);
	WSADATA wsaData;
	int err = WSAStartup(wVersionRequested, &wsaData);

	{
		gethostname(szLocalHost, sizeof(szLocalHost));
		struct hostent* he = gethostbyname(szLocalHost);
		if (he == NULL)
		{
			ULONG addr = inet_addr(szLocalHost);
			if (addr == INADDR_NONE)
				return false;

			he = gethostbyaddr((const char*)&addr, sizeof (addr), AF_INET);
			if (he == NULL)
				return false;
		}

		char* addr0 = he->h_addr_list[0];
		ipLocalHost = *(UINT*)addr0;
	}
#endif

	while (argc > 1 && argv[1][0] == '-')
	{
		if (argv[1][1] == 'n')
		{
			g_bNotifications = true;
		}

		argc -= 1;
		argv += 1;
	}

	if (argc >= 3)
	{
		if (strcasecmp(argv[1], "notify") == 0)
		{
            const char* szNT = argv[2];
            const char* szNTS = NULL;
            if (argc == 4)
                szNTS = argv[3];
                
			SsdpNotify(szNT, szNTS);
		}
	}
	else if (argc == 2)
		SsdpStartSearch(argv[1], 5);
	else
		SsdpServerProc(0);

	return 0;
}
