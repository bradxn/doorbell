// This code monitors the Apex secirity system events

#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>
#include <time.h>

struct apex_word_desc
{
	const char* m_szWord;
	int m_nCode;
};

apex_word_desc g_words [] = 
{
	{ "A", 233 }, 
	{ "Accept", 251 }, 
	{ "Access", 24 }, 
	{ "Account", 25 }, 
	{ "Active", 278 }, 
	{ "Air", 205 }, 
	{ "Alarm", 384 }, 
	{ "Alert", 26 }, 
	{ "AM", 33 }, 
	{ "And", 197 }, 
	{ "Apartment", 402 }, 
	{ "Apex", 332 }, 
	{ "Appliance", 27 }, 
	{ "April", 430 }, 
	{ "Are", 28 }, 
	{ "Area", 29 }, 
	{ "Armed", 30 }, 
	{ "Art", 362 }, 
	{ "Asleep", 281 }, 
	{ "At", 306 }, 
	{ "Atrium", 374 }, 
	{ "Attic", 31 }, 
	{ "Audio", 279 }, 
	{ "August", 434 }, 
	{ "Auto", 235 }, 
	{ "Automatic", 283 }, 
	{ "Automation", 344 }, 
	{ "Awake", 280 }, 
	{ "Away", 32 }, 
	{ "B", 34 }, 
	{ "Baby", 335 }, 
	{ "Back", 35 }, 
	{ "Bar", 176 }, 
	{ "Basement", 36 }, 
	{ "Bathroom", 37 }, 
	{ "Battery", 38 }, 
	{ "Bay", 39 }, 
	{ "Bedroom", 40 }, 
	{ "Bell", 366 }, 
	{ "Boiler", 403 }, 
	{ "Bottom", 41 }, 
	{ "Boys", 42 }, 
	{ "Break", 43 }, 
	{ "Breakfast", 418 }, 
	{ "Building", 404 }, 
	{ "Burglary", 392 }, 
	{ "Butler", 373 }, 
	{ "Button", 206 }, 
	{ "Bypassed", 44 }, 
	{ "C", 45 }, 
	{ "Cabana", 295 }, 
	{ "Cabinet", 177 }, 
	{ "Call", 388 }, 
	{ "Camera", 207 }, 
	{ "Carbon-monoxide", 256 },
	{ "Ceiling", 363 }, 
	{ "Center", 208 }, 
	{ "Central", 399 }, 
	{ "Change", 250 }, 
	{ "Check", 385 }, 
	{ "Choices", 317 }, 
	{ "Christmas", 320 }, 
	{ "Circuit", 414 }, 
	{ "Closed", 46 }, 
	{ "Closet", 47 }, 
	{ "Code", 48 }, 
	{ "Coffee", 302 }, 
	{ "Communicator", 175 }, 
	{ "Conservatory", 334 }, 
	{ "Console", 398 }, 
	{ "Control", 49 }, 
	{ "Cool", 234 }, 
	{ "Cooling", 247 }, 
	{ "Corner", 50 }, 
	{ "Court", 376 }, 
	{ "Crawlspace", 51 }, 
	{ "Current", 270 }, 
	{ "Curtain", 209 }, 
	{ "D", 52 }, 
	{ "Danger", 401 }, 
	{ "Date", 299 }, 
	{ "Debounce", 340 }, 
	{ "Deck", 53 }, 
	{ "Decrease", 346 }, 
	{ "December", 438 }, 
	{ "Default", 380 }, 
	{ "Defaults", 381 }, 
	{ "Defined", 284 }, 
	{ "Degrees", 54 }, 
	{ "Delay", 341 }, 
	{ "Den", 55 }, 
	{ "Destiny", 333 }, 
	{ "Detected", 56 }, 
	{ "Detector", 57 }, 
	{ "Device", 397 }, 
	{ "Dialing", 390 }, 
	{ "Digit", 245 }, 
	{ "Dining", 58 }, 
	{ "Disable", 240 }, 
	{ "Disarmed", 59 }, 
	{ "Dock", 217 }, 
	{ "Door", 60 }, 
	{ "Doors", 218 }, 
	{ "Down", 61 }, 
	{ "Driveway", 198 }, 
	{ "E", 62 }, 
	{ "East", 63 }, 
	{ "Eight", 8 }, 
	{ "Eighty", 22 }, 
	{ "Eleven", 11 },
	{ "Emergency", 327 }, 
	{ "Enable", 239 }, 
	{ "End", 353 }, 
	{ "Energy", 287 }, 
	{ "Enter", 64 }, 
	{ "Entering", 359 }, 
	{ "Entertainment", 286 }, 
	{ "Entry", 65 }, 
	{ "Equipment", 405 }, 
	{ "Error", 252 }, 
	{ "Exercise", 300 }, 
	{ "Expander", 423 }, 
	{ "Exit", 66 }, 
	{ "F", 67 }, 
	{ "Factory", 406 }, 
	{ "Fail", 68 }, 
	{ "Family", 69 }, 
	{ "Fan", 178 }, 
	{ "February", 428 }, 
	{ "Fence", 352 }, 
	{ "Fifteen", 14 }, 
	{ "Fifty", 19 }, 
	{ "Fire", 70 }, 
	{ "First", 71 }, 
	{ "Five", 5 }, 
	{ "Flood", 72 }, 
	{ "Floor", 179 }, 
	{ "Florida", 73 }, 
	{ "Force", 74 }, 
	{ "Forty", 18 }, 
	{ "Fountain", 309 }, 
	{ "Four", 4 }, 
	{ "Foyer", 75 }, 
	{ "Freeze", 290 }, 
	{ "French", 76 }, 
	{ "Friday", 276 }, 
	{ "Front", 77 }, 
	{ "Full", 322 }, 
	{ "Function", 338 }, 
	{ "Furnace", 258 }, 
	{ "Fuse", 78 }, 
	{ "Gallery", 345 }, 
	{ "Game", 310 }, 
	{ "Garage", 79 }, 
	{ "Garden", 377 }, 
	{ "Gas", 180 }, 
	{ "Gate", 199 }, 
	{ "Girls", 80 }, 
	{ "Glass", 81 }, 
	{ "Good-bye", 355 }, 
	{ "Great", 82 }, 
	{ "Green", 369 }, 
	{ "Group", 285 }, 
	{ "Guest", 83 }, 
	{ "Gun", 181 },
	{ "Gym", 294 }, 
	{ "H", 232 }, 
	{ "Hall", 84 }, 
	{ "Head", 85 }, 
	{ "Heat", 86 }, 
	{ "Heater", 311 }, 
	{ "Heating", 246 }, 
	{ "Hello", 356 }, 
	{ "High", 219 }, 
	{ "Hold", 254 }, 
	{ "Home", 87 }, 
	{ "Hottub", 182 }, 
	{ "Hour", 244 }, 
	{ "House", 183 }, 
	{ "HVAC", 439 }, 
	{ "In", 88 }, 
	{ "Increase", 387 }, 
	{ "Input", 339 }, 
	{ "Inside", 361 }, 
	{ "Instant", 393 }, 
	{ "Interior", 257 }, 
	{ "Intruder", 89 },  
	{ "Intrusion-detected", 90 }, 
	{ "Is", 91 }, 
	{ "Jacuzzi", 370 }, 
	{ "January", 427 }, 
	{ "Jewelry", 184 }, 
	{ "June", 432 }, 
	{ "July", 433 }, 
	{ "Key", 253 }, 
	{ "Keypad", 92 }, 
	{ "Kitchen", 93 }, 
	{ "Lamp", 94 }, 
	{ "Lanai", 304 }, 
	{ "Laundry", 95 }, 
	{ "Lawn", 296 }, 
	{ "Leak", 308 }, 
	{ "Left", 97 }, 
	{ "Leave-immediately", 96 },
	{ "Level", 185 }, 
	{ "Library", 98 }, 
	{ "Light", 99 }, 
	{ "Lightning", 347 }, 
	{ "Lights", 100 }, 
	{ "Line", 210 }, 
	{ "Living", 101 }, 
	{ "Loading", 407 }, 
	{ "Lobby", 211 }, 
	{ "Location", 200 }, 
	{ "Lock", 102 }, 
	{ "Loft", 212 }, 
	{ "Loop", 342 }, 
	{ "Low", 103 }, 
	{ "Lower", 213 }, 
	{ "Machine", 408 },
	{ "Maids", 214 }, 
	{ "Main", 104 },
	{ "Management", 288 }, 
	{ "Manual", 236 }, 
	{ "March", 429 }, 
	{ "Master", 105 }, 
	{ "Mat", 215 }, 
	{ "Max", 394 }, 
	{ "May", 431 }, 
	{ "Mechanical", 291 }, 
	{ "Medicine", 186 }, 
	{ "Mens", 216 }, 
	{ "Menu", 231 }, 
	{ "Middle", 106 }, 
	{ "Minute", 396 }, 
	{ "Mode", 107 }, 
	{ "Modes", 243 }, 
	{ "Module", 108 }, 
	{ "Monday", 272 }, 
	{ "Monitor", 187 }, 
	{ "Month", 424 }, 
	{ "Motion", 109 }, 
	{ "Motor", 323 }, 
	{ "Mud", 110 }, 
	{ "Natural", 305 }, 
	{ "Next", 329 }, 
	{ "Night", 111 }, 
	{ "Nine", 9 }, 
	{ "Ninety", 23 }, 
	{ "No", 415 }, 
	{ "Nook", 112 }, 
	{ "North", 113 }, 
	{ "November", 437 }, 
	{ "Now", 386 }, 
	{ "Number", 114 }, 
	{ "Nursery", 115 }, 
	{ "O", 116 }, 
	{ "October", 436 }, 
	{ "Off", 117 }, 
	{ "Office", 118 }, 
	{ "On", 119 }, 
	{ "One", 1 }, 
	{ "Only", 326 }, 
	{ "Open", 120 }, 
	{ "Operating", 268 }, 
	{ "Option", 201 }, 
	{ "Options", 337 }, 
	{ "Or", 202 }, 
	{ "Other", 242 }, 
	{ "Out", 121 }, 
	{ "Outlet", 122 }, 
	{ "Over", 123 }, 
	{ "Overhead", 259 }, 
	{ "Overload", 413 }, 
	{ "Panel", 331 },
	{ "Panic", 220 }, 
	{ "Pantry", 364 }, 
	{ "Parlor", 188 }, 
	{ "Partition", 400 }, 
	{ "Patio", 203 }, 
	{ "Pause", 124 }, 
	{ "Pendant", 221 }, 
	{ "Perimeter", 260 }, 
	{ "Personal", 354 }, 
	{ "Pet", 189 }, 
	{ "Phone", 125 }, 
	{ "Place", 307 }, 
	{ "Play", 126 }, 
	{ "PM", 134 }, 
	{ "Pool", 127 }, 
	{ "Porch", 128 }, 
	{ "Pot", 303 }, 
	{ "Pound", 129 }, 
	{ "Power", 130 }, 
	{ "Pressure", 261 }, 
	{ "Pro", 131 }, 
	{ "Problem", 132 }, 
	{ "Program", 133 }, 
	{ "Protected", 358 }, 
	{ "Pump", 222 }, 
	{ "Radio", 422 }, 
	{ "Raise", 360 }, 
	{ "Reading", 348 }, 
	{ "Ready", 395 }, 
	{ "Rear", 190 }, 
	{ "Rec", 135 }, 
	{ "Receiver", 417 }, 
	{ "Red", 367 }, 
	{ "Remain-calm", 136 }, 
	{ "Remote", 137 }, 
	{ "Repeat", 230 }, 
	{ "Report", 138 }, 
	{ "Reprogram", 328 }, 
	{ "Restored", 139 }, 
	{ "Restricted-area", 379 }, 
	{ "Right", 140 }, 
	{ "Roof", 223 }, 
	{ "Room", 141 }, 
	{ "Run", 301 }, 
	{ "Safe", 191 }, 
	{ "Saturday", 277 }, 
	{ "Save", 330 }, 
	{ "Screen", 142 }, 
	{ "Second", 143 }, 
	{ "Secure", 312 }, 
	{ "Security", 314 }, 
	{ "Sensor", 144 }, 
	{ "September", 434 }, 
	{ "Serial", 343 }, 
	{ "Servants", 372 },
	{ "Service", 224 }, 
	{ "Set", 241 }, 
	{ "Setback", 237 }, 
	{ "Setbacks", 238 }, 
	{ "Setpoint", 324 }, 
	{ "Setpoints", 325 }, 
	{ "Setting", 383 }, 
	{ "Seven", 7 }, 
	{ "Seventy", 21 }, 
	{ "Shed", 409 }, 
	{ "Shipping", 225 }, 
	{ "Shock", 145 }, 
	{ "Shop", 146 }, 
	{ "Showing", 147 }, 
	{ "Side", 148 }, 
	{ "Simultaneously", 412 }, 
	{ "Siren", 365 }, 
	{ "Six", 6 }, 
	{ "Sixty", 20 }, 
	{ "Sky", 149 }, 
	{ "Slider", 192 }, 
	{ "Sliding", 150 }, 
	{ "Smart", 313 }, 
	{ "Smoke", 151 }, 
	{ "Soffit", 319 }, 
	{ "South", 152 }, 
	{ "Spa", 193 }, 
	{ "Spare", 153 }, 
	{ "Spot", 419 }, 
	{ "Sprinkler", 297 }, 
	{ "Stairs", 154 }, 
	{ "Stairwell", 350 }, 
	{ "Star", 155 }, 
	{ "Station", 410 }, 
	{ "Stay", 204 }, 
	{ "Stereo", 321 }, 
	{ "Stock", 229 }, 
	{ "Storage", 156 }, 
	{ "Stress", 194 }, 
	{ "Studio", 226 }, 
	{ "Study", 157 }, 
	{ "Suite", 351 }, 
	{ "Sump", 227 }, 
	{ "Sun", 158 }, 
	{ "Sunday", 271 }, 
	{ "Switch", 248 }, 
	{ "System", 159 }, 
	{ "Table", 421 }, 
	{ "Tamper", 195 }, 
	{ "Teen", 15 }, 
	{ "Television", 292 }, 
	{ "Temporary", 382 }, 
	{ "Temperature", 160 }, 
	{ "Ten", 10 }, 
	{ "Tenant", 349 },
	{ "Tennis", 375 }, 
	{ "Test", 416 }, 
	{ "Theater", 316 }, 
	{ "Thermostat", 267 }, 
	{ "Third", 161 }, 
	{ "Thirteen", 13 }, 
	{ "Thirty", 17 }, 
	{ "Three", 3 }, 
	{ "Thursday", 275 }, 
	{ "Time", 249 }, 
	{ "Tone", 162 }, 
	{ "Top", 163 }, 
	{ "Transmitter", 262 }, 
	{ "Trespassing", 391 }, 
	{ "Trouble", 164 }, 
	{ "Tuesday", 273 }, 
	{ "Twelve", 12 }, 
	{ "Twenty", 16 }, 
	{ "Two", 2 }, 
	{ "Type", 336 }, 
	{ "Under", 165 }, 
	{ "Unit", 166 }, 
	{ "Unoccupied", 282 }, 
	{ "Up", 167 }, 
	{ "User", 168 }, 
	{ "Utility", 169 }, 
	{ "Vacation", 269 }, 
	{ "Valve", 378 }, 
	{ "Vanity", 420 }, 
	{ "VCR", 293 }, 
	{ "Video", 315 }, 
	{ "Volume", 389 }, 
	{ "Waiting", 265 }, 
	{ "Walk", 170 }, 
	{ "Warehouse", 266 }, 
	{ "Warning", 371 }, 
	{ "Water", 196 }, 
	{ "Wednesday", 274 }, 
	{ "Welcome", 289 }, 
	{ "West", 171 }, 
	{ "Window", 172 }, 
	{ "Windows", 263 }, 
	{ "Wing", 411 }, 
	{ "Women's", 264 }, 
	{ "Work", 298 }, 
	{ "Yard", 228 }, 
	{ "Year", 425 }, 
	{ "Yellow", 368 }, 
	{ "You", 357 }, 
	{ "Zero", 0 }, 
	{ "Zone", 173 }, 
	{ "Zones", 174 },
};

int apex_word_code(const char* szWord)
{
	for (int i = 0; i < sizeof(g_words)/sizeof(g_words[0]); i += 1)
	{
		if (strcasecmp(szWord, g_words[i].m_szWord) == 0)
			return g_words[i].m_nCode;
	}
	
	return -1;
}

int apex_checksum(const char* pch, int cch)
{
	int cs = 0;
	while (cch > 0)
	{
		cs += *pch;
		pch += 1;
		cch -= 1;
	}
	
	return -cs & 255;
}

void apex_cmd(int fd, const char* szCmd)
{
	char cmd [256];
	int cch = sprintf(cmd, "%02X%s00", 6 + strlen(szCmd), szCmd);
	cch += sprintf(cmd + cch, "%02X\r\n", apex_checksum(cmd, cch));
	printf("> %s\n", cmd);
				
	if (write(fd, cmd, cch) != cch)
		fprintf(stderr, "write to port failed: %d\n", errno);
}

void apex_say(int fd, const char* szPhrase)
{
	const char* pch = szPhrase;
	for (;;)
	{
		while (*pch == ' ')
			pch += 1;
			
		if (*pch == '\0')
			break;
		
		char szWord [128];
		char* pchDest = szWord;
		
		while (*pch != '\0' && *pch != ' ')
			*pchDest++ = *pch++;
		*pchDest = '\0';
		
		int nCode = apex_word_code(szWord);
		if (nCode >= 0)
		{
			char szCmd [64];
			sprintf(szCmd, "si%03d", nCode);
			apex_cmd(fd, szCmd);
		}
	}
}

int apex_open(const char* szPort)
{
	int fd = open(szPort, O_RDWR | O_NOCTTY);
	if (fd < 0)
	{
		fprintf(stderr, "Can't open %s!\n", szPort);
		return -1;
	}
	
	struct termios tio;
	bzero(&tio, sizeof(tio));
	tio.c_cflag = B1200 /*| CRTSCTS*/ | CS8 | CLOCAL | CREAD;
	tio.c_iflag = IGNPAR;
	tio.c_oflag = 0;
	tio.c_lflag = 0;
	tio.c_cc[VTIME] = 0;
	tio.c_cc[VMIN] = 1; // blocking read until 5 chars received
	tcflush(fd, TCIOFLUSH);
	if (tcsetattr(fd, TCSANOW, &tio) != 0)
	{
		fprintf(stderr, "Can't configure port %s (err=%d)\n", szPort, errno);
		return -1;
	}
	
//	printf("apex_open %d\n", fd);
	
	return fd;
}

int decodehex1(char ch)
{
	if (ch >= '0' && ch <= '9')
		return ch - '0';
	if (ch >= 'A' && ch <= 'F')
		return ch - 'A' + 10;
	if (ch >= 'a' && ch <= 'f')
		return ch - 'a' + 10;
	return 0;
}

int decodehex2(const char* pch)
{
	return decodehex1(pch[0]) * 16 + decodehex1(pch[1]);
}

int decodedec2(const char* pch)
{
	return decodehex1(pch[0]) * 10 + decodehex1(pch[1]);
}

const char* rgszEventTypes [] = 
{
	"Exterior Instant",
	"Exterior Delay 1",
	"Exterior Delay 2",
	"Interior Instant",
	"Interior Delay 1",
	"Interior Delay 2",
	"Fire",
	"Panic",
	"Silent",
	"Emergency",
	"Follower",
	"Aux type 1 or 2",
	"Duress",
	"Duress not Armed",
	"Zone restore after activation",
	"Transmitter low battery",
	"Transmitter low battery restore",
	"Zone Trouble",
	"Zone Trouble Restore",
	"Fuse Trouble",
	"Fuse Trouble Restore",
	"Phone Line Restore",
	"Disarm",
	"Disarm After Activate",
	"Arm",
	"Arm With Zones Open",
	"Control Low Battery",
	"Control Battery Restore",
	"AC Fail",
	"AC Restore",
	"User Communicator Test",
	"Auto Communicator Test",
	"Cancel Alert",
	"Zone Bypass",
	"Zone Unbypass",
	"Day Zone Trouble",
	"Day Zone Trouble Restore",
	"Up/Download Attempt",
	"Program Mode Entered",
	"Fail to Disarm",
	"Fail to Arm",
	"HWB-416 Extender Trouble",
	"HWB-416 Trouble Restore",
	"Zone Open",
	"Zone Restore (close)"
};

void DumpSystemEvent(const char* buffer)
{
	int nEventType = decodehex2(buffer + 4);
	int nZone = decodedec2(buffer + 6);
	int mm = decodedec2(buffer + 8);
	int hh = decodedec2(buffer + 10);
	int dd = decodedec2(buffer + 12);
	int xx = decodedec2(buffer + 14);
	
	const char* szEvent = "???";
	if (nEventType < sizeof(rgszEventTypes)/sizeof(rgszEventTypes[0]))
		szEvent = rgszEventTypes[nEventType];
	
	time_t t = time(NULL);
	char szTime [64];
	strftime(szTime, sizeof(szTime), "%F %T", gmtime(&t));
	printf("%s: System Event: type=%02X \"%s\" zone=%d %d-%d %d:%02d\n", szTime, nEventType, szEvent, nZone, xx, dd, hh, mm);
}

int main(int argc, const char* argv [])
{
	const char* szPort = "/dev/ttyUSB1";
	int fd = apex_open(szPort);
	
	for (;;)
	{
//		printf("loop...\n");

		struct pollfd pfd [2];
		
		pfd[0].fd = fd;
		pfd[0].events = POLLIN;
		pfd[0].revents = 0;
		
		pfd[1].fd = 0; // stdin
		pfd[1].events = POLLIN;
		pfd[1].revents = 0;
		
		int p = poll(pfd, 2, -1/*10 * 1000*/);
		
		if (p < 0)
		{
			fprintf(stderr, "poll failed %d\n", errno);
			exit(1);
		}
		
		if (p == 0)
		{
			printf("poll timeout\n");
			continue;
		}
		
		if (pfd[0].revents & POLLIN)
		{
			char buffer [256];
			int cb = read(pfd[0].fd, buffer, sizeof(buffer));
//			printf("received (%d) %s\n", cb, buffer);
			write(1, buffer, cb);
			write(1, "\r\n", 2);
			
			if (buffer[2] == 'N' && buffer[3] == 'Q')
			{
				// System Event
				DumpSystemEvent(buffer);
			}
		}
		
		if (pfd[1].revents & POLLIN)
		{
			char buffer [256];
			int cb = read(pfd[1].fd, buffer, sizeof(buffer));
			if (cb > 0)
			{
				if (buffer[cb - 1] != '\n')
				{
					fprintf(stderr, "bad input line\n");
				}
				else
				{
					cb -= 1;
					buffer[cb] = '\0';
					
					if (strncmp(buffer, "say ", 4) == 0)
					{
						apex_say(fd, buffer + 4);
					}
					else
					{
						apex_cmd(fd, buffer);
					}
				}
			}
		}
	}
}
