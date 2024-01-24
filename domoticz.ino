#include <HTTPClient.h>
#include <string.h>
#include <ArduinoJson.h>

// FIXME
#include <utils.h>
#include <wificonnect.h>

//------------------------------------------------------------------------------

//void updatedomoticz_svalue( const char * domo , int idx , int nvalue , const char * svalue , int battery = -1 )
void updatedomoticz_svalue( const char * domo , int idx , int nvalue , const char * svalue , int battery )
{
  HTTPClient http;
  char url[256];

    if ((WiFi.status() != WL_CONNECTED))
    {
      WifiConnect();
    }

    if ((WiFi.status() == WL_CONNECTED))
    {
        ledblink();
        rprintf( "Device %d update:: nvalue: %d svalue: %s battery: %d\n" , idx,nvalue,svalue,battery);

        // TEMP update
        // /json.htm?type=command&param=udevice&idx=IDX&nvalue=0&svalue=TEMP
        // Rain update
        // /json.htm?type=command&param=udevice&idx=IDX&nvalue=0&svalue=RAINRATE;RAINCOUNTER
        //sprintf( url , "http://192.168.0.200:9180/json.htm?type=command&param=udevice&idx=%d&nvalue=%d&svalue=%s", idx , nvalue , svalue );
        sprintf( url , "http://%s/json.htm?type=command&param=udevice&idx=%d&nvalue=%d&svalue=%s" , domo , idx , nvalue , svalue );
        if( battery != -1 )
        {
            char t[32];
            sprintf( t , "&battery=%d" , battery );
            strcat( url , t );
        }

        rprintf( "%s\n" , url );
        // ESP32 use seconds
        http.setTimeout(2);
        http.begin(url); //Specify the URL
        int httpCode = http.GET();

        //Check for the returning code
        if (httpCode == HTTP_CODE_OK )
        {
            // OK
            String payload = http.getString();
            DEBUG_PRINTF("HTTP response: %d Payload: %s\n" , httpCode , payload.c_str() );
        }
        else
        {
            rprintf("!!! ERROR: HTTP request error: %d Payload: %d\n" , httpCode , http.getString().c_str() );
            //syslog.logf( LOG_ERR , "HTTP request ERROR! Error code: %d Response: %s", httpCode ,  http.errorToString(httpCode).c_str() );
        }
        //Free the resources
        http.end();
        // Feed the watchdog
        utilsloop();
        ledblink();
    }
    else
    {
      rprintf("Wifi connection error!\n");
    }

}

void updatedomoticz_temp( const char * domo ,  int idx , float temp , float hum )
{
  HTTPClient http;
  char url[256];

    if ((WiFi.status() != WL_CONNECTED))
    {
        WifiConnect();
    }

    if ((WiFi.status() == WL_CONNECTED))
    {
        ledblink();
        // Test.Temp Device 859
        // /json.htm?type=command&param=udevice&idx=IDX&nvalue=0&svalue=TEMP
        //json.htm?type=command&param=udevice&idx=IDX&nvalue=0&svalue=TEMP;HUM;HUM_STAT
        char fstr[128] = "http://%s/json.htm?type=command&param=udevice&idx=%d&nvalue=0&svalue=%.2f";
        int  humstat = 0;
        if( hum > 0 )
        {
            if( hum > 65 )
                humstat = 3;
            else if( hum <= 60 && hum >= 45 )
                humstat = 1;
            else if( hum < 40 )
                humstat = 2;

            strcat( fstr , ";%.2f;%d" );
        }

        sprintf( url , fstr , domo , idx , temp , hum , humstat );

        rprintf( "%s\n" , url );
        // ESP32 use seconds
        http.setTimeout(2);
        http.begin(url);
        int httpCode = http.GET();
        if (httpCode > 0)
        {
            // char t[32];
            String payload = http.getString();
            // sprintf( t , "HTTP response: %d", httpCode );
            rprintf( "HTTP response: %d\n%s\n", httpCode , payload.c_str() );
            // Serial.println( t );
          // Serial.println(payload);
        }
        else
        {
            rprintf("Error on HTTP request\n");
        }

        //Free the resources
        http.end();
        ledblink();
        // Feed the watchdog
        utilsloop();
    }
    else
    {
      rprintf("Wifi connection error!\n");
    }

}

unsigned int _getdomoticz_temp( const char * domo ,  int idx , float * temp , float * humidity )
{
DynamicJsonDocument root(1700);
HTTPClient http;
char url[256];

    if ((WiFi.status() != WL_CONNECTED))
    {
        WifiConnect();
    }

    if ((WiFi.status() == WL_CONNECTED))
    {
        ledblink();
        // Test.Temp Device 859
        // http://192.168.0.200:9180/json.htm?type=devices&rid=560
        sprintf( url , "http://%s/json.htm?type=devices&rid=%d" , domo , idx );

        rprintf( "%s\n" , url );
        // ESP32 use seconds
        http.setTimeout(2);
        http.begin(url); //Specify the URL
        int httpCode = http.GET();

        if (httpCode > 0)
        {
            // char t[32];
            int bodylen = http.getSize();
            rprintf( "HTTP response size: %d\n", bodylen );

            DeserializationError err = deserializeJson(root, http.getString() );
            if( err == DeserializationError::Ok  && root.containsKey("result") )
            {
                JsonObject result = root["result"][0];
                if( result.containsKey("Temp") &&
                    (humidity == NULL || result.containsKey("Humidity") )
                )
                {
                    *temp = result["Temp"].as<float>();
                    if( humidity != NULL )
                    {
                        *humidity = result["Humidity"].as<float>();
                    }
                    return true;
                }
                else
                {
                    rprintf("Invalid result JSON!\n");
                }
            }
            else
            {
                rprintf("JSON parsing error!\n");
            }
        }
        else
        {
            rprintf("Error on HTTP request\n");
        }

        //Free the resources
        http.end();
        ledblink();
        // Feed the watchdog
        utilsloop();
    }
    else
    {
        rprintf("Wifi connection error!\n");
    }

    return false;
}

unsigned int getdomoticz_temp( const char * domo ,  int idx , float &temp )
{
    return _getdomoticz_temp( domo , idx , &temp , NULL );
}

unsigned int _getdomoticz_temp( const char * domo ,  int idx , float &temp , float &humidity )
{
    return _getdomoticz_temp( domo , idx , &temp , &humidity );
}

unsigned int getdomoticz_counter( const char * domo ,  int idx , float * value , float * todayvalue )
{
DynamicJsonDocument root(1700);
HTTPClient http;
char url[256];

    if ((WiFi.status() != WL_CONNECTED))
    {
        WifiConnect();
    }

    if ((WiFi.status() == WL_CONNECTED))
    {
        ledblink();
        // Test.Temp Device 859
        // http://192.168.0.200:9180/json.htm?type=devices&rid=560
        sprintf( url , "http://%s/json.htm?type=devices&rid=%d" , domo , idx );

        rprintf( "%s\n" , url );
        // ESP32 use seconds
        http.setTimeout(2);
        http.begin(url); //Specify the URL
        int httpCode = http.GET();                                        //Make the request

        if (httpCode > 0)
        {
            // char t[32];
            // String payload = http.getString();
            // sprintf( t , "HTTP response: %d", httpCode );
            // rprintf( "HTTP response: %d\n%s\n", httpCode , payload.c_str() );
            int bodylen = http.getSize();
            rprintf( "HTTP response size: %d\n", bodylen );

            DeserializationError err = deserializeJson(root, http.getString() );
            if( err == DeserializationError::Ok  && root.containsKey("result") )
            {
                JsonObject result = root["result"][0];
                if( result.containsKey("Counter") &&
                  (todayvalue == NULL || result.containsKey("CounterToday") )
                )
                {
                    if(  value != NULL )
                    {
                        *value = result["Counter"].as<float>();
                    }
                    if( todayvalue != NULL )
                    {
                        *todayvalue = result["CounterToday"].as<float>();
                        rprintf("Today value: %f\n" , result["CounterToday"].as<float>());
                    }
                    return true;
                }
                else
                {
                    rprintf("Invalid result JSON!\n");
                }
            }
            else
            {
                rprintf("JSON parsing error!\n");
            }
        }
        else
        {
            rprintf("Error on HTTP request\n");
        }
         //Free the resources
        http.end();
        ledblink();
        // Feed the watchdog
        utilsloop();
    }
    else
    {
        rprintf("Wifi connection error!\n");
    }

    return false;
}


unsigned int getdomoticz_selector( const char * domo ,  int idx , int * value )
{
DynamicJsonDocument root(1700);
HTTPClient http;
char url[256];

    if ((WiFi.status() != WL_CONNECTED))
    {
        WifiConnect();
    }

    if ((WiFi.status() == WL_CONNECTED))
    {
        ledblink();
        // Request format
        // http://192.168.0.200:9180/json.htm?type=devices&rid=666
        sprintf( url , "http://%s/json.htm?type=devices&rid=%d" , domo , idx );
        rprintf( "%s\n" , url );

        // ESP32 use seconds
        http.setTimeout(2);
        http.begin(url); //Specify the URL
        // Start HTTP request
        int httpCode = http.GET();

        if (httpCode > 0 )
        {
            int bodylen = http.getSize();
            rprintf( "HTTP response size: %d\n", bodylen );

            DeserializationError err = deserializeJson(root, http.getString() );
            if( err == DeserializationError::Ok  && root.containsKey("result") )
            {
                JsonObject result = root["result"][0];
                if( result.containsKey("Level") )
                {
                    *value = result["Level"].as<int>();
                    return true;
                }
                else
                {
                    rprintf("Invalid result JSON!\n");
                }
            }
            else
            {
              rprintf("JSON parsing error!\n");
            }
        }
        else
        {
            rprintf("Error on HTTP request\n");
        }

        //Free the resources
        http.end();
        ledblink();
        // Feed the watchdog
        utilsloop();
    }
    else
    {
        rprintf("Wifi connection error!\n");
    }

    return false;
}
