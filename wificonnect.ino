#include <WiFi.h>
#include <ledblink.h>
#include <wificonnect.h>

//------------------------------------------------------------------------------
// WIFI settings
const char* wifi_ssid = NULL;
const char* wifi_password = NULL;
bool    wifiinited = false;


bool iswificonnected( void )
{
    if( wifiinited == true && WiFi.status() == WL_CONNECTED )
    {
        return true;
    }
    return false;
}

void WifiConnect( const char* ssid , const char* psw )
{
  int trycount = 0;

  if ((WiFi.status() == WL_CONNECTED))
  {
    return;
  }
  //----------------------------------------------------------------------------
  if( ssid != NULL )
  {
      wifi_ssid = ssid;
  }

  if( psw != NULL )
  {
      wifi_password = psw;
  }
  //rprintff("SSID: %s PSW: %s\n", wifi_ssid , wifi_password );

  //----------------------------------------------------------------------------
    do
    {
        // Reset wifi
        Serial.printf("Reset Wifi...\n");

        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // FIXME: this recconnect method is not the best approach
        WiFi.disconnect(true);
        Serial.printf("Wifi reset success...\n");
        if( trycount )
            delay(1000);
        // Connect to Wifi
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

        // config WiFi as client
        WiFi.mode(WIFI_STA);
        WiFi.begin(wifi_ssid, wifi_password);
        Serial.printf("Connecting to WiFi network: ");
        for( int i = 0 ; WiFi.status() != WL_CONNECTED && i < 25 ; i++ )
        {
            ledblink();
            delay(250);
            Serial.printf(".");
        }
        Serial.printf("\n");
        trycount++;
    } while( WiFi.status() != WL_CONNECTED && trycount < 2 );

    Serial.printf("Successfully connected!\n" );

    if( WiFi.status() == WL_CONNECTED )
    {
        // IPAddress ip = WiFi.localIP();
        // Serial.printf("Successfully connected! IP: %s\n", ip.toString().c_str() );
        Serial.printf("Successfully connected! IP: %s\n", WiFi.localIP().toString().c_str() );
        ledblink( HIGH );
        wifiinited = true;
    }
    else
    {
        Serial.printf("Cannot connect to the WiFi network! Reset the app.\n" );
        for( int i = 0 ; i < 10 ; i++ )
        {
            delay(50);
            ledblink();
        }
        ESP.restart();
    }
}
