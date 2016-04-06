/*
 *  ssdplib.cpp
 *
 *  Created by Brad Christian on 3/9/11.
 *  Copyright 2011 Rose Point Navigation Systems. All rights reserved.
 *
 */

#include "ssdplib.h"

#ifndef _WIN32

#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define IPPROTO_IP 0
#define closesocket close

#define WSAGetLastError() (errno)

unsigned GetTickCount()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

#endif


#define SSDP_ADDRESS		"239.255.255.250"
#define SSDP_PORT           1900

#define	SSDP_BUFSIZE 2500

////////////////////////////////////////////////////////////////////////////

SOCKET SsdpCreateServerSocket()
{
	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == SOCKET_ERROR)
	{
		//fprintf(stderr, "Error %d creating the socket\n", WSAGetLastError());
		return INVALID_SOCKET;
	}
	
	{
		int bTrue = 1;
		if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &bTrue, sizeof (bTrue)) == SOCKET_ERROR)
		{
			//fprintf(stderr, "Error %d setting reuse address flag\n", WSAGetLastError());
			closesocket(sock);
			return INVALID_SOCKET;
		}
	}
	
	{
		struct sockaddr_in sock_addr;
		bzero(&sock_addr, sizeof (sock_addr));
		sock_addr.sin_family = AF_INET;
		sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		sock_addr.sin_port = htons(SSDP_PORT);
		
		// Binding the socket to the address
		if (bind(sock, (struct sockaddr*)&sock_addr, sizeof (sock_addr)) == SOCKET_ERROR)
		{
			//fprintf(stderr, "Error %d binding the socket\n", WSAGetLastError());
			closesocket(sock);
			return INVALID_SOCKET;
		}
	}
	
	{
		char bTrue = 1;
		if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &bTrue, sizeof (bTrue)) == SOCKET_ERROR)
		{
			//fprintf(stderr, "Warning: Could not set loopback - %d\n", WSAGetLastError());
		}
	}
	
	// Joining a multicast group
	{
		struct ip_mreq mreq;
		bzero(&mreq, sizeof (mreq));
		mreq.imr_multiaddr.s_addr = inet_addr(SSDP_ADDRESS);
		mreq.imr_interface.s_addr = htonl(INADDR_ANY);
		
		if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof (struct ip_mreq)) == SOCKET_ERROR)
		{
			//fprintf(stderr, "Error %d joining Multicast Group\n", WSAGetLastError());
			closesocket(sock);
			return INVALID_SOCKET;
		}
	}
	
	{
		char ttl = 4;
		if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof (ttl)) == SOCKET_ERROR)
		{
			//fprintf(stderr, "Error %d setting Multicast TTL\n", WSAGetLastError());
			closesocket(sock);
			return INVALID_SOCKET;
		}
	}
	
	return sock;
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

/////////////////////////////////////////////////////////////////////////////
//
// Each CSsdpService represents one known service that is either one of ours 
// (m_bOurs==1) or one that we've discovered on the netowrk (m_bOurs==0).


struct CSsdpService* g_pFirstService;

#ifdef _THREADED
pthread_rwlock_t g_lockServicelist;
#endif

void (*Ssdp_FoundNewService)(const char* szType, const char* szName, const char* szLocation, const char* szFriendlyName) = NULL;
void (*Ssdp_ByeByeService)(const char* szType, const char* szName) = NULL;
void (*Ssdp_ServiceChanged)(const char* szType, const char* szName, const char* szLocation) = NULL;

static void SsdpCreateService(const char* szType, const char* szName, const char* szLocation, const char* szFriendlyName, int bOurs)
{
//	fprintf(stderr, "SsdpCreateService: type: %s name: %s loc: %s\n", szType, szName, szLocation);
	
	CSsdpService* pService;
	
#ifdef _THREADED
	pthread_rwlock_wrlock(&g_lockServicelist);
#endif
	
	for (pService = g_pFirstService; pService != NULL; pService = pService->m_pNextService)
	{
		if (strcmp(pService->m_szType, szType) == 0 && strcmp(pService->m_szName, szName) == 0)
		{
			int bChanged = 0;
			if (strcmp(pService->m_szLocation, szLocation) != 0)
			{
				free(pService->m_szLocation);
				pService->m_szLocation = strdup(szLocation);
				bChanged = 1;
			}
			
			if (pService->m_bGone)
			{
				pService->m_bGone = 0;
				bChanged = 1;
			}
			
			pService->m_time = time(NULL);
			
#ifdef _THREADED
			pthread_rwlock_unlock(&g_lockServicelist);
#endif
			
			if (bChanged && Ssdp_ServiceChanged != NULL)
				Ssdp_ServiceChanged(szType, szName, szLocation);
			
			return;
		}
	}
	
	pService = (CSsdpService*)malloc(sizeof(CSsdpService));
	
	pService->m_bOurs = bOurs;
	pService->m_bGone = 0;
	pService->m_nMaxAge = 300; // TODO: This needs to come from the ssdp:alive message in the form of "cache-control: max-age = <n>"
	pService->m_szType = strdup(szType);
	pService->m_szName = strdup(szName);
	pService->m_szLocation = strdup(szLocation);
    if (szFriendlyName == NULL)
        szFriendlyName = "";
    pService->m_szFriendlyName = strdup(szFriendlyName);
	pService->m_time = time(NULL);
	pService->m_pNextService = NULL;
	
	CSsdpService** ppNextService;
	for (ppNextService = &g_pFirstService; *ppNextService != NULL; ppNextService = &(*ppNextService)->m_pNextService)
		;
	*ppNextService = pService;
	
	if (!bOurs && Ssdp_FoundNewService != NULL)
		Ssdp_FoundNewService(szType, szName, szLocation, szFriendlyName);
	
#ifdef _THREADED
	pthread_rwlock_unlock(&g_lockServicelist);
#endif
}

static void SsdpRemoveService(const char* szType, const char* szName)
{
	//	fprintf(stderr, "SsdpRemoveService: type: %s name: %s loc: %s\n", szType, szName, szLocation);
	
	CSsdpService* pService;
	
#ifdef _THREADED
	pthread_rwlock_wrlock(&g_lockServicelist);
#endif
	
	for (pService = g_pFirstService; pService != NULL; pService = pService->m_pNextService)
	{
		if (strcmp(pService->m_szType, szType) == 0 && strcmp(pService->m_szName, szName) == 0)
		{
			pService->m_bGone = 1;
			
#ifdef _THREADED
			pthread_rwlock_unlock(&g_lockServicelist);
#endif
			return;
		}
	}
	
#ifdef _THREADED
	pthread_rwlock_unlock(&g_lockServicelist);
#endif
}
			
////////////////////////////////////////////////////////////////////////////
//
// CSsdpTask represents a task that the SSDP server needs to perform, usually
// sometime in the future. Two types of tasks are defined. One is a simple
// timer that waits for a period of time and then calls the task's Main
// function. To create a repeating timer, the Main function should set a new
// m_nTickStart value. The other waits until a socket has data ready to read, 
// then it calls the task's Main. If the m_nTickDeadline is non-zero, then at 
// that point in time, the task's Timeout function is called and the task is
// marked as 'finished'.

struct CSsdpTask
{
	struct CSsdpTask* m_pNext;
	
	void (*Main)(struct CSsdpTask* pTask);
	void (*Timeout)(struct CSsdpTask* pTask);
	void (*Cleanup)(struct CSsdpTask* pTask);
	
	unsigned m_task : 4;
	unsigned m_bFinished : 1;
	unsigned m_nTickStart;
	unsigned m_nTickDeadline;
	SOCKET m_socket;
	void* m_pvData;
};

typedef struct CSsdpTask CSsdpTask;

// Values for CSsdpTask::m_task
#define SSDP_TASK_TIMER		0	// Simple Timer
#define SSDP_TASK_READ		1	// Read from a socket
#define SSDP_TASK_WRITE		2	// Write to a socket

void SsdpInitTask(CSsdpTask* pTask, unsigned task)
{
	pTask->m_pNext = NULL;
	pTask->m_task = task;
	pTask->m_bFinished = 0;
	pTask->Main = NULL;
	pTask->Timeout = NULL;
	pTask->Cleanup = NULL;
	pTask->m_nTickStart = 0;
	pTask->m_socket = INVALID_SOCKET;
	pTask->m_nTickDeadline = 0;
	pTask->m_pvData = NULL;
}

/////////////////////////////////////////////////////////////////////////////

struct CSsdpServer
{
	CSsdpTask* m_pFirstTask;
};

struct CSsdpServer g_theSsdpServer;

void SsdpAddTask(struct CSsdpServer* pServer, CSsdpTask* pTask)
{
	pTask->m_pNext = pServer->m_pFirstTask;
	pServer->m_pFirstTask = pTask;
}

////////////////////////////////////////////////////////////////////////////
//
// A CNotifyTask is created for each one of our services when we need to
// send out a notification (such as "alive"). It will delay a random
// period of time then send a response describing our service and then
// repeat once.

struct CNotifyTask
{
	struct CSsdpTask m_task;
	
	int m_nRepeat;
	struct sockaddr_in m_addr;
	struct CSsdpService* m_pService;
	const char* m_szNTS;
};

typedef struct CNotifyTask CNotifyTask;

void NotifyTask_Main(CSsdpTask* pBaseTask)
{
	CNotifyTask* pTask = (CNotifyTask*)pBaseTask;
	
	time_t t;
	time(&t);
	
	char szTime [32];
	strftime(szTime, sizeof(szTime), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&t));
	
	char szResponse [2048];
	int cch = sprintf(szResponse, "NOTIFY * HTTP/1.1\r\n"
					  "DATE: %s\r\n"
					  "SERVER: RosePoint UPnP/1.0\r\n"
					  "HOST: 239.255.255.250:1900\r\n"
					  "NTS: %s\r\n"
					  "CACHE-CONTROL: max-age=300\r\n"
					  "NT: %s\r\n"
					  "USN: %s\r\n"
					  "LOCATION: %s\r\n"
					  "CONTENT-LENGTH: 0\r\n"
					  "\r\n",
					  szTime, pTask->m_szNTS, 
					  pTask->m_pService->m_szType, pTask->m_pService->m_szName, pTask->m_pService->m_szLocation);
	
	if (sendto(pTask->m_task.m_socket, szResponse, cch, 0, (struct sockaddr*)&pTask->m_addr, sizeof (pTask->m_addr)) == SOCKET_ERROR)
	{
	}
	
	pTask->m_nRepeat -= 1;
	if (pTask->m_nRepeat <= 0)
		pTask->m_task.m_bFinished = 1;
	else
		pTask->m_task.m_nTickStart = GetTickCount() + rand() % 25;
}

void NotifyTask_Cleanup(CSsdpTask* pTask)
{
	if (pTask->m_socket != INVALID_SOCKET)
		closesocket(pTask->m_socket);
}

CSsdpTask* CreateNotifyTask(const char* szNTS, CSsdpService* pService)
{
	CNotifyTask* pTask = (CNotifyTask*)malloc(sizeof(CNotifyTask));
	SsdpInitTask((CSsdpTask*)pTask, SSDP_TASK_WRITE);
	pTask->m_task.Main = NotifyTask_Main;
	pTask->m_task.Cleanup = NotifyTask_Cleanup;
	pTask->m_task.m_nTickStart = GetTickCount() + rand() % 25;
	pTask->m_task.m_socket = SsdpCreateSimpleSocket();
	pTask->m_nRepeat = 2;
	SsdpInitAddr(&pTask->m_addr);
	pTask->m_szNTS = szNTS;
	pTask->m_pService = pService;
	
	return (CSsdpTask*)pTask;
}

////////////////////////////////////////////////////////////////////////////
//
// A CSearchResponseTask is created for each one of our services that match
// the requested type when we receive an M-SEARCH. It will delay a random
// period of time then send a response describing our service and then
// repeat once.

struct CSearchResponseTask
{
	struct CSsdpTask m_task;
	
	int m_nRepeat;
	struct sockaddr_in m_addr;
	struct CSsdpService* m_pService;
};

typedef struct CSearchResponseTask CSearchResponseTask;

void SearchResponseTask_Main(CSsdpTask* pBaseTask)
{
	CSearchResponseTask* pTask = (CSearchResponseTask*)pBaseTask;
	
	time_t t;
	time(&t);
	
	char szTime [32];
	strftime(szTime, sizeof(szTime), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&t));
	
	char szResponse [2048];
	int cch = sprintf(szResponse, "HTTP/1.1 200 OK\r\n"
					  "CACHE-CONTROL: max-age=1800\r\n"
					  "DATE: %s\r\n"
					  "EXT:\r\n"
					  "LOCATION: %s\r\n"
					  "SERVER: RosePoint UPnP/1.0\r\n"
					  "ST:%s\r\n"
					  "USN:%s\r\n"
					  //"CONTENT-LENGTH: 0\r\n"
					  "\r\n",
					  szTime, 
					  pTask->m_pService->m_szLocation, pTask->m_pService->m_szType, pTask->m_pService->m_szName);
	
	if (sendto(pTask->m_task.m_socket, szResponse, cch, 0, (struct sockaddr*)&pTask->m_addr, sizeof (pTask->m_addr)) == -1)
	{
		// REVIEW: Errors?
	}
	
	pTask->m_nRepeat -= 1;
	if (pTask->m_nRepeat <= 0)
		pTask->m_task.m_bFinished = 1;
	else
		pTask->m_task.m_nTickStart = GetTickCount() + rand() % 25;
}

CSsdpTask* CreateSearchResponseTask(SOCKET sock, struct sockaddr* paddr, CSsdpService* pService)
{
	CSearchResponseTask* pTask = (CSearchResponseTask*)malloc(sizeof(CSearchResponseTask));
	SsdpInitTask((CSsdpTask*)pTask, SSDP_TASK_WRITE);
	pTask->m_task.Main = SearchResponseTask_Main;
	pTask->m_task.m_nTickStart = GetTickCount() + rand() % 25;
	pTask->m_task.m_socket = sock;
	pTask->m_nRepeat = 2;
	memcpy(&pTask->m_addr, paddr, sizeof (pTask->m_addr));
	pTask->m_pService = pService;
	
	return (CSsdpTask*)pTask;
}

/////////////////////////////////////////////////////////////////////////////

static void SsdpHandlePacket(SOCKET sock, char* szPacket, struct sockaddr* paddr, int naddrlen)
{
	const char* szFirstLine = NULL;
	
	const char* szNTS = NULL;
	const char* szLocation = NULL;
	const char* szType = NULL;
	const char* szName = NULL;
    const char* szFriendlyName = NULL;
	
	char* szLine = szPacket;
	while (*szLine != '\0')
	{
		char* pch = strchr(szLine, '\r');
		if (pch == NULL)
		{
			pch = strchr(szLine, '\n');
			if (pch == NULL)
				pch = szLine + strlen(szLine);
			else
				*pch++ = '\0';
		}
		else
		{
			*pch++ = '\0';
			
			if (*pch == '\n')
				pch += 1;
		}
		
		if (szFirstLine == NULL)
		{
			szFirstLine = szLine;
		}
		else
		{
			char* pchColon = strchr(szLine, ':');
			if (pchColon != NULL)
			{
				int cchToken = (int)(pchColon - szLine);
				char* szValue = pchColon + 1;
				while (*szValue == ' ')
					szValue += 1;
				
				if (cchToken == 3 && strncasecmp(szLine, "NTS", 3) == 0)
					szNTS = szValue;
				else if (cchToken == 8 && strncasecmp(szLine, "LOCATION", 8) == 0)
					szLocation = szValue;
				else if (cchToken == 2 && strncasecmp(szLine, "AL", 2) == 0)
					szLocation = szValue;
				else if (cchToken == 3 && strncasecmp(szLine, "USN", 3) == 0)
					szName = szValue;
				else if (cchToken == 2 && strncasecmp(szLine, "NT", 2) == 0)
					szType = szValue;
				else if (cchToken == 2 && strncasecmp(szLine, "ST", 2) == 0)
					szType = szValue;
                else if (cchToken == 4 && strncasecmp(szLine, "X-FN", 4) == 0)
                    szFriendlyName = szValue;
			}
		}
		
		szLine = pch;
	}
	
	if (strncasecmp(szFirstLine, "HTTP/1.1 200 OK", 15) == 0)
	{
		if (szType != NULL && szLocation != NULL)
		{
			if (szName == NULL)
				szName = "";
			
			SsdpCreateService(szType, szName, szLocation, szFriendlyName, 0);
		}
	}
	else if (strncasecmp(szFirstLine, "NOTIFY ", 7) == 0)
	{
		if (szNTS != NULL)
		{
			if (strcasecmp(szNTS, "ssdp:alive") == 0 || strcasecmp(szNTS, "\"ssdp:alive\"") == 0)
			{
				if (szType != NULL && szLocation != NULL)
				{
					if (szName == NULL)
						szName = "";
					
					SsdpCreateService(szType, szName, szLocation, szFriendlyName, 0);
				}
			}
			else if (strcasecmp(szNTS, "ssdp:byebye") == 0 || strcasecmp(szNTS, "\"ssdp:byebye\"") == 0)
			{
				SsdpRemoveService(szType, szName);
				
				if (Ssdp_ByeByeService != NULL)
					Ssdp_ByeByeService(szType, szName);
			}
		}
	}
	else if (strncasecmp(szFirstLine, "M-SEARCH ", 9) == 0)
	{
		if (szType != NULL)
		{
#ifdef _THREADED
			pthread_rwlock_rdlock(&g_lockServicelist);
#endif
			
			for (struct CSsdpService* pService = g_pFirstService; pService != NULL; pService = pService->m_pNextService)
			{
				if (pService->m_bOurs && (strcmp(pService->m_szType, szType) == 0 /*|| strcmp(szType, "ssdp:all") == 0 || strcmp(szType, "upnp:rootdevice") == 0*/))
				{
					// NOTE: This does keep track of pService until after the lock is released, 
					// but that is okay because we never delete or change the services we own.
					
					SsdpAddTask(&g_theSsdpServer, CreateSearchResponseTask(sock, paddr, pService));
				}
			}
			
#ifdef _THREADED
			pthread_rwlock_unlock(&g_lockServicelist);
#endif
		}
	}
}

static void SendNotify(const char* szNTS)
{
#ifdef _THREADED
	pthread_rwlock_rdlock(&g_lockServicelist);
#endif
	
	for (struct CSsdpService* pService = g_pFirstService; pService != NULL; pService = pService->m_pNextService)
	{
		if (pService->m_bOurs)
		{
			SsdpAddTask(&g_theSsdpServer, CreateNotifyTask(szNTS, pService));
		}
	}
	
#ifdef _THREADED
	pthread_rwlock_unlock(&g_lockServicelist);
#endif
}

////////////////////////////////////////////////////////////////////////////
//
// The ServerTask continously listens for SSDP packets and handles any
// that it receives.

void ServerTask_Main(CSsdpTask* pTask)
{
	char buf [SSDP_BUFSIZE];
	
	struct sockaddr_in new_sock_addr;
	socklen_t new_sock_addr_len = sizeof (new_sock_addr);
	
	int bytes = recvfrom(pTask->m_socket, buf, SSDP_BUFSIZE, 0, (struct sockaddr*)&new_sock_addr, &new_sock_addr_len);
	if (bytes == SOCKET_ERROR)
	{
		closesocket(pTask->m_socket);
		pTask->m_socket = INVALID_SOCKET;
		// REVIEW: Need to recreate the socket now or everything will stop working!
		return;
	}
	
	SsdpHandlePacket(pTask->m_socket, buf, (struct sockaddr*)&new_sock_addr, new_sock_addr_len);
}

CSsdpTask* CreateSsdpServerTask()
{
	CSsdpTask* pTask = (CSsdpTask*)malloc(sizeof(CSsdpTask));
	SsdpInitTask(pTask, SSDP_TASK_READ);
	pTask->m_socket = SsdpCreateServerSocket();
	pTask->Main = ServerTask_Main;
	return pTask;
}

////////////////////////////////////////////////////////////////////////////
//
// The SearchTask handles responses to M-SEARCH queries that we send.
// The task will keep the socket open and handle any message it receives
// for four seconds and then shut down.

void SearchTask_Main(CSsdpTask* pTask)
{
	char buf [1000];
	int cch = recv(pTask->m_socket, buf, sizeof (buf), 0);
	if (cch != -1)
		SsdpHandlePacket(pTask->m_socket, buf, NULL, 0);
}

void SearchTask_Cleanup(CSsdpTask* pTask)
{
	if (pTask->m_socket != INVALID_SOCKET)
		closesocket(pTask->m_socket);
}

CSsdpTask* CreateSearchTask(SOCKET socket)
{
	CSsdpTask* pTask = (CSsdpTask*)malloc(sizeof(CSsdpTask));
	SsdpInitTask(pTask, SSDP_TASK_READ);
	pTask->m_socket = socket;
	pTask->m_nTickDeadline = GetTickCount() + 4000;	
	pTask->Main = SearchTask_Main;
	pTask->Cleanup = SearchTask_Cleanup;
	return pTask;
}

////////////////////////////////////////////////////////////////////////////

CSsdpTask* SsdpCreateTimerTask(unsigned nTickDelay, void (*fnMain)(CSsdpTask*))
{
	CSsdpTask* pTask = (CSsdpTask*)malloc(sizeof(CSsdpTask));
	SsdpInitTask(pTask, SSDP_TASK_TIMER);
	pTask->Main = fnMain;
	pTask->m_nTickStart = GetTickCount() + nTickDelay;
	return pTask;
}

////////////////////////////////////////////////////////////////////////////

void AliveTask_Main(CSsdpTask* pTask)
{
	pTask->m_nTickStart = GetTickCount() + 30000;
	SendNotify("ssdp:alive");
}

////////////////////////////////////////////////////////////////////////////

int SsdpPump(/*struct CSsdpServer* pServer, */unsigned nMaxTimeout)
{
	struct CSsdpServer* pServer = &g_theSsdpServer;
	
	unsigned nTick = GetTickCount();
	
	fd_set reading;
	FD_ZERO(&reading);
	
	fd_set writing;
	FD_ZERO(&writing);
	
	unsigned nMinTimeout = 1000000;
	
	for (CSsdpTask* pTask = pServer->m_pFirstTask; pTask != NULL; pTask = pTask->m_pNext)
	{
		switch (pTask->m_task)
		{
		case SSDP_TASK_READ:
			if (pTask->m_socket != INVALID_SOCKET && nTick >= pTask->m_nTickStart)
			{
				if (pTask->m_nTickDeadline != 0)
				{
					if (nTick >= pTask->m_nTickDeadline)
					{
						pTask->m_bFinished = 1;
						if (pTask->Timeout != NULL)
							pTask->Timeout(pTask);
					}
					else
					{
						unsigned timeout = pTask->m_nTickDeadline - nTick;
						if (nMinTimeout > timeout)
							nMinTimeout = timeout;
					}
				}
				
				if (!pTask->m_bFinished)
					FD_SET(pTask->m_socket, &reading);
			}
			break;
				
		case SSDP_TASK_WRITE:
			if (pTask->m_socket != INVALID_SOCKET && nTick >= pTask->m_nTickStart)
			{
				if (!pTask->m_bFinished)
					FD_SET(pTask->m_socket, &writing);
			}
			break;
		}
	}
	
	if (nMinTimeout > nMaxTimeout)
		nMinTimeout = nMaxTimeout;
	
	struct timeval tv;
	tv.tv_sec = nMinTimeout / 1000;
	tv.tv_usec = (nMinTimeout % 1000) * 1000;

	int s = select(FD_SETSIZE, &reading, &writing, NULL, &tv);
	
	if (s == SOCKET_ERROR)
	{
		return 0;
	}
	
	for (CSsdpTask* pTask = pServer->m_pFirstTask; pTask != NULL; pTask = pTask->m_pNext)
	{
		switch (pTask->m_task)
		{
		case SSDP_TASK_TIMER:
			if (pTask->m_nTickStart != 0 && nTick >= pTask->m_nTickStart)
			{
				pTask->m_nTickStart = 0;
				pTask->Main(pTask);
				pTask->m_bFinished = pTask->m_nTickStart == 0;
			}
			break;
				
		case SSDP_TASK_READ:
			if (s > 0 && pTask->m_socket != INVALID_SOCKET && nTick >= pTask->m_nTickStart && FD_ISSET(pTask->m_socket, &reading))
			{
				pTask->m_nTickStart = 0;
				pTask->Main(pTask);
			}
			break;
				
		case SSDP_TASK_WRITE:
			if (s > 0 && pTask->m_socket != INVALID_SOCKET && nTick >= pTask->m_nTickStart && FD_ISSET(pTask->m_socket, &writing))
			{
				pTask->m_nTickStart = 0;
				pTask->Main(pTask);
			}
			break;
		}
	}
	
	// Cleanup completed tasks
	{
		CSsdpTask** ppTask = &pServer->m_pFirstTask;		
		for (;;)
		{
			CSsdpTask* pTask = *ppTask;
			if (pTask == NULL)
				break;
			
			if (pTask->m_bFinished)
			{
				*ppTask = pTask->m_pNext;
				
				pTask->m_pNext = NULL;
				if (pTask->Cleanup != NULL)
					pTask->Cleanup(pTask);
				free(pTask);
			}
			else
			{
				ppTask = &pTask->m_pNext;
			}
		}
	}
	
	// TODO: Should also check for lost services!
	
	return 1;
}

////////////////////////////////////////////////////////////////////////////

static int SsdpSend(char* pchPacket, int cchPacket, int nSeconds)
{
	SOCKET sock = SsdpCreateSimpleSocket();
	
	struct sockaddr_in dst_sock_addr;
	SsdpInitAddr(&dst_sock_addr);
	
	if (sendto(sock, pchPacket, cchPacket, 0, (struct sockaddr*)&dst_sock_addr, sizeof (dst_sock_addr)) == SOCKET_ERROR)
	{
		//fprintf(stderr, "Error in Sending data on the socket - %d\n", WSAGetLastError());
	}
	
	usleep(1000);
	
	if (sendto(sock, pchPacket, cchPacket, 0, (struct sockaddr*)&dst_sock_addr, sizeof (dst_sock_addr)) == SOCKET_ERROR)
	{
		//fprintf(stderr, "Error in Sending data on the socket - %d\n", WSAGetLastError());
	}
	
	if (nSeconds > 0)
	{
		SsdpAddTask(&g_theSsdpServer, CreateSearchTask(sock));
	}
	else
	{
		closesocket(sock);
	}
	
	return 1;
}

////////////////////////////////////////////////////////////////////////////

#ifdef _THREADED
void* SsdpServerProc(void* pv)
{
	for (;;)
	{
		if (!SsdpPump(1000))
			break;
	}
	
	return NULL;
}
#endif

int SsdpInitialize()
{
#ifdef _THREADED
	pthread_rwlock_init(&g_lockServicelist, NULL);
#endif
	
	g_theSsdpServer.m_pFirstTask = NULL;
	
	SsdpAddTask(&g_theSsdpServer, CreateSsdpServerTask());
	SsdpAddTask(&g_theSsdpServer, SsdpCreateTimerTask(30000, AliveTask_Main));
	
#ifdef _THREADED
	pthread_t thread;
	pthread_create(&thread, NULL, SsdpServerProc, NULL);
#endif
	
	return 1;
}

int SsdpStartSearch(const char* szType, int mx/*=3*/)
{
	char buf [SSDP_BUFSIZE];
	char* pch = buf + sprintf(buf, "M-SEARCH * HTTP/1.1\r\n");
	pch += sprintf(pch, "Host:%s:%d\r\n", SSDP_ADDRESS, SSDP_PORT);
	pch += sprintf(pch, "MAN:\"ssdp:discover\"\r\n");
	pch += sprintf(pch, "ST:%s\r\n", szType);
	pch += sprintf(pch, "MX:%d\r\n", mx);
	pch += sprintf(pch, "Server: RosePoint UPnP/1.0\r\n");
	//	pch += sprintf(pch, "Content-Length:0\r\n");
	pch += sprintf(pch, "\r\n");
	
	return SsdpSend(buf, (int)(pch - buf), mx);
}

int SsdpAnnounce(const char* szType, const char* szName, const char* szLocation)
{
	SsdpCreateService(szType, szName, szLocation, NULL, 1);
	
	char buf [SSDP_BUFSIZE];
	char* pch = buf + sprintf(buf, "NOTIFY * HTTP/1.1\r\n");
	pch += sprintf(pch, "HOST: %s:%d\r\n", SSDP_ADDRESS, SSDP_PORT);
	pch += sprintf(pch, "NTS: ssdp:alive\r\n");
	pch += sprintf(pch, "NT: %s\r\n", szType);
	pch += sprintf(pch, "USN: %s\r\n", szName);
	pch += sprintf(pch, "LOCATION: %s\r\n", szLocation);
	pch += sprintf(pch, "CACHE-CONTROL: max-age=300\r\n");
	pch += sprintf(pch, "SERVER: RosePoint UPnP/1.0\r\n");
	pch += sprintf(pch, "CONTENT-LENGTH: 0\r\n");
	pch += sprintf(pch, "\r\n");
	
	return SsdpSend(buf, (int)(pch - buf), 0);
}


