// #ifndef _UTILS
// #define _UTILS

// ESP32 Utilities
#include <Esp.h>
#include "esp_system.h"
#include <utils.h>
#include <wificonnect.h>
#include <ota.h>
#include <Arduino_CRC32.h>

//------------------------------------------------------------------------------

String devicebooted;

//------------------------------------------------------------------------------
// Source: https://android.googlesource.com/platform/bionic/+/fe6338d/libc/string/strcasestr.c
/*
 * Find the first occurrence of find in s, ignore case.
 */

char * strcasestr(const char *s, const char *find)
{
	char c, sc;
	size_t len;
	if ((c = *find++) != 0) {
		c = (char)tolower((unsigned char)c);
		len = strlen(find);
		do {
			do {
				if ((sc = *s++) == 0)
					return (NULL);
			} while ((char)tolower((unsigned char)sc) != c);
		} while (strncasecmp(s, find, len) != 0);
		s--;
	}
	return ((char *)s);
}

//----------------------------------------------------------------------

bool numtobool( int value )
{
	return (value == 0 ) ? false : true;
}

//----------------------------------------------------------------------
/* Time Stamp */

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// FIXME:
// There is no DST handling in NTP library!!!
// Sources for DST
// Using ESP standard functions
// https://github.com/espressif/arduino-esp32/blob/master/libraries/ESP32/examples/Time/SimpleTime/SimpleTime.ino
// struct tm timeinfo;
// configTime(5*3600, 3600, "my.ntp.server", NULL, NULL); // EST5EDT
// while (!getLocalTime(&timeinfo));
// Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
// https://knowledgebase.progress.com/articles/Article/000044163
// setenv("TZ", "EST5EDT4,M3.2.0/02:00:00,M11.1.0/02:00:00", 1);
// tzset();


// https://github.com/JChristensen/Timezone
//

// #include <NTPClient.h>
// #include <WiFiUdp.h>
//
// // FIXME: It should handle daylight saving !!!
// #define NTP_OFFSET  +1  * 60 * 60 // In seconds
// // #define NTP_OFFSET  +2  * 60 * 60 // In seconds
// #define NTP_INTERVAL 60 * 1000    // In miliseconds
// //#define NTP_ADDRESS  "0.pool.ntp.org"
// #define NTP_ADDRESS  "pool.ntp.org"
 // TODO remove this!!!
bool			bootloopcheck = true;
unsigned long   ntplastsync = 0;

// WiFiUDP 	ntpUDP;
// NTPClient 	timeClient(ntpUDP, NTP_ADDRESS, NTP_OFFSET, NTP_INTERVAL);

DSTTime		timeClient( 1 * 3600, 3600, "hu.pool.ntp.org", "pool.ntp.org", "time.cloudflare.com" );

//------------------------------------------------------------------------------
// https://github.com/espressif/arduino-esp32/blob/master/libraries/Preferences/src/Preferences.cpp
#include <Preferences.h>
Preferences Utilsprefs;

void setdebugmode( bool enable )
{
    SYSTEMdebug = enable;
    Utilsprefs.begin("UTILSprefs", false);
    Utilsprefs.putBool("SYSTEMdebug", enable );
    Utilsprefs.end();

}

//------------------------------------------------------------------------------

String getMacAddress( bool compact )
{
uint8_t baseMac[6];
char baseMacChr[18] = {0};
const char *formatstr;

	formatstr = compact == false ?
		"%02X:%02X:%02X:%02X:%02X:%02X" :
		"%02X%02X%02X%02X%02X%02X";
	// Get MAC address for WiFi station
	esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
	sprintf(baseMacChr, formatstr , baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
	return String(baseMacChr);
}

String getIPAddress( void )
{
	return WiFi.localIP().toString();
}

//------------------------------------------------------------------------------
//

void utilsresetloop( int count )
{
	bool infineite = false;
	// If count is negative that means infinite

	if( count == UTILS_INFINITELOOP )
	{
		infineite = true;
	}

	// Disable the loop reset the boot loop counter
	bootloopcheck = false;

	while( infineite == true || count-- < 0 )
	{
		WifiConnect();
		OTAHandle();
		utilsloop();
		delay( 150 );
	}
	ESP.restart();

}

//------------------------------------------------------------------------------

void utilsinit( void )
{

	remoteprintinit();
    if( Utilsprefs.begin("UTILSprefs", false) )
    {
        SYSTEMdebug = Utilsprefs.getBool("SYSTEMdebug", false);
        DEBUG_PRINTF( "SYSTEMdebug inited:%s\n", SYSTEMdebug ? "true" : "false" );
    }
    else
    {
        rprintf( "!!! WARNING: UTILS prefs restore failed!");
    }

	timeClient.begin();
	timeClient.update();

	devicebooted = timeClient.getFormattedTime();

	//--------------------------------------------------------------------------
	// Bootloop detection

	getstacksize();

	Preferences savedprefs;
	unsigned int bootcount = 0;

	if( savedprefs.begin("Prefs", false) )
	{
		bootcount = savedprefs.getUChar( "rebootcount", 0 );
		savedprefs.putUChar( "rebootcount" , bootcount+1 );

		rprintf( ">>> Reboot count is: %d\n" , bootcount );
	}

	if( bootcount > 5 )
	{
		char devicename[32];
		rprintf( "!!! ERROR: Bootloop detected. Enter emergency OTA state!\n");

		sprintf( devicename , "emergency-%s" , getMacAddress( true ).c_str());
		OTAinit( 3232 , devicename );

		// Reset the reboot count to give a chance for the update....
		savedprefs.putUChar( "rebootcount" , 0 );
		savedprefs.end();

		// // Disable the loop reset the boot loop counter
		// bootloopcheck = false;
		//
		// while( true )
		// {
		// 	WifiConnect();
		// 	OTAHandle();
		// 	utilsloop();
		// 	delay( 500 );
		// }
		// ESP.restart();

		utilsresetloop( UTILS_INFINITELOOP );

	}

	savedprefs.end();

	//--------------------------------------------------------------------------

	initwatchdog();
}

//------------------------------------------------------------------------------
// Debug out over Wifi
// Use nc <ip> 80
#include <WiFi.h>
#include <WiFiClient.h>
#include <WifiServer.h>

WiFiServer 	debugserver(80);
WiFiClient 	debugclient;
bool        debugclientwelcome = true;
bool		rprintinited = false;

// void rprintf( const char * format, ... );


void remoteprintloop( )
{
    if( rprintinited && !debugclient )
    {
        debugclient = debugserver.available();
    }
    if( rprintinited && debugclient && debugclient.connected() )
    {
        if( debugclientwelcome )
        {
            IPAddress   ip = debugclient.remoteIP();
            Serial.printf("Debug client connected from: %u.%u.%u.%u\n" , ip[0], ip[1], ip[2], ip[3]);
			#ifndef VERSION
			// #define version ""
			char	version[]="NOT DEFINED";
			#else
			char	version[64];
			sprintf( version , "VER: %s" , VERSION );
			#endif
            rprintf( "EHLO from: %s [%s] %s Current Time: %s Device Booted: %s\n", WiFi.localIP().toString().c_str() , getMacAddress().c_str(), version , timeClient.getFormattedTime().c_str() , devicebooted.c_str() );
            debugclientwelcome = false;
        }
    }
    else
    {
        debugclientwelcome = true;
    }
}

#ifndef BAUD
#define BAUD 115200
#endif //BAUD

SemaphoreHandle_t mutex_rptintf = NULL;


void remoteprintinit( void )
{
	if( mutex_rptintf == NULL )
	{
		if( iswificonnected() )
		{
			debugserver.begin();
			// FIXME: Init if not inited!
			//Serial.begin(BAUD);    // Initialise the serial port
			rprintinited = true;
		}
		else
		{
			Serial.printf("!!! ERROR: Remote print cannot init use without Wifi connection!\n");
		}

		// mutex_rptintf = xSemaphoreCreateMutex();
		vSemaphoreCreateBinary( mutex_rptintf );

	}

}

//------------------------------------------------------------------------------

unsigned int 	rp_filteredlines = 0;
uint32_t 		rp_lastcrc = 0;

void rprintf( const char * format, ... )
{
int 			obtained = pdTRUE;
char 			buffer[512];
char 			filtered[64] = "";
int 			size;
va_list 		args;
Arduino_CRC32 	crc32;

	va_start(args, format);

	size = vsnprintf(buffer,sizeof(buffer),format, args);
	va_end(args);

	if( mutex_rptintf != NULL )
	{
		obtained = xSemaphoreTake( mutex_rptintf, portMAX_DELAY );
		if( obtained != pdTRUE )
			return;
	}

	// Filter the duplicated lines
	uint32_t const crc = crc32.calc((uint8_t const *)buffer , size );

	if( rp_lastcrc == 0 || crc != rp_lastcrc )
	{
		bool filteredmsg = false;
		// Create filtered line text...
		if( rp_filteredlines )
		{
			sprintf( filtered , "... %d same line(s) was filtered\n" , rp_filteredlines );
			filteredmsg = true;
		}

		rp_filteredlines = 0;
		rp_lastcrc = crc;

		if(rprintinited && debugclient && debugclient.connected() )
		{
			if( filteredmsg )
			{
				debugclient.printf(filtered);
			}
			debugclient.printf(buffer);
		}
		else
		{
			if( filteredmsg )
			{
				debugclient.printf(filtered);
			}
			Serial.printf( buffer );
		}
	}
	else
	{
		rp_filteredlines++;
	}

    if( size > sizeof(buffer) )
    {
        Serial.printf("\n!!! WARNING: Buffer is shorter than necessary, output is truncated.\n");
    }
	if( mutex_rptintf != NULL )
	{
		xSemaphoreGive( mutex_rptintf );
	}

}

//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// FIXME !!!!
// This functions are created because the Acurite0899 firmware
// reset and MDNS rescan issue.
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

reatartprolog_t UTILSonrestart = NULL;

void registerprerestart( reatartprolog_t onrestart )
{
	UTILSonrestart = onrestart;
}


void prerestart( void )
{
	if( UTILSonrestart != NULL )
	{
		rprintf( ">>> RESTART IS BEGINNING...\n");
		UTILSonrestart();
	}

}

void restart( void )
{
		prerestart( );
		rprintf( ">>> RESTART THE ESP...\n");
		delay( 100 );
		ESP.restart();
}


//------------------------------------------------------------------------------
// Watchdog functions

hw_timer_t *WDTtimer = NULL;

void IRAM_ATTR WDmodulereset() {
  DEBUG_PRINTF("Watchdog timeout! Reboot the system...\n");
  // esp_restart_noos();
  esp_restart();
}

// period in sec

void initwatchdog( int period )
{

	if( WDTtimer == NULL )
	{
		WDTtimer = timerBegin(0, 80, true); //timer 0, div 80
	    timerAttachInterrupt(WDTtimer, &WDmodulereset, true);
	}

	setwatchdogtimeout( period );

    // timerAlarmWrite(WDTtimer, period , false); //   set watchdog time: 7 sec
	// timerStart(WDTtimer);
    // timerAlarmEnable(WDTtimer);     //enable interrupt
}

// Period in sec
void setwatchdogtimeout( int period )
{
	period = period == 0 ? 7000 : period * 1000;

	setwdogtimeout_msec( period );

}

// Period is millisec
void setwdogtimeout_msec( int period )
{

	rprintf("setwatchdogtimeout:: timeout value:%d\n", period );

	period *= 1000;

	if( WDTtimer != NULL )
	{
	 	timerAlarmDisable(WDTtimer);
		timerAlarmWrite(WDTtimer, period , false);
		timerStart(WDTtimer);
		feedwatchdog();
	    timerAlarmEnable(WDTtimer);     //enable interrupt
	}
	else
	{
		rprintf("setwatchdogtimeout:: Failed, because timer is not initiated!\n");
	}

	rprintf("setwatchdogtimeout:: Finished\n");

}

void disablewatchdog( void )
{
	if( WDTtimer != NULL )
	{
		timerAlarmDisable(WDTtimer);    // disable interrupt
	}
}

void feedwatchdog( void )
{
	if( WDTtimer != NULL )
	{
    	timerWrite(WDTtimer, 0);    //reset timer (feed watchdog)
	}
}

//------------------------------------------------------------------------------

void utilsloop( void )
{

	remoteprintloop();
	feedwatchdog();
	if( ( millis() - ntplastsync ) > 86000000 )
	{
		DEBUG_PRINTF("%s: -->> NTP Sync...\n" , timeClient.getFormattedTime().c_str());
		ntplastsync = millis();
		timeClient.update();
	}

	//--------------------------------------------------------------------------
	//  Clear the reboot counter if still running
	if( ( millis() - ntplastsync ) > 60000 && bootloopcheck )
	{
		// clear boot count
		bootloopcheck = false;
		Preferences savedprefs;
		unsigned int bootcount = 0;

		rprintf( "\n\n>>> Checking the reboot counter...\n\n" );

		if( savedprefs.begin("Prefs", false) )
		{
			bootcount = savedprefs.getUChar( "rebootcount", 0 );
			if ( bootcount > 0 )
			{
				if( bootcount > 1 )
				{
					rprintf( "!!! NOTICE: Reset the reboot counter!\n" );
				}
				savedprefs.putUChar( "rebootcount" , 0 );
			}
			savedprefs.end();
		}
	}
}

//------------------------------------------------------------------------------

#include <Wire.h>

#define I2C_SDA 21
#define I2C_SCL 22

void i2cdevicesacan( )
{

    Wire.begin(I2C_SDA, I2C_SCL);

    byte error, address;
    int nDevices;

    rprintf("Scanning...");
    nDevices = 0;
    for(address = 1; address < 127; address++ )
    {
		// The i2c_scanner uses the return value of
		// the Write.endTransmisstion to see if
		// a device did acknowledge to the address.
		Wire.beginTransmission(address);
		error = Wire.endTransmission();
		if (error == 0)
		{
		rprintf("I2C device found at address 0x%02X\n" , address);
		nDevices++;
		}
		else if (error==4)
		{
		rprintf("Unknown error at address 0x%02x\n" , address);
		}
    }

    if (nDevices == 0)
	{
		rprintf("No I2C devices found\n");
	}
    else
	{
		rprintf("done\n");
	}

}

//------------------------------------------------------------------------------
#include <ESPmDNS.h>

// struct MDNShost {
// 	String		hostname;
// 	IPAddress	ip;
// 	uint16_t 	port;
// };

bool    MDNSinited = false;

void MDNSinit( void )
{
	char	host[128];

    if( !MDNSinited )
    {
        rprintf("Init MDNS...\n");
        sprintf( host , "ESP32_Browser:%s" , getMacAddress( true ).c_str() );
    	if( !MDNS.begin(host))
    	{
    		rprintf("Error setting up MDNS responder!\n");
    	}
    }
}

int MDNSbrowseservice(const char * service, const char * proto)
{
	for( int retry = 0; retry < 5 ; retry++ )
	{
		utilsloop();
	    rprintf("Browsing for service _%s._%s.local. ... ", service, proto);
	    int n = MDNS.queryService( service, proto );
	    if (n == 0)
		{
	        rprintf("no services found\n");
	    }
		else
		{
			rprintf( " %d service(s) found\n" , n );
	        for (int i = 0; i < n; ++i)
			{
	            // Print details for each service found
				rprintf( " %d: %s (%u.%u.%u.%u:%d)\n" , i+1 , MDNS.hostname(i).c_str() , MDNS.IP(i)[0], MDNS.IP(i)[1], MDNS.IP(i)[2], MDNS.IP(i)[3] , MDNS.port(i) );
	        }
			return n;
	    }
	    //Serial.println();
		for( int i = 0; i < 10 ; i++ )
		{
			delay(100);
			utilsloop();
		}
	}
	//MDNSinit();
	return 0;
}

struct MDNShost * MDNSgethost( int idx )
{
	struct MDNShost * host = new struct MDNShost;
	host->hostname = MDNS.hostname(idx);
	host->ip = MDNS.IP(idx);
	host->port = MDNS.port(idx);

	return host;
}

size_t getstacksize( bool show )
{

void * StackPtrAtStart;
void * StackPtrEnd;
UBaseType_t watermarkStart;
void* SpStart = NULL;
size_t stacksize;
void* SpActual = NULL;


	StackPtrAtStart = (void *)&SpStart;
	watermarkStart =  uxTaskGetStackHighWaterMark(NULL);
	StackPtrEnd = StackPtrAtStart - watermarkStart;

	stacksize = (uint32_t)&SpActual - (uint32_t)StackPtrEnd;

	if( show )
	{
		Serial.printf("\n>>> Free Stack at actual position is: %d \r\n", stacksize );
	}

	return stacksize;
}


// #endif // _UTILS
