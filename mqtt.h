#ifndef _MQTT_H
#define _MQTT_H

//------------------------------------------------------------------------------
// MQTT

#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <utils.h>

// My apps is required minimum MQTT buffer size defined here. Ensure this
// will be available because the lower buffersize can cause loss
// in reflink packets

#define MQTT_MYPOCKETSIZE 512

// The value is set in the _MQTTConnect unction further
// #if (MQTT_MAX_PACKET_SIZE < MQTT_MYPOCKETSIZE)
// #error "MQTT_MAX_PACKET_SIZE IS TOO SMALL! Please chenge the value in PubSubClient.h file!"
// #endif

#define MQTT_MYSOCKETTIMEOUT 5

// #if (MQTT_SOCKET_TIMEOUT > MQTT_MYSOCKETTIMEOUT)
// #error "MQTT_SOCKET_TIMEOUT IS TOO HIGH! Please chenge the value in PubSubClient.h file!"
// #endif

//------------------------------------------------------------------------------
//typedef void (* MQTTCallback )( char* topic, byte* payload, unsigned int length );
typedef void (*MQTTCallback)( char * , uint8_t * , unsigned int );

//------------------------------------------------------------------------------
// Debug
// #ifndef DEBUG_PRINTF
// #define DEBUG_PRINTF(f_, ...) Serial.printf((f_), ##__VA_ARGS__)
// #endif // DEBUG_PRINTF

//------------------------------------------------------------------------------
// Callback

struct MQTTcallbackitem {
    bool            alltopics;
    MQTTCallback    func;
    const char *    topic;
};

//------------------------------------------------------------------------------

#define TEST_TOPIC    "test"
#define RFLINK_RECEIVE_TOPIC    "rflink/in"
// MQTT controller ttopics
#define MQTT_BASETOPIC "myesp32"
#define MQTT_CONTROLLERCOMMANDS "controllers"

void _MQTTCallback( char* , uint8_t * , unsigned int );

// TODO rewrite this to dynamic allocations
#define MQTT_MAXBROKERS 5

//------------------------------------------------------------------------------
// TODO:
// Obsolote ezeket fel kell számolni!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// Dev Class
#define MQTT_ILUMINANCE "iluminance"
#define MQTT_TEMPHUMBARO "temp+hum+baro"

#define MQTT_SENSOR "sensor"
#define MQTT_LUX "lux"
#define MQTT_MOTION "motion"
#define MQTT_GPIOIN "GPIOin"
#define MQTT_RELAY "relay"
#define MQTT_SENSORTOPIC "sensors"
// #define MQTT_TEMPHUMBARO "temp hum baro"
#define MQTT_TEMPBARO "temp+baro"
#define MQTT_TEMP "temp"

#define MQTT_ENVIROMENT "environment"
#define MQTT_UTILITY "utility"
#define MQTT_TEMPERATURE "temperature"
#define MQTT_WEATHER "weather"
#define MQTT_SECURITY "security"
#define MQTT_SWITCH "switch"
#define MQTT_METER "meter"
#define MQTT_GENERIC "generic"

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


class MQTTbase {

    public:
        void publish( const char * basetopic , const char * devclass , const char * device , const char * name , const char * type , JsonObject values );
};

//------------------------------------------------------------------------------

void MQTTquerybrokers( bool forcerescan = false );
void MQTTsetserver( const char * server = NULL , int port = 0 );
void MQTTsetserver( bool );
void MQTTConnect( const char * server , int port , const char * node , const char * user = NULL , const char * psw = NULL );
void MQTTConnect( const char * node , const char * user = NULL , const char * psw = NULL );
void MQTTPublish( const char * , const char * , time_t delay = 0 , bool retain = false );
void MQTTPublish( String , String , time_t delay = 0 );
void MQTTPublish( String , String , bool );
bool MQTTtopicmatch( const char * , const char * );
// Subscribe to controller topic
void MQTTSubscribe( MQTTCallback );
// Subscribe callback function to topic
// It ccould be use # for get all topics
// Or specific topic: in this case the different callbacks
// is called just when that specific topic (or partially match from the begining)
// is received.
void MQTTSubscribe( const char * , MQTTCallback );
// Call this function in main loop
void MQTTLoop( void );
void MQTTend( void );

#include <mqtt.ino>

#endif //_MQTT_H
