#ifndef _DOMOTICZ_H
#define _DOMOTICZ_H
//------------------------------------------------------------------------------
// Function prototypes

void updatedomoticz_svalue( const char * domo , int idx , int nvalue , const char * svalue , int battery = -1 );
void updatedomoticz_temp( const char * domo ,  int idx , float temp , float hum = -1 );
unsigned int getdomoticz_temp( const char * domo ,  int idx , float &temp , float &humidity );
unsigned int getdomoticz_temp( const char * domo ,  int idx , float &temp );
unsigned int getdomoticz_counter( const char * domo ,  int idx , float * value , float * todayvalue = NULL );

//------------------------------------------------------------------------------

#define MASTER_DOMOTICZ "192.168.0.200:9180"

//------------------------------------------------------------------------------

#include <domoticz.ino>

#endif // _DOMOTICZ_H
