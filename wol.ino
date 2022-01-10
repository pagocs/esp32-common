#ifndef _WOLINO
#define _WOLINO
//------------------------------------------------------------------------------
// WOL module
// Based on: https://github.com/koen-github/WakeOnLan-ESP8266
//
//------------------------------------------------------------------------------

// Source: https://stackoverflow.com/questions/35227449/convert-ip-or-mac-address-from-string-to-byte-array-arduino-or-c

bool parseBytes(const char* str, char sep, byte* bytes, int maxBytes, int base)
{
int i;

    Serial.print( "Convert: ");
    for ( i = 0; i < maxBytes; i++)
    {
        bytes[i] = strtoul(str, NULL, base);  // Convert byte
        Serial.printf( "%d " , (int)bytes[i] );
        str = strchr(str, sep);               // Find next separator

        if (str == NULL || *str == '\0')
        {
            break;                            // No more separators, exit
        }
        str++;                                // Point to next character after separator
    }
    Serial.println();
    return i == maxBytes-1 ? true :  false;
}

//------------------------------------------------------------------------------

WOL::WOL( void )
{
    udp.begin( port );
}

void WOL::wol( String  mac )
{
    IPAddress ipaddr( 255 , 255 , 255 , 255 );
    byte    macaddr[6];

    if( parseBytes( mac.c_str() , ':' , macaddr , 6 , 16 ) == false )
    {
        Serial.printf( "Invalid MAC address: %s\n" , mac.c_str());
        return;
    }

    byte preamble[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    int i,j;

    // Send the magic packet
    for( j = 0 ; j < 3 ; j++ )
    {
        udp.beginPacket( ipaddr , port );
        udp.write(preamble, sizeof preamble);

        for (i = 0; i < 16; i++)
    	{
            udp.write(macaddr, sizeof(macaddr) );
    	}
        udp.endPacket();
    }
}
#endif //_WOLINO
