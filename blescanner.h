#ifndef _BLESCANNER_H
#define _BLESCANNER_H

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#define MQTT_BLESCANTOPIC   "blescan"

class BLEScanner {
    protected:
        int     scanperiod = 5;
        int     rescanwait = 5000;
        BLEScan* pBLEScan;
        TaskHandle_t    BLEscan=NULL;
    public:

        // BLEScanner( void );
        BLEScanner( int rescan = 0 , int period = 0 );
        void setperiod( int rescan = 0 , int period = 0 );
        void disable( void );
        void enable( void );
        int getperiod( void );
        int getrescanwait( void );
        BLEScan* getscanobject( void );
        const char * getname( BLEAdvertisedDevice & );
};

void ble_scanthread( void * params );
void ble_scan( BLEScanner* params );

#include <blescanner.ino>

#endif //_BLESCANNER_H
