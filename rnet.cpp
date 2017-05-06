// This is code to talk to the Russound amplifiers

#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "rnet.h"

#define _DEBUG
#define TRACE printf
#define ASSERT(b)

#ifdef _DEBUG
void HexDump(const BYTE* pb, int cb)
{
     while (cb > 0)
     {
          TCHAR szLine [100];
          char* pch = szLine;
          int i;
          for (i = 0; i < 16 && i < cb; i += 1)
               pch += _stprintf(pch, "%02x ", pb[i]);
          for ( ; i < 16; i += 1)
               pch += _stprintf(pch, "   ");
          for (i = 0; i < 16 && i < cb; i += 1)
               *pch++ = pb[i] >= ' ' ? (char)pb[i] : '.';
          *pch = 0;
          TRACE("%s\n", szLine);
          cb -= i;
          pb += i;
     }
}
#endif

#ifdef _WIN32
extern HWND g_hMainWnd;
UINT WM_RNET_KEY = RegisterWindowMessage("WM_RNET_KEY");
#endif

/*

Per Zone:
     On/Off
     Source
     Volume
     Bass
     Treble
     Loudness
     Balance
     Volume
     "Turn On" Volume
     Do Not Disturb
     Party Mode

Per Controller:
     Background Color
     Message (alignment, flash rate)

*/

/*
struct RnetKey
{
     WORD wCode;
     const char* szLabel;
};

RnetKey g_rnetKeys [] =
{
     { 0x01, "1" },
     { 0x02, "2" },
     { 0x03, "3" },
     { 0x04, "4" },
     { 0x05, "5" },
     { 0x06, "6" },
     { 0x07, "7" },
     { 0x08, "8" },
     { 0x09, "9" },
     { 0x0A, "0" },
     { 0x0B, "Volume Up" },
     { 0x0C, "Volume Down" },
     { 0x0D, "Mute Zone" },
     { 0x0E, "Channel Up" },
     { 0x0F, "Channel Down" },
     { 0x10, "Power" },
     { 0x11, "Enter" },
     { 0x12, "Previous Channel" },
     { 0x13, "TV/Video" },
     { 0x14, "TV/VCR" },
     { 0x15, "A/B" },
     { 0x16, "TV/DVD" },
     { 0x17, "TV/LD" },
     { 0x18, "Input" },
     { 0x19, "TV/DSS" },
     { 0x1A, "Play" },
     { 0x1B, "Stop" },
     { 0x1C, "Search Forward" },
     { 0x1D, "Search Rewind" },
     { 0x1E, "Pause" },
     { 0x1F, "Record" },
     { 0x20, "Menu" },
     { 0x21, "Menu Up" },
     { 0x22, "Menu Down" },
     { 0x23, "Menu Left" },
     { 0x24, "Menu Right" },
     { 0x25, "Select" },
     { 0x26, "Exit" },
     { 0x27, "Display" },
     { 0x28, "Guide" },
     { 0x29, "Page Up" },
     { 0x2A, "Page Down" },
     { 0x2B, "Disk" },
     { 0x2C, "Plus 10" },
     { 0x2D, "Open/Close" },
     { 0x2E, "Random" },
     { 0x2F, "Track Forward" },
     { 0x30, "Track Reverse" },
     { 0x31, "Surround On/Off" },
     { 0x32, "Surround Mode" },
     { 0x33, "Surround Up" },
     { 0x34, "Surround Down" },
     { 0x35, "PIP" },
     { 0x36, "PIP Move" },
     { 0x37, "PIP Swap" },
     { 0x38, "Program" },
     { 0x39, "Sleep" },
     { 0x3A, "On" },
     { 0x3B, "Off" },
     { 0x3C, "11" },
     { 0x3D, "12" },
     { 0x3E, "13" },
     { 0x3F, "14" },
     { 0x40, "15" },
     { 0x41, "16" },
     { 0x42, "Bright" },
     { 0x43, "Dim" },
     { 0x44, "Close" },
     { 0x45, "Open" },
     { 0x46, "Stop 2" },
     { 0x47, "AM/FM" },
     { 0x48, "Cue" },
     { 0x49, "Disk Up" },
     { 0x4A, "Disk Down" },
     { 0x4B, "Info" },

     // These are for the UNO Pads
     { 0x64, "Setup Button" },
     { 0x67, "Previous" },
     { 0x68, "Next" },
     { 0x69, "Plus" },
     { 0x6A, "Minus" },
     { 0x6B, "Source" },
     { 0x6C, "Power" },
     { 0x6D, "Stop" },
     { 0x6E, "Pause" },
     { 0x6F, "Favorite 1" },
     { 0x70, "Favorite 2" },
     { 0x73, "Play" },
     { 0x7F, "Volume Up" },
     { 0x80, "Volume Down" },
};
*/

CRnetMonitor::CRnetMonitor()
{
     m_bInvertNext = false;
     m_bInPacket = false;
     m_ib = 0;
}

void CRnetMonitor::OnReceive(BYTE b)
{
     if (b == 0xf0)
     {
          m_ib = 0;
          m_bInvertNext = false;
          m_bInPacket = true;
     }

     if (m_bInPacket)
     {
          if (b == 0xf1)
          {
               m_bInvertNext = true;
               return;
          }

          if (m_bInvertNext)
          {
               m_bInvertNext = false;
               b = ~b;
          }

          ASSERT(m_ib < sizeof (m_packet)); // buffer too small?
          if (m_ib < sizeof (m_packet))
          {
               m_packet[m_ib] = b;
               m_ib += 1;
          }

          if (b == 0xf7)
          {
               OnReceivePacket(m_packet, m_ib);

               m_ib = 0;
               m_bInvertNext = false;
               m_bInPacket = false;
          }
     }
}

void CRnetMonitor::OnReceivePacket(const BYTE* packet, UINT nSize)
{
     TRACE("OnReceivePacket:\n");
     HexDump(packet, nSize);

     if (packet[1] == 0x00 && packet[2] == 0x00 && packet[3] == 0x70 &&
          packet[5] == 0x00 && packet[6] == 0x7F && packet[7] == 0x00 &&
          packet[8] == 0x00 && packet[9] == 0x04 && packet[10] == 0x02 &&
          packet[11] == 0x00 && packet[13] == 0x07 && packet[14] == 0x00 &&
          packet[15] == 0x00 && packet[16] == 0x01 && packet[17] == 0x00 &&
          packet[18] == 0x0C && packet[19] == 0x00)
     {
          // int nController = packet[4];
          int nZone = packet[12];
          ASSERT(nZone >= 0 && nZone < 6);

          m_zones[nZone].m_bPowerOn = packet[20] != 0;
          m_zones[nZone].m_nSource = packet[21];
          m_zones[nZone].m_nVolume = packet[22];
          m_zones[nZone].m_nBass = packet[23];
          m_zones[nZone].m_nTreble = packet[24];
          m_zones[nZone].m_bLoudness = packet[25] != 0;
          m_zones[nZone].m_nBalance = packet[26];
          m_zones[nZone].m_bSystemOn = packet[27] != 0;
          m_zones[nZone].m_bShared = packet[28] != 0;
          m_zones[nZone].m_nPartyMode = packet[29];
          m_zones[nZone].m_bDoNotDisturb = packet[30] != 0;

          TRACE("Zone State %d\n", nZone + 1);
          TRACE("\tPower On: %d\n", packet[20]);
          TRACE("\tSource: %d\n", packet[21] + 1);
          TRACE("\tVolume: %d\n", packet[22]);
          TRACE("\tBass: %d\n", packet[23]);
          TRACE("\tTreble: %d\n", packet[24]);
          TRACE("\tLoudness: %d\n", packet[25]);
          TRACE("\tBalance: %d\n", packet[26]);
          TRACE("\tSystem On: %d\n", packet[27]);
          TRACE("\tShared: %d\n", packet[28]);
          TRACE("\tParty Mode: %d\n", packet[29]);
          TRACE("\tDo Not Disturb: %d\n", packet[30]);
          return;
     }

     TRACE("Target Controller, Zone, Keypad ID: 0x%02x, 0x%02x, 0x%02x\n", packet[1], packet[2], packet[3]);
     TRACE("Source Controller, Zone, Keypad ID: 0x%02x, 0x%02x, 0x%02x\n", packet[4], packet[5], packet[6]);

     switch (packet[7])
     {
     default:
          TRACE("Message Type: %d\n", packet[7]);
          break;

     case 5: // Event
          {
               int nZone = packet[5];

               const BYTE* pb = &packet[8];
               {
                    int nPathSize = *pb++;
                    if (nPathSize > 0)
                    {
                         TRACE("Target Path: ");
                         while (nPathSize-- > 0)
                              TRACE("0x%02x ", *pb++);
                         TRACE("\n");
                    }
               }
               {
                    int nPathSize = *pb++;
                    if (nPathSize > 0)
                    {
                         TRACE("Source Path: ");
                         while (nPathSize-- > 0)
                              TRACE("0x%02x ", *pb++);
                         TRACE("\n");
                    }
               }

               WORD wEventID = (WORD)pb[0] + ((WORD)pb[1] << 8);
               pb += 2;

               WORD wEventTime = (WORD)pb[0] + ((WORD)pb[1] << 8);
               pb += 2;

               WORD wEventData = (WORD)pb[0] + ((WORD)pb[1] << 8);
               pb += 2;

               BYTE bEventPriority = *pb++;

               pb += 1; // skip checksum
               ASSERT(*pb == 0xf7); // end of packet!

               TRACE("Event: 0x%04x 0x%04x 0x%04x %d\n", wEventID, wEventTime, wEventData, bEventPriority);
/*
               if (wEventID == RNET_KEY_DISPLAY)
               {
                    // wEventData is the new source
                    m_zones[(wEventTime >> 8) - 1].m_nSource = wEventData;
                    TRACE("Set zone %d source to %d\n", (wEventTime >> 8) - 1, wEventData);
               }

               for (int i = 0; i < sizeof(g_rnetKeys) / sizeof(g_rnetKeys[0]); i += 1)
               {
                    if (g_rnetKeys[i].wCode == wEventID)
                    {
                         TRACE("%s\n", g_rnetKeys[i].szLabel);
                         break;
                    }
               }

               switch (wEventID)
               {
               case RNET_KEY_INFO:
                    DisplayMessage("Hello Love U");
                    break;

               case RNET_KEY_STOP:
                    break;

               case RNET_KEY_UNO_PREVIOUS:
                    TRACE("Previous zone %d\n", nZone);
                    ASSERT(nZone >= 0 && nZone < 6);
#ifdef _WIN32
                    ::PostMessage(g_hMainWnd, WM_RNET_KEY, wEventID, m_zones[nZone].m_nSource - 1);
#endif
                    break;

               case RNET_KEY_UNO_NEXT:
                    TRACE("Next zone %d\n", nZone);
                    ASSERT(nZone >= 0 && nZone < 6);
#ifdef _WIN32
                    ::PostMessage(g_hMainWnd, WM_RNET_KEY, wEventID, m_zones[nZone].m_nSource - 1);
#endif
                    break;
               }
*/
          }
          break;

     }

     TRACE("\n");
}

void CRnetMonitor::SendRnet(const BYTE* pbPacket, UINT cbPacket)
{
     BYTE data [1024];
     UINT cbData = 0;

     data[0] = 0xf0;
     cbData += 1;

	UINT i;
     for (i = 0; i < cbPacket; i += 1)
     {
          BYTE b = pbPacket[i];

          if ((b & 0x80) != 0)
          {
               data[cbData++] = 0xf1;
               b = ~b;
          }

          data[cbData++] = b;
     }

     BYTE bCheck = cbData;
     for (i = 0; i < cbData; i += 1)
          bCheck += data[i];
     bCheck &= 0x7f;

     data[cbData++] = bCheck;
     data[cbData++] = 0xf7;

     Write(data, cbData);

//     TRACE("Send %d bytes\n", cbData);
//     HexDump(data, cbData);
//     TRACE("\n");
     
#ifndef _WIN32
	if (tcdrain(m_fd) != 0)
	{
		fprintf(stderr, "tcdrain failed %d\n", errno);
	}
	Sleep(100);
#endif
}

void CRnetMonitor::DisplayMessage(const char* szMsg)
{
     BYTE packet [33] = { 0x7f, 0x00, 0x00, 0x00, 0x00, 0x70, 0x00, 0x02, 0x01, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x10, 0x00 };
     ZeroMemory(packet + 17, sizeof(packet) - 17);
     packet[17] = 1; //alignment
     packet[18] = 0; // flash (lsb)
     packet[19] = 0; // flash (msb)
     strncpy((char*)&packet[20], szMsg, 12);
     SendRnet(packet, 33);
}

void CRnetMonitor::SendRemoteKey(UINT nKey)
{
     BYTE packet [18] = { 0x00, 0x00, 0x7f, 0x00, 0x00, 0x70, 0x05, 0x02, 0x02, 0x00, 0x00, 0xBF, 0x00, 0x00, 0x00, 0x20, 0x00, 0x01 };
     packet[15] = nKey;
     SendRnet(packet, 18);
}

void CRnetMonitor::SendAllOnOff(bool bOn)
{
     BYTE packet [18] = { 0x7e, 0x00, 0x7F, 0x00, 0x00, 0x70, 0x05, 0x02, 0x02, 0x00, 0x00, 0xDD, 0x00, 0x00, 0x00/*param*/, 0x00, 0x00, 0x01 };
     packet[14] = bOn ? 0x01 : 0x00;
     SendRnet(packet, 18);
}

void CRnetMonitor::SendGetState(int nZone)
{
     BYTE packet [14] = { 0x00, 0x00, 0x7F, 0x00, 0x00, 0x70, 0x01, 0x04, 0x02, 0x00, 0x00/*zone*/, 0x07, 0x00, 0x00 };
//     packet[0] = nController;
     packet[10] = nZone;
     SendRnet(packet, 14);
}

void CRnetMonitor::ZoneOnOff(int nZone, bool bOn)
{
	printf("SetPower: %d %d\n", nZone, bOn);
     BYTE packet [18] = { 0x00, 0x00, 0x7f, 0x00, 0x00, 0x70, 0x05, 0x02, 0x02, 0x00, 0x00, 0xDC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 };
//     packet[0] = nController;
     packet[13] = bOn ? 0x01 : 0x00;
     packet[15] = nZone;
     SendRnet(packet, 18);
}

void CRnetMonitor::SetSource(int nZone, int nSource)
{
	printf("SetSource: %d %d\n", nZone, nSource);
     BYTE packet [18] = { 0x00, 0x00, 0x7f, 0x00, 0x00, 0x70, 0x05, 0x02, 0x00, 0x00, 0x00, 0xC1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 };
//     packet[0] = nController;
     packet[4] = nZone;
     packet[15] = nSource;
     SendRnet(packet, 18);
}

void CRnetMonitor::SetVolume(int nZone, int nVolume)
{
	printf("SetVolume: %d %d\n", nZone, nVolume);
     BYTE packet [18] = { 0x00, 0x00, 0x7f, 0x00, 0x00, 0x70, 0x05, 0x02, 0x02, 0x00, 0x00, 0xDE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 };
//     packet[0] = nController;
     packet[13] = nVolume;
     packet[15] = nZone;
     SendRnet(packet, 18);
}

void CRnetMonitor::SetLoudness(int nZone, bool bOn)
{
     BYTE packet [21] = { 0x00, 0x00, 0x7f, 0x00, 0x00, 0x70, 0x00, 0x05, 0x02, 0x00, 0x00/*zone*/, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00/*bOn*/ };
//     packet[0] = nController;
     packet[10] = nZone;
     packet[20] = bOn ? 0x01 : 0x00;
     SendRnet(packet, 21);
}

void CRnetMonitor::SetBass(int nZone, int nBass)
{
     BYTE packet [21] = { 0x00, 0x00, 0x7f, 0x00, 0x00, 0x70, 0x00, 0x05, 0x02, 0x00, 0x00/*zone*/, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00/*bass*/ };
//     packet[0] = nController;
     packet[10] = nZone;
     packet[20] = nBass;
     SendRnet(packet, 21);
}

void CRnetMonitor::SetTreble(int nZone, int nTreble)
{
     BYTE packet [21] = { 0x00, 0x00, 0x7f, 0x00, 0x00, 0x70, 0x00, 0x05, 0x02, 0x00, 0x00/*zone*/, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00/*treble*/ };
//     packet[0] = nController;
     packet[10] = nZone;
     packet[20] = nTreble;
     SendRnet(packet, 21);
}

void CRnetMonitor::SetBalance(int nZone, int nBalance)
{
     BYTE packet [21] = { 0x00, 0x00, 0x7f, 0x00, 0x00, 0x70, 0x00, 0x05, 0x02, 0x00, 0x00/*zone*/, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00/*balance*/ };
//     packet[0] = nController;
     packet[10] = nZone;
     packet[20] = nBalance;
     SendRnet(packet, 21);
}

void CRnetMonitor::SetBacklight(int nZone, int nBacklight)
{
     TRACE("SetBacklight: %d\n", nBacklight);

     BYTE packet [21] = { 0x00, 0x00, 0x7f, 0x00, 0x00, 0x70, 0x00, 0x05, 0x02, 0x00, 0x00/*zone*/, 0x00, 0x05, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00/*color*/ };
//     packet[0] = nController;
     packet[10] = nZone;
     packet[20] = nBacklight;
     SendRnet(packet, 21);
}

CRnetMonitor g_russound;

bool CRnetMonitor::Start(const char* szPort)
{
	m_fd = open(szPort, O_RDWR | O_NOCTTY);
	if (m_fd < 0)
	{
		printf("Can't open %s! (%d)\n", szPort, errno);
		fprintf(stderr, "Can't open %s!\n", szPort);
		return false;
	}
	
	struct termios tio;
	bzero(&tio, sizeof(tio));
	tio.c_cflag = B19200 | CS8 | CLOCAL | CREAD;
	tio.c_iflag = IGNPAR;
	tio.c_oflag = 0;
	tio.c_lflag = 0;
	tio.c_cc[VTIME] = 0;
	tio.c_cc[VMIN] = 5; // blocking read until 5 chars received
	tcflush(m_fd, TCIOFLUSH);
	if (tcsetattr(m_fd, TCSANOW, &tio) != 0)
	{
		fprintf(stderr, "Can't configure port %s (err=%d)\n", szPort, errno);
		return false;
	}
	
	return true;
}

void Init_Russound()
{
#ifdef _WIN32
     g_russound.Start("COM1", CBR_19200);
#else
#endif
     for (int nZone = 0; nZone < 6; nZone += 1)
     {
          g_russound.SendGetState(nZone);
          Sleep(10);
     }
}

void Rnet_DisplayMessage(const char* szMessage)
{
     g_russound.DisplayMessage(szMessage);
}

void Rnet_SendAllOnOff(bool bOn)
{
     g_russound.SendAllOnOff(bOn);
}

void Rnet_ZoneOnOff(int nZone, bool bOn)
{
     g_russound.ZoneOnOff(nZone, bOn);
}

void Rnet_SetSource(int nZone, int nSource)
{
     g_russound.SetSource(nZone, nSource);
}

void Rnet_SetVolume(int nZone, int nVolume)
{
     g_russound.SetVolume(nZone, nVolume);
}

void Rnet_SetLoudness(int nZone, bool bOn)
{
     g_russound.SetLoudness(nZone, bOn);
} 

void Rnet_SaveState(int nZone, const char* szFile)
{
	BYTE packet [14] = { 0x00, 0x00, 0x7f, 0x00, 0x00, 0x70, 0x01, 0x04, 0x02, 0x00, 255, 0x07, 0x00, 0x00 };
	packet[10] = nZone;
	g_russound.SendRnet(packet, 14);
	
	FILE* pFile = fopen(szFile, "w");

	BYTE buf [256];
	int ib = 0;	
	for (;;)
	{
		int cb = read(g_russound.m_fd, buf + ib, 34 - ib);
		if (cb == 0)
			break;

printf("Dump %d\n", cb);
		HexDump(buf + ib, cb);

		ib += cb;
		if (ib >= 34 && buf[0] == 0xf0 && buf[1] == 0x00 && buf[2] == 0x00 && buf[3] == 0x70)
		{
			int nController = buf[4];
			int nZone = buf[12];
				
			fprintf(pFile, "zone%d\n", nZone + 1);
			if (buf[20])
				fprintf(pFile, "\tpower: %s\n", buf[20] ? "on" : "off");
			fprintf(pFile, "\tsource: %d\n", buf[21] + 1);
			fprintf(pFile, "\tvolume: %d\n", buf[22] * 2);
			fprintf(pFile, "\tbass: %d\n", int(buf[23]) - 10);
			fprintf(pFile, "\ttreble: %d\n", int(buf[24]) - 10);
			fprintf(pFile, "\tloudness: %s\n", buf[25] ? "on" : "off");
			fprintf(pFile, "\tbalance: %d\n", int(buf[26]) - 10);
			if (!buf[20])
				fprintf(pFile, "\tpower: %s\n", buf[20] ? "on" : "off");
			break;
		}

		if (ib >= 34)
		{
			printf("Unexpected packet!\n");
			break;
		}
	}

	fclose(pFile);
}

void Rnet_LoadState(const char* szFile)
{
	FILE* pFile = fopen(szFile, "r");
	if (pFile == NULL)
		return;
	
	int nZone = -1;
	char szLine [256];
	while (fgets(szLine, sizeof(szLine), pFile) != NULL)
	{
		if (strncmp(szLine, "zone", 4) == 0)
		{
			nZone = atoi(szLine + 4);
			if (nZone < 1 || nZone > 6)
			{
				fprintf(stderr, "Invalid zone: %s\n", szLine);
				break;
			}
			
			printf("Zone %d\n", nZone);
			
			nZone -= 1;
		}
		else if (nZone >= 0)
		{
			char* pch = szLine;
			while (*pch == ' ' || *pch == '\t')
				pch += 1;
			char* szToken = pch;
			while (*pch != '\0' && *pch != ':')
				pch += 1;
			if (*pch == ':')
			{
				*pch = '\0';
				pch += 1;
				while (*pch == ' ' || *pch == '\t')
					pch += 1;
					
				if (strcmp(szToken, "power") == 0)
				{
					Rnet_ZoneOnOff(nZone, strncmp(pch, "on", 2) == 0);
				}
				else if (strcmp(szToken, "source") == 0)
				{
					int nSource = atoi(pch);
					if (nSource < 1 || nSource > 6)
					{
						fprintf(stderr, "Invalid source: %s\n", szLine);
						break;
					}
					
					nSource -= 1;
					Rnet_SetSource(nZone, nSource);
				}
				else if (strcmp(szToken, "volume") == 0)
				{
					int nVolume = atoi(pch);
					if (nVolume < 0 || nVolume > 100)
					{
						fprintf(stderr, "Invalid volume: %s\n", szLine);
						break;
					}
					nVolume /= 2;
					Rnet_SetVolume(nZone, nVolume);
				}
			}
			else
			{
				fprintf(stderr, "syntax error: %s\n", szLine);
				break;
			}
		}
	}
	
	fclose(pFile);
}

void RunCmd(char* szLine)
{
    printf("RunCmd: %s\n", szLine);

    int nZone, nSource;

    if (sscanf(szLine, "on %d\n", &nZone) == 1 && nZone >= 1 && nZone <= 6)
	    Rnet_ZoneOnOff(nZone - 1, true);
    else if (sscanf(szLine, "off %d\n", &nZone) == 1 && nZone >= 1 && nZone <= 6)
	    Rnet_ZoneOnOff(nZone - 1, false);
    else if (sscanf(szLine, "source %d %d\n", &nZone, &nSource) == 2 && nZone >= 1 && nZone <= 6 && nSource >= 1 && nSource <= 6)
	    Rnet_SetSource(nZone - 1, nSource - 1);
    else
        printf("unknown command\n");
}

int main(int argc, char* argv [])
{
	if (argc < 2)
	{
	     printf("Usage: rnet <port> <cmd> [<zone>] [<params>]\n");
	     printf("\tall on\n");
	     printf("\tall off\n");
	     printf("\ton <zone>\n");
	     printf("\toff <zone>\n");
	     printf("\tsource <zone> <source>\n");
	     printf("\tvolume <zone> <0..100>\n");
	     printf("\tloudness <zone> on|off\n");
	     printf("\tmessage <message>\n");
	     exit(0);
	}

	const char* szPort = argv[1];
	if (!g_russound.Start(szPort))
		return 0;
	
    if (argc == 4 && strcmp(argv[2], "-input") == 0)
    {
        for (;;)
        {
            FILE* pInputFile = fopen(argv[3], "r");
            if (pInputFile == NULL)
            {
                fprintf(stderr, "Cannot open input file: %s\n", argv[3]);
                exit(0);
            }

            char szLine [256];
            while (fgets(szLine, sizeof(szLine), pInputFile) != NULL)
            {
                RunCmd(szLine);
            }
        }
        exit(0);
    }
	if (argc > 2)
	{
		const char* szCmd = argv[2];
		int nZone = atoi(argv[3]) - 1;
	
		if (strcmp(szCmd, "all") != 0 && strcmp(szCmd, "message") != 0 && strcmp(szCmd, "loadstate") != 0 && (nZone < 0 || nZone > 5))
		{
			printf("Invalid zone: %s\n", argv[3]);
			return 0;
		}
		
		if (strcmp(szCmd, "all") == 0)
		{
			bool bAllOn = strcmp(argv[3], "on") == 0;
			Rnet_SendAllOnOff(bAllOn);
		}
		else if (strcmp(szCmd, "on") == 0)
		{
			Rnet_ZoneOnOff(nZone, true);
		}	 
		else if (strcmp(szCmd, "off") == 0)
		{
			Rnet_ZoneOnOff(nZone, false);
		}
		else if (strcmp(szCmd, "source") == 0)
		{
			int nSource = atoi(argv[4]) - 1;
			Rnet_SetSource(nZone, nSource);
		}
		else if (strcmp(szCmd, "volume") == 0)
		{
			int nVolume = atoi(argv[4]) / 2;
			Rnet_SetVolume(nZone, nVolume);
		}
		else if (strcmp(szCmd, "loudness") == 0)
		{
			bool bLoudness = strcmp(argv[4], "on") == 0;
			Rnet_SetVolume(nZone, bLoudness);
		}
		else if (strcmp(szCmd, "message") == 0)
		{
			Rnet_DisplayMessage(argv[3]);
		}
		else if (strcmp(szCmd, "savestate") == 0)
		{
			const char* szFile = argv[4];
			Rnet_SaveState(nZone, szFile);
		}
		else if (strcmp(szCmd, "loadstate") == 0)
		{
			const char* szFile = argv[3];
			Rnet_LoadState(szFile);
		}
		else
		{
			fprintf(stderr, "Unknown command: %s\n", szCmd);
		}
	}
	else
	{
		printf("rnet messages:\n");
		for (;;)
		{
			BYTE buf [256];
			int cb = read(g_russound.m_fd, buf, sizeof(buf));
			if (cb == 0)
				break;
			HexDump(buf, cb);
		}
		return 0;
	}
	
    return 0;
}
