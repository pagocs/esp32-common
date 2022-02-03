#ifndef _WOL_H
#define _WOL_H_H

#include <WiFiUdp.h>

//------------------------------------------------------------------------------

class WOL {
    WiFiUDP udp;
    int port = 9;
public:
    WOL( void );
    ~WOL( ) {};
    void wol( String  mac  );

};

// FIXME:
#include <wol.ino>

#endif //_WOL_H
