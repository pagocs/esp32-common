#ifndef _WIFICONNECT_H
#define _WIFICONNECT_H

void WifiConnect( const char* ssid = NULL , const char* psw = NULL );
bool iswificonnected( void );

#include <wificonnect.ino>

#endif //_WIFICONNECT_H
