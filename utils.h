#ifndef _UTILS_H
#define _UTILS_H

// ESP32 Utilities

#include "esp_system.h"


#ifdef DEBUG_MODE
#undef DEBUG_MODE
#endif // DEBUG_PRINTF
#define DEBUG_MODE() SYSTEMdebug

#ifndef DEBUG_PRINTF
// #undef DEBUG_PRINTF
#define DEBUG_PRINTF(f_, ...)
#endif //DEBUG_PRINTF

#define UTILS_INFINITELOOP -333

bool SYSTEMdebug = false;

#define DEBUG_PRINTF(f_, ... ) if( SYSTEMdebug ) { rprintf((f_), ##__VA_ARGS__); }

//------------------------------------------------------------------------------

struct MDNShost {
	String		hostname;
	IPAddress	ip;
	uint16_t 	port;
};

extern bool    MDNSinited;

//------------------------------------------------------------------------------
// prototypes

char * strca1sestr(const char *, const char *);
int StringSplit(String sInput, char cDelim, String sParams[], int iMaxParams );
boolean isint( String value );
void rprintf( const char *, ... );
String getMacAddress( bool compact = false );
String getIPAddress( void );
void remoteprintinit( void );
void setdebugmode( bool );
String getMacAddress( bool );
void utilsinit( void );
void remoteprintloop( );
void initwatchdog( int period = 0 );
void setwatchdogtimeout( int period = 0 );
void setwdogtimeout_msec( int period );
void disablewatchdog( void );
void feedwatchdog( void );
void MDNSinit( void );
int MDNSbrowseservice(const char * , const char *);
struct MDNShost * MDNSgethost( int );
void utilsloop( void );

typedef void (*reatartprolog_t)();

void registerprerestart( reatartprolog_t onrestart );
void restart( void );
void prerestart( void );
bool numtobool( int );
size_t getstacksize( bool show = true );
void utilsresetloop( int count = -1 );

//----------------------------------------------------------------------
// Source: https://stackoverflow.com/questions/650162/why-cant-the-switch-statement-be-applied-to-strings/46711735#46711735
// Use this hash function for string based switch case construction:
// switch( hash(str) ){
// case hash("one") : // do something
// case hash("two") : // do something
// }
// 
// In C++14 and C++17 you can use following hash function:
// Also C++17 have std::string_view, so you can use it instead of const char *.
// In C++20, you can try using consteval.
// constexpr uint32_t hash(const char* data, size_t const size) noexcept{
//     uint32_t hash = 5381;

//     for(const char *c = data; c < data + size; ++c)
//         hash = ((hash << 5) + hash) + (unsigned char) *c;

//     return hash;
// }

// C++11
constexpr unsigned int hash(const char *s, int off = 0) {                        
    return !s[off] ? 5381 : (hash(s, off+1)*33) ^ s[off];                           
}    

//------------------------------------------------------------------------------
// This is for backward compatibility for earlier projects
// The folowing DSTime code is based on the original arduiono iplementation
// https://github.com/arduino-libraries/NTPClient

#include <time.h>
// #include <NTPClient.h>
// extern NTPClient 	timeClient;
//
//
// #pragma once
//
// #include "Arduino.h"
//
// #include <Udp.h>
//
// #define SEVENZYYEARS 2208988800UL
// #define NTP_PACKET_SIZE 48
// #define NTP_DEFAULT_LOCAL_PORT 1337

// struct tm
// {
//   int	tm_sec;
//   int	tm_min;
//   int	tm_hour;
//   int	tm_mday;
//   int	tm_mon;
//   int	tm_year;
//   int	tm_wday;
//   int	tm_yday;
//   int	tm_isdst;
// #ifdef __TM_GMTOFF
//   long	__TM_GMTOFF;
// #endif
// #ifdef __TM_ZONE
//   const char *__TM_ZONE;
// #endif
// };

class DSTTime {
  private:
    // UDP*          _udp;
    // bool          _udpSetup       = false;
	//
    // const char*   _poolServerName = "time.nist.gov"; // Default time server
    // int           _port           = NTP_DEFAULT_LOCAL_PORT;
    // int           _timeOffset     = 0;
	//
    // unsigned int  _updateInterval = 60000;  // In ms
	//
    // unsigned long _currentEpoc    = 0;      // In s
    // unsigned long _lastUpdate     = 0;      // In ms
	//
    // byte          _packetBuffer[NTP_PACKET_SIZE];
	//
    // void          sendNTPPacket();

	long gmtOffset_sec;
	int daylightOffset_sec;
	const char* server1;
	const char* server2;
	const char* server3;
	const char* timezone = NULL;
	unsigned long lasterrorprint = 0;

	void errorprint( const char *errormsg )
	{
		if( (millis() - lasterrorprint) > 15000 )
		{
			rprintf( errormsg );
			lasterrorprint = millis();
		}
	}
  public:

 	DSTTime( long param_gmtOffset_sec, int param_daylightOffset_sec, const char* param_server1, const char* param_server2, const char* param_server3 , const char* parma_timezone = NULL )
	{
		// rprintf( "Time intitialised...\n");
		gmtOffset_sec = param_gmtOffset_sec;
		daylightOffset_sec = param_daylightOffset_sec;
		server1 = param_server1;
		server2 = param_server2;
		server3 = param_server3;
		timezone = parma_timezone;
	}

    /**
     * Starts the underlying UDP client with the default local port
     */
    void begin()
	{
		configTime( 0 , 0 , server1 , server2 , server3 );
		// configTime( gmtOffset_sec , daylightOffset_sec , server1 , server2 , server3 );

		struct timezone tz={0,0};
		struct timeval tv={0,0};

		//  https://stackoverflow.com/questions/56412864/esp8266-timezone-issues/56422600#56422600
		settimeofday(&tv, &tz);
		const char* tmz = timezone;

		if( tmz == NULL  )
		{
			// Default is CEST
			// https://github.com/nayarsystems/posix_tz_db
			// "Europe/Budapest","CET-1CEST,M3.5.0,M10.5.0/3"
			tmz = "CET-1CEST,M3.5.0,M10.5.0/3";
		}
		// int setenv(const char *name, const char *value, int overwrite);
		setenv("TZ", tmz , 1);
		tzset();
	};

    /**
     * Starts the underlying UDP client with the specified local port
     */
    void begin(int port) {};

    /**
     * This should be called in the main loop of your application. By default an update from the NTP Server is only
     * made every 60 seconds. This can be configured in the NTPClient constructor.
     *
     * @return true on success, false on failure
     */
    bool update() { return false; };

    /**
     * This will force the update from the NTP Server.
     *
     * @return true on success, false on failure
     */
    bool forceUpdate() { return false; };

    int getDay()
	{
		struct tm timeinfo;
		if(!getLocalTime(&timeinfo))
		{
			errorprint("Failed to obtain time\n");
		}
		return timeinfo.tm_wday;
	};
    int getHours()
	{
		struct tm timeinfo;
		if(!getLocalTime(&timeinfo))
		{
			errorprint("Failed to obtain time\n");
		}
		return timeinfo.tm_hour;
	}
    int getMinutes()
	{
		struct tm timeinfo;
		if (!getLocalTime(&timeinfo))
		{
			errorprint("Failed to obtain time\n");
		}
		return timeinfo.tm_min;
	}
    int getSeconds()
	{
		struct tm timeinfo;
		if(!getLocalTime(&timeinfo))
		{
			errorprint("Failed to obtain time\n");
		}
		return timeinfo.tm_sec;
	}

    /**
     * Changes the time offset. Useful for changing timezones dynamically
     */
    void setTimeOffset(int timeOffset) {};

    /**
     * Set the update interval to another frequency. E.g. useful when the
     * timeOffset should not be set in the constructor
     */
    void setUpdateInterval(int updateInterval) {};

    /**
     * @return time formatted like `hh:mm:ss`
     */
    String getFormattedTime()
	{
		char	date[128];
		struct tm timeinfo;
		if(!getLocalTime(&timeinfo))
		{
			rprintf("Failed to obtain time\n");
		}
		// rprintf( "asctime: %s\n" , asctime(&timeinfo) );
		snprintf( date , sizeof(date), "%d-%02d-%02d %2d:%02d:%02d" ,
			timeinfo.tm_year+1900,timeinfo.tm_mon+1,timeinfo.tm_mday,
			timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec
		);
		// return String( asctime(&timeinfo) );
		return String( date );
	}

    /**
     * @return time in seconds since Jan. 1, 1970
     */
    unsigned long getEpochTime() { return 0; };

    /**
     * Stops the underlying UDP client
     */
    void end() {};
};

extern DSTTime 	timeClient;

//------------------------------------------------------------------------------
// FIXME
// error: 'min' was not declared in this scope
// #define min(a,b) ((a)<(b)?(a):(b))
#include <algorithm>

//------------------------------------------------------------------------------

#include <utils.ino>

#endif // _UTILS_H
