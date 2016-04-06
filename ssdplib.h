/*
 *  ssdplib.h
 *
 *  Created by Brad Christian on 3/9/11.
 *  Copyright 2011 Rose Point Navigation Systems. All rights reserved.
 *
 */

#define _THREADED

#ifdef _THREADED
#include <pthread.h>
#endif
#include <string.h>
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

#ifdef __cplusplus
extern "C" {
#endif
	
struct CSsdpService
{
	struct CSsdpService* m_pNextService;
	
	unsigned m_bOurs : 1;
	unsigned m_bGone : 1;
	int m_nMaxAge;
	time_t m_time;
	char* m_szType;
	char* m_szName;
	char* m_szLocation;
    char* m_szFriendlyName;
};

typedef struct CSsdpService CSsdpService;
	
extern struct CSsdpService* g_pFirstService;

#ifdef _THREADED
extern pthread_rwlock_t g_lockServicelist;
#endif
	
extern int SsdpInitialize(/*SsdpCallbackFunc* func*/);
extern int SsdpStartSearch(const char* szType, int mx/*=3*/);
extern int SsdpAnnounce(const char* szType, const char* szName, const char* szLocation);
extern int SsdpPump(/*struct CSsdpServer* pServer, */unsigned nMaxTimeout);
	
// These are optional event call back functions
extern void (*Ssdp_FoundNewService)(const char* szType, const char* szName, const char* szLocation, const char* szFriendlyName);
extern void (*Ssdp_ByeByeService)(const char* szType, const char* szName);
extern void (*Ssdp_ServiceChanged)(const char* szType, const char* szName, const char* szLocation);
	
#ifdef __cplusplus
};
#endif
