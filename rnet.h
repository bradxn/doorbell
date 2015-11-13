#pragma once

// #include "PortMonitor.h"
// #include "rnetkeys.h"

#ifndef _WIN32
#include <unistd.h>
#include <string.h>
#endif

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int UINT;
typedef char TCHAR;

#define _stprintf sprintf
#define ZeroMemory(pv, cb) memset(pv, 0, cb)
#define Sleep(ms) usleep(ms * 1000)

class CRnetZone
{
public:
     bool m_bPowerOn;
     int m_nSource;
     int m_nVolume;
     int m_nBass;
     int m_nTreble;
     bool m_bLoudness;
     int m_nBalance;
     bool m_bSystemOn;
     bool m_bShared;
     int m_nPartyMode;
     bool m_bDoNotDisturb;
};

class CRnetMonitor // : public CPortMonitor
{
public:
     CRnetMonitor();
     
     bool Start(const char* szPort);

     void OnReceive(BYTE b);
     void OnReceivePacket(const BYTE* packet, UINT nSize);

     void SendRnet(const BYTE* pbPacket, UINT cbPacket);

     void SendGetState(int nZone);

     void DisplayMessage(const char* szMsg);
     void SendRemoteKey(UINT nKey);
     void SendAllOnOff(bool bOn);
    
     void ZoneOnOff(int nZone, bool bZone);
     void SetSource(int nZone, int nSource);
     void SetVolume(int nZone, int nVolume);
     void SetLoudness(int nZone, bool bOn);
     void SetBass(int nZone, int nBass);
     void SetTreble(int nZone, int nTreble);
     void SetBalance(int nZone, int nBalance);
     void SetBacklight(int nZone, int nBacklight);

     BYTE m_packet [256];
     UINT m_ib;
     bool m_bInvertNext;
     bool m_bInPacket;

     CRnetZone m_zones [6];
     
#ifndef _WIN32
	void Write(const void* pv, int cb)
	{
		if (write(m_fd, pv, cb) != cb)
		{
			fprintf(stderr, "Write failed %d\n", errno);
		}
	}
	
	int m_fd;
#endif
};

extern CRnetMonitor g_russound;
