//------------------------------------------------------------------------------
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
// #include <sstream>
#include <utils.h>
#include <blescanner.h>
#include <blebeacon.h>


class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        Serial.printf("Advertised Device: %s \n", advertisedDevice.toString().c_str());
        // rprintf("Advertised Device: %s \n", advertisedDevice.toString().c_str());

      if( advertisedDevice.haveRSSI() )
      {
          StaticJsonBuffer<1024> jsonBuffer;
          String topic;
          String payload;
          //
          topic = String(MQTT_BLESCANTOPIC) + String('/') + esp32devicename + String('/');
          JsonObject& root = jsonBuffer.createObject();
          String name;
          if( advertisedDevice.haveName() )
          {
              name = advertisedDevice.getName().c_str();
              root["name"] = name;
          }
          root["addr"] = String(advertisedDevice.getAddress().toString().c_str());
          root["rssi"] = advertisedDevice.getRSSI();


          if (advertisedDevice.haveTXPower())
          {
               root["txpower"] = advertisedDevice.getTXPower();
              // ss << ",\"TxPower\":" << (int)d.getTXPower();
          }

          if (advertisedDevice.haveAppearance())
          {
              root["appearance"] = advertisedDevice.getAppearance();
              // ss     << ",\"Appearance\":" << d.getAppearance();
          }

          String manufacturerdata;
          if (advertisedDevice.haveManufacturerData())
          {
              std::string md = advertisedDevice.getManufacturerData();
              uint8_t* mdp = (uint8_t*)advertisedDevice.getManufacturerData().data();
              char *pHex = BLEUtils::buildHexData(nullptr, mdp, md.length());
              // ss << ",\"ManufacturerData\":\"" << pHex << "\"";
              manufacturerdata = String(pHex);
              root["manufacturerdata"] = manufacturerdata;
              free(pHex);
          }

          String uuid;
          if (advertisedDevice.haveServiceUUID())
          {
              uuid = String(advertisedDevice.getServiceUUID().toString().c_str());
              root["uuid"] = uuid;
              // ss << ",\"ServiceUUID\":\"" << d.getServiceUUID().toString() << "\"" ;
          }

          // It should be decode the device informtion, but currently i did not find solution for
          // apple devices...
          // This is notworking....
          // BLEBeacon bacon;
          // bacon.setData(advertisedDevice.getManufacturerData().data());
          // rprintf( "Manufacturer: %d major: %d minor: %d\n" , (int)bacon.getManufacturerId() , (int)bacon.getMajor() , (int)bacon.getMinor() );

          root.printTo( payload );
          // payload.replace( "\\r\\n" , "" );
          MQTTPublish( topic , payload , 1 );
          //     rprintf( "MQTTbase::publish::topic: %s payload: %s\n" , topic.c_str() ,  payload.c_str());
      }

      ledblink();
    }
};

// BLEScan* pBLEScan;

// BLEScanner::BLEScanner( void )
// {
//     init( 0 );
// }


BLEScanner::BLEScanner( int rescan , int period )
// {
//     init( rescan , period );
// }
//
// // void BLE_ScanInit( )
// void BLEScanner::init( int rescan , int period )
{

    setperiod( rescan , period );
    rprintf("Init BLE Scanning...(on core:%d)\n",xPortGetCoreID());
    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan(); //create new scan
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster

    enable();

    // xTaskCreate(
    //     ble_scanthread,         /* pvTaskCode */
    //     "BLEScan",              /* pcName */
    //     8000,                   /* usStackDepth */
    //     this,                   /* pvParameters */
    //     1,                      /* uxPriority */
    //     &BLEscan                /* pxCreatedTask */
    //     );

}

void BLEScanner::setperiod( int rescan , int period )
{
    scanperiod = (period == 0 ) ? 5 : period;
    rescanwait = (rescan == 0 ) ? 5*1000 : rescan*1000;
}

void BLEScanner::disable( void )
{
    if( BLEscan != NULL )
    {
        vTaskDelete( BLEscan );
    }
}

void BLEScanner::enable( void )
{
    xTaskCreate(
        ble_scanthread,         /* pvTaskCode */
        "BLEScan",              /* pcName */
        5000,                   /* usStackDepth */
        this,                   /* pvParameters */
        1,                      /* uxPriority */
        &BLEscan                /* pxCreatedTask */
        );

}

int BLEScanner::getperiod( void )
{
    return( scanperiod );
}

int BLEScanner::getrescanwait( void )
{
    return( rescanwait );
}


BLEScan* BLEScanner::getscanobject( void )
{
    return( pBLEScan );
}

// WRANING: This is just a prototype and does not work currently
bool connectToserver(BLEAdvertisedDevice *device)
{

    return false;

    BLEClient*  pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");

    // Connect to the BLE Server.
    if( pClient->connect(device) )
    {
        Serial.println(" - Connected to BLE device");

        // Obtain a reference to the service we are after in the remote BLE server.
        // BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
        // if (pRemoteService != nullptr)
        // {
        //   Serial.println(" - Found our service");
        //   return true;
        // }
        // else
        // return false;
        //
        // // Obtain a reference to the characteristic in the service of the remote BLE server.
        // pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
        // if (pRemoteCharacteristic != nullptr)
        //   Serial.println(" - Found our characteristic");
        //
        //   return true;

        pClient->disconnect();
    }
    else
    {
        Serial.println(" - Connect FAILD!");
    }

}

struct bledevices {
    String macaddr;
    String name;
};

struct bledevices knowndevices[] = {
        // { "c0:a6:00:60:fc:6b" , "Pagocs iPhone"},
        // { "38:F9:D3:61:FA:F4" , "Peter’s Mac mini"},
        { "ff:ff:c3:0e:5e:55" , "iTag 01" },
        { "a0:e6:f8:6c:43:33" , "BeeNode3" },
        { "ac:bc:32:6a:ab:a3" , "Apple TV Living room" }
        // { "" , "" },
        // 38:C7:46:8E:23:BC
    };

const char * getname( BLEAdvertisedDevice &device )
{
    int i;

    String  mac;

    mac = String( device.getAddress().toString().c_str() );
    mac.toLowerCase();

    // rprintf( "Device address: %s\n" , mac.c_str());

    for( i = 0 ; i < sizeof( knowndevices ) / sizeof(struct bledevices) ; i++ )
    {
        String knownmac = knowndevices[i].macaddr;
        knownmac.toLowerCase();
        if( mac == knownmac )
        {
            return knowndevices[i].name.c_str();
        }
    }

    return NULL;
}

// void BLE_Scan( unsigned int scanTime = 5 )
void ble_scan( BLEScanner* params  )
{
String  device;

    rprintf( "%s: BLE Scanning started (on core:%d)...\n" , timeClient.getFormattedTime().c_str(), xPortGetCoreID() );
    BLEScanResults foundDevices = params->getscanobject()->start(params->getperiod());
    rprintf( "%s: Devices found: %d\n" , timeClient.getFormattedTime().c_str() , foundDevices.getCount() );
    rprintf( "%s: Scan done!\n" , timeClient.getFormattedTime().c_str() );

    // std::stringstream ss;

    // ss << "[" ;
    for (int i = 0; i < foundDevices.getCount(); i++)
    {
        // if (i > 0) {
        //     ss << ",";
        // }
        BLEAdvertisedDevice d = foundDevices.getDevice(i);
        // ss << "{\"Address\":\"" << d.getAddress().toString() << "\",\"Rssi\":" << d.getRSSI();

        if (d.haveName())
        {
            // ss << ",\"Name\":\"" << d.getName() << "\"";
        }

        if (d.haveTXPower())
        {
            // ss << ",\"TxPower\":" << (int)d.getTXPower();
        }

        if (d.haveAppearance())
        {
            // ss     << ",\"Appearance\":" << d.getAppearance();
        }

        if (d.haveManufacturerData())
        {
            // std::string md = d.getManufacturerData();
            // uint8_t* mdp = (uint8_t*)d.getManufacturerData().data();
            // char *pHex = BLEUtils::buildHexData(nullptr, mdp, md.length());
            // ss << ",\"ManufacturerData\":\"" << pHex << "\"";
            // free(pHex);
        }

        if (d.haveServiceUUID())
        {
            // ss << ",\"ServiceUUID\":\"" << d.getServiceUUID().toString() << "\"" ;
        }

        // ss << "}";
        // ss << "\n";
        const char *name = getname( d );
        rprintf( "Address: %s (%s) RSSI %d %s\n" , d.getAddress().toString().c_str() ,
            (name != NULL) ? name : (d.haveName())? d.getName().c_str() : "N/A" ,
            d.getRSSI(),
            d.haveServiceUUID() ? d.getServiceUUID().toString().c_str() : ""
        );

        // connectToserver( &d );

    }
    // ss << "]";

    (params->getscanobject())->clearResults();

  // Serial.println(ss.str().c_str());
  // rprintf( "%s\n" , ss.str().c_str() );

}

// void BLE_GetParameters()
// {
//
// }


// void BLE_scanthread( void * params )
void ble_scanthread( void * params )
{

    rprintf("%s: BLE scan running on core: %d\n" , timeClient.getFormattedTime().c_str() , xPortGetCoreID() );
    rprintf("%s: Initial delay:" , timeClient.getFormattedTime().c_str() );
    int i;
    for( i = 0 ; i < 15 ; i++ )
    {
        delay( 1000 );
        rprintf("." );
    }

    rprintf("\n" );

    // BLE_ScanInit( );
    while( true )
    {
        // FIXME: Is this a good idea to start stop BLE always?
        // btStart();
        ble_scan( static_cast<BLEScanner*>(params) );
        // btStop();
        delay( static_cast<BLEScanner*>(params)->getrescanwait() );
    }
}
