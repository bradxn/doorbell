#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>
#include <errno.h>

#if 0
var b = require('bonescript');
var spawn = require('child_process').spawn;

/* not working yet
var serialport = require("serialport");

serialport.list(function (err, ports) {
  ports.forEach(function(port) {
    console.log(port.comName);
    console.log(port.pnpId);
    console.log(port.manufacturer);
  });
});
*/

var pirPin = "P9_12";
var buttonPin = "P9_14";

var ledPin1 = "USR0";
var ledPin2 = "USR1";

b.pinMode(pirPin, b.INPUT);
b.pinMode(buttonPin, b.INPUT);

b.pinMode(ledPin1, b.OUTPUT);
b.pinMode(ledPin2, b.OUTPUT);

b.attachInterrupt(pirPin, true, b.RISING, OnPirRise);
b.attachInterrupt(buttonPin, true, b.RISING, OnButtonRise);

function OnPirRise(x)
{
    console.log("Someone's here!");
}

function OnButtonRise(x)
{
    console.log("Ding Dong!");
    spawn("mplayer", ["/home/root/DingDong.wav", "-ao", "alsa:device=hw=1.0"]);
}
#endif

int g_nButtonPin = 50; // P9_14

bool gpio_export(int nPin, bool bExport)
{
	char szPath [256];
	sprintf(szPath, "/sys/class/gpio/%sexport", bExport ? "" : "un");
	
	int fd = open(szPath, O_WRONLY);
	if (fd < 0)
		return false;
	
	char szBuf [32];
	int cch = sprintf(szBuf, "%d", nPin);	
	write(fd, szBuf, cch + 1);
	close(fd);
	
	return true;
}

bool gpio_set(int nPin, const char* szValueName, const char* szValue)
{
	char szPath [256];
	sprintf(szPath, "/sys/class/gpio/gpio%d/%s", nPin, szValueName);
	
	int fd = open(szPath, O_WRONLY);
	if (fd < 0)
		return false;
		
	int nWrite = write(fd, szValue, strlen(szValue) + 1);
	
	close(fd);
	
	return nWrite > 0;
}

int gpio_open(int nPin)
{
	char szPath [256];
	sprintf(szPath, "/sys/class/gpio/gpio%d/value", nPin);
	
	return open(szPath, O_RDONLY /*| O_NONBLOCK*/);
}

void gpio_close(int fd)
{
	close(fd);
}

void DingDong()
{
	printf("Ding Dong!\n");
	
	system("rnet /dev/ttyUSB0 savestate 1 rnet1.txt");
	system("rnet /dev/ttyUSB0 savestate 2 rnet2.txt");
	system("rnet /dev/ttyUSB0 savestate 3 rnet3.txt");
	system("rnet /dev/ttyUSB0 savestate 4 rnet4.txt");
	system("rnet /dev/ttyUSB0 savestate 5 rnet5.txt");
	system("rnet /dev/ttyUSB0 savestate 6 rnet6.txt");
	
    system("rnet /dev/ttyUSB0 loadstate doorbell.rnet");
    
//    system("rnet /dev/ttyUSB0 on 1");
//    system("rnet /dev/ttyUSB0 source 1 3");
//    system("rnet /dev/ttyUSB0 volume 1 50");
    
    system("mplayer /home/root/projects/rnet/Doorbell.m4a -ao alsa:device=hw=1.0");
    
    system("rnet /dev/ttyUSB0 loadstate rnet1.txt");
    system("rnet /dev/ttyUSB0 loadstate rnet2.txt");
    system("rnet /dev/ttyUSB0 loadstate rnet3.txt");
    system("rnet /dev/ttyUSB0 loadstate rnet4.txt");
    system("rnet /dev/ttyUSB0 loadstate rnet5.txt");
    system("rnet /dev/ttyUSB0 loadstate rnet6.txt");
}

bool gpio_poll(int fd, bool bTimeout/*char* szValue, int nValueSize*/)
{
	pollfd pfd;
	pfd.fd = fd;
	pfd.events = POLLPRI;
	pfd.revents = 0;
	int p = poll(&pfd, 1, bTimeout ? 5 * 1000 : -1/*10 * 1000*/);
	if (p < 0)
		printf("poll error %d\n", errno);
	else if (p == 0)
	{
		printf("poll timeout\n");
		return false;
	}
//	else if (p > 0)
//		printf("poll %d\n", p);
	
	int nRead = 0;
	if (p > 0 && (pfd.revents & POLLPRI) != 0)
	{
//		printf("reading...\n");
		char szValue [32];
		int nValueSize = sizeof(szValue);
		nRead = read(fd, szValue, nValueSize);
		if (nRead < 0)
			return false;
//		printf("%d bytes\n", nRead);
		
		if (!bTimeout)
			DingDong();
	}
	
//	szValue[nRead] = '\0';
	
	return true;
}

bool gpio_get(int nPin, const char* szValueName, char* szValue, int nValueSize)
{
	char szPath [256];
	sprintf(szPath, "/sys/class/gpio/gpio%d/%s", nPin, szValueName);
	
	int fd = open(szPath, O_RDONLY | O_NONBLOCK);
	if (fd < 0)
		return false;
	
	pollfd pfd;
	pfd.fd = fd;
	pfd.events = POLLPRI;
	pfd.revents = 0;
	int p = poll(&pfd, 1, -1);
	if (p < 0)
		printf("poll error %d\n", errno);
	else if (p == 0)
		printf("poll timeout\n");
//	else if (p > 0)
//		printf("poll %d\n", p);
	
	int nRead = 0;
	if (p > 0 && (pfd.revents & POLLPRI) != 0)
		nRead = read(fd, szValue, nValueSize);
			
	close(fd);
	
	if (nRead >= 0)
		szValue[nRead] = '\0';
		
	return nRead > 0;
}

bool gpio_setdir(int nPin, const char* szDir)
{
	return gpio_set(nPin, "direction", szDir);
}

int main(int argc, const char* argv [])
{
	gpio_export(g_nButtonPin, true);
	gpio_setdir(g_nButtonPin, "in");
	gpio_set(g_nButtonPin, "edge", "rising");
	
	int fd = gpio_open(g_nButtonPin);
	
	{
		printf("Getting initial value...\n");
		char szValue [32];
		int nRead = read(fd, szValue, sizeof(szValue));
		printf("...%s\n", szValue);
	}
		
	for (;;)
	{
		printf("Waiting for visitor...\n");
		
		char szBuf [32];
		if (!gpio_poll(fd, false/*szBuf, sizeof(szBuf)*/))
//		if (!gpio_get(g_nButtonPin, "value", szBuf, sizeof(szBuf)))
		{
			fprintf(stderr, "Error in gpio_get\n");
			exit(0);
		}
		
		printf("Waiting for idle...\n");
		while (gpio_poll(fd, true))
			;
		
//		printf("GOT: %s\n", szBuf);
		
//		usleep(1000 * 1000 * 5);
	}
	
	return 0;
}
