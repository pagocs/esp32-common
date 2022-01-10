#ifndef _OTA_H
#define _OTA_H

//------------------------------------------------------------------------------
// Initialisie the OTA functions

typedef void (*voidfunc_t)();

//------------------------------------------------------------------------------
void OTAinit( int port = 0 , const char *hostname = NULL , const char *password = NULL , voidfunc_t osntart = NULL );
void OTAHandle( void );

// FIXME:
#include <ota.ino>

#ifdef _OTA_DEBUG_PRINTF
#undef DEBUG_PRINTF
#endif //_OTA_DEBUG_PRINTF

#endif // _OTA_H
