//------------------------------------------------------------------------------
// OTA
#include <Esp.h>
#include <WiFi.h>
#include <string.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

//------------------------------------------------------------------------------

#ifndef DEBUG_PRINTF
#define _OTA_DEBUG_PRINTF
// #define DEBUG_PRINTF(f_, ...)
#endif // DEBUG_PRINTF

#include <utils.h>
#include <ledblink.h>

const char *OTAhostname = NULL;
const char * OTApsw = NULL;
int OTAPort = 3232;
bool OTAstarted = false;
bool OTArestart = false;
bool OTAinited = false;
unsigned long OTAheartbit = 0;
voidfunc_t OTAonstart = NULL;

//------------------------------------------------------------------------------
// void OTAinit( int port = 0 , const char *hostname = NULL , const char *password = NULL , voidfunc_t osntart = NULL )
void OTAinit( int port , const char *hostname , const char *password , voidfunc_t osntart )
{
    ledblink( LOW );

    //--------------------------------------------------------------------------
    OTAonstart = osntart;
    if( hostname != NULL )
    {
        OTAhostname = hostname;
    }
    else if( OTAhostname == NULL )
    {
        OTAhostname = "esp32";
    }
    if( port != 0 )
    {
        OTAPort = port;
    }
    //--------------------------------------------------------------------------

    port = port == 0 ? OTAPort : port;
    hostname = hostname == NULL ? OTAhostname : hostname;

    DEBUG_PRINTF( "OTA init. port: %d; hostname: %s\n" , port , hostname );
    ArduinoOTA.setPort( port );
    /* we use mDNS instead of IP of ESP32 directly */
    ArduinoOTA.setHostname( hostname );
    /* we set password for updating */
    ArduinoOTA.setPassword( password == NULL ? OTApsw : password );
    /* this callback function will be invoked when updating start */
    ArduinoOTA.onStart( []()
        {
            OTAstarted = true;
            rprintf("\n>>> OTA Start updating\n");
            if( OTAonstart != NULL )
            {
                OTAonstart();
            }

        });
    /* this callback function will be invoked when updating end */
    ArduinoOTA.onEnd([]()
        {
            rprintf("\n>>> OTA updating finished succesfully!\n");
            //OTArestart = true;
            ledblink( LOW );
        });
    /* this callback function will be invoked when a number of chunks of software was flashed
    so we can use it to calculate the progress of flashing */
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
        {
            //rprintf("Progress: %u%%\r", (progress / (total / 100)));
            ledblink( );
        });

    /* this callback function will be invoked when updating error */
    ArduinoOTA.onError([](ota_error_t error)
        {
            rprintf("\nError[%u]: ", error);
            if (error == OTA_AUTH_ERROR) rprintf("Auth Failed\n");
            else if (error == OTA_BEGIN_ERROR) rprintf("Begin Failed\n");
            else if (error == OTA_CONNECT_ERROR) rprintf("Connect Failed\n");
            else if (error == OTA_RECEIVE_ERROR) rprintf("Receive Failed\n");
            else if (error == OTA_END_ERROR) rprintf("End Failed\n");
            ESP.restart();
        });

    ArduinoOTA.setTimeout( 5000 );
    //ArduinoOTA.setMdnsEnabled( false );
    /* start updating */
    ArduinoOTA.begin();
#ifdef _UTILS
    MDNSinited = true;
#endif // _UTILS
    OTAinited = true;
    DEBUG_PRINTF( "OTA init finished...\n" );
}

//------------------------------------------------------------------------------
// Call this function from main loop regulary

void OTAHandle( void )
{
    // if( OTArestart == true )
    // {
    //     OTArestart = false;
    //     delay( 1000 );
    //     ESP.restart();
    // }

    ArduinoOTA.handle();

    if(  (millis() - OTAheartbit) > 7200000 && OTAstarted == false )
    {
        rprintf(">>> OTA reinit\n");
        OTAheartbit = millis();
        // WORKAROUND: 2018.07.15. Based on my experience the OTA
        // could be disapear from the network after a long time
        // Based on this OTA should reinit regulary
        OTAinit();
    }
}
#ifdef _OTA_DEBUG_PRINTF
#undef DEBUG_PRINTF
#endif //_OTA_DEBUG_PRINTF
