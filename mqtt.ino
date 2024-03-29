#include <Esp.h>
#include <WiFi.h>
#include <string.h>
#include <math.h>
#include <ArduinoJson.h>
#include <vector>
#include <time.h>

// Resources
// ArduinoJson
// https://arduinojson.org/v5/assistant/

//------------------------------------------------------------------------------
// MQTT
// #include <PubSubClient.h>

#include <utils.h>
#include <mqtt.h>

//------------------------------------------------------------------------------

WiFiClient espClient;
PubSubClient client(espClient);
bool MQTTinited = false;
int numofMQTTcallbackitems = 0;
std::vector<MQTTcallbackitem *> MQTTcallbacks;
const char *MQTTnode = NULL;
const char *MQTTuser = NULL;
const char *MQTTpassword = NULL;
//bool MQTTinited = false;

String MQTTserver;
unsigned long MQTTserverport = 0;


TaskHandle_t pxMQTTloopthread = NULL;
SemaphoreHandle_t mutex_mqttpublish = NULL;
SemaphoreHandle_t mutex_mqttqueue = NULL;

// Custom controller command handling
MQTTCallback MQTTcontrollertopiccallback = NULL;

// TODO rewrite this to dynamic allocations
#define MQTT_MAXBROKERS 5

struct  MDNShost *MQTTbrokers[MQTT_MAXBROKERS];
bool    MQTTusemdns = false;
int     MQTTnumofbrokers = 0;
int     MQTTcurrentbroker = 0;
int     MQTTconnectionfail = 0;
int     MQTTpublishfail = 0;

//------------------------------------------------------------------------------
// Private ffunctions

void _MQTTloopinit( void );

//------------------------------------------------------------------------------
#include <Preferences.h>
Preferences MQTTprefs;

void deletebrokers( void )
{
    // delete actual brokers
    if( MQTTnumofbrokers )
    {
        for( int i = 0; i < MQTTnumofbrokers ; i++)
        {
            delete MQTTbrokers[i];
            MQTTbrokers[i] = NULL;
        }
        MQTTnumofbrokers = 0;
    }
}

// IP shorting ascending
int _hostcompare(const void *val1, const void *val2)
{
  IPAddress a = (*(struct MDNShost**)val1)->ip;
  IPAddress b = (*(struct MDNShost**)val2)->ip;;
  DEBUG_PRINTF( "Compare: %u.%u.%u.%u ? %u.%u.%u.%u Result: %d\n" , a[0],a[1],a[2],a[3],b[0],b[1],b[2],b[3], a > b ? -1 : (a < b ? 1 : 0) );
  return a < b ? -1 : (a > b ? 1 : 0);
}

void MQTTdeletestoredbrokers( void )
{
Preferences prefs;

    if( prefs.begin("MQTTsettings", false) )
    {
        rprintf( "-- Delete stored brokers --\n" );
        prefs.remove( "Brokers" );
        prefs.end();
        rprintf( "-- Delete success --\n" );
    }
}

void MQTTquerybrokers( bool forcerescan )
{
JsonDocument    array;

bool prefs;
int i,brokers;

    // FIXME
    // Currently there is an bug in the SDK about the MDNS.queryService which is cause
    // the OTA mailfunction
    // Because of this temporaly ti should store the brokers in the falsh storage
    // When new query is take place it should be reset the device

    prefs = MQTTprefs.begin("MQTTsettings", false);
    // There is saved brokers
    if( !forcerescan && prefs )
    {
        char jsonbuff[384];
        deletebrokers();

        if( MQTTprefs.getBytes("Brokers", jsonbuff , sizeof(jsonbuff)) )
        {
            DeserializationError err = deserializeJson(array,jsonbuff);
            if( err == DeserializationError::Ok && array.size() > 0 )
            {
                rprintf( "Num of stored brokers: %d\n",array.size());
                // for (auto value : arr) {
                //     Serial.println(value.as<char*>());
                // }
                MQTTnumofbrokers = array.size();

                for( i = 0 ; i < MQTTnumofbrokers ; i++ )
                {
                    rprintf( "Add broker: host: %s ip: %s:%u \n" ,
                        array[i]["hostname"].as<const char *>(),
                        array[i]["ip"].as<const char *>(),
                        array[i]["port"].as<unsigned int>()
                    );
                    MQTTbrokers[i] = new struct MDNShost;
                    MQTTbrokers[i]->hostname = array[i]["hostname"].as<String>();
                    IPAddress ip;
                    ip.fromString( array[i]["ip"].as<const char *>() );
                    MQTTbrokers[i]->ip = ip;
                    MQTTbrokers[i]->port = array[i]["port"].as<unsigned int>();
                }
                MQTTcurrentbroker = 0;
                MQTTprefs.end();

                // Sorting the hosts by IP
                qsort( MQTTbrokers , i , sizeof(struct MDNShost *), _hostcompare );
                rprintf( "-- Sorted brokers --\n" );

                // Print the shorted list
                for( i = 0 ; i < MQTTnumofbrokers ; i++ )
                {
                    rprintf( "Broker: host: %s ip: %u.%u.%u.%u:%u \n" ,
                        MQTTbrokers[i]->hostname.c_str(),
                        MQTTbrokers[i]->ip[0],MQTTbrokers[i]->ip[1],MQTTbrokers[i]->ip[2],MQTTbrokers[i]->ip[3],
                        MQTTbrokers[i]->port
                    );
                }

                return;
            }
            else
            {
                rprintf( "!!! ERROR: Unable to parse saved JSON!\n");
            }
        }
        else
        {
            rprintf( "!!! ERROR: Unable to restore broker settings!\n");
        }
    }
    array.clear();
    brokers = MDNSbrowseservice("mqtt", "tcp");
    if( brokers )
    {
        // DEBUG_PRINTF( "Use new brokers...\n" );
        rprintf( "Use new brokers...\n" );

        // Call restart preparation if it is registered
        prerestart( );

        deletebrokers();
        //MQTTprefs.clear();
        MQTTprefs.remove( "Brokers" );
        rprintf( "Old brokers deleted...\n" );

        //----------------------------------------------------------------------
        rprintf( "Store brokers...\n" );
        MQTTnumofbrokers = brokers;
        for( int i = 0; i < brokers ; i++)
        {
            MQTTbrokers[i] = MDNSgethost( i );
            // Store in prefs
            if( prefs )
            {
                JsonObject nested = array.add<JsonObject>();
                nested["hostname"] = MQTTbrokers[i]->hostname;
                rprintf( "IP: %s\n", MQTTbrokers[i]->ip.toString().c_str());
                nested["ip"] = (char *)MQTTbrokers[i]->ip.toString().c_str();
                nested["port"] = MQTTbrokers[i]->port;
            }
        }
        if( prefs )
        {
            // Save MQTT brokers to prefs
            String jsonbuff;
            serializeJsonPretty( array , jsonbuff );
            
            rprintf( "Brokers JSON: %s\n" , jsonbuff.c_str());
            // DEBUG_PRINTF( "Brokers JSON: %s\n" , jsonbuff.c_str());
            rprintf( "Store presets...\n" );
            if( MQTTprefs.putBytes("Brokers", jsonbuff.c_str() , jsonbuff.length()) )
            {
                // Saved succesfully
                // DEBUG_PRINTF( "MQTT prefs saved successfully!\n");
                rprintf( "MQTT prefs saved successfully!\n");
                MQTTprefs.end();
            }
            else
            {
                // Prefs save error
                rprintf( "!!! ERROR: Unable to save broker list!\n");
            }
            // MQTTprefs.end();
        }
        MQTTcurrentbroker = 0;
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // FIXME reset ESP
        // DEBUG_PRINTF( "Restart ESP32 because MDNS and OTA incompatibility issue...\n");

        // MQTTend( );
        rprintf( "Restart ESP32 because MDNS and OTA incompatibility issue...\n");
        restart();
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    }

    MQTTprefs.end();

}

void MQTTsetserver( const char * server , int port )
{
    if( server != NULL )
    {
        MQTTserver = String( server );
        MQTTserverport = port;
        client.setServer( server , port );
        MQTTnumofbrokers = 1;
        rprintf("%s: MQTTSetServer::server: %s:%d fails: %d\n" , timeClient.getFormattedTime().c_str() , server , port , MQTTconnectionfail );
    }
    else if( MQTTusemdns == true )
    {
        if( !MQTTnumofbrokers || MQTTconnectionfail >= MQTTnumofbrokers )
        {
            if( !MQTTnumofbrokers || (MQTTconnectionfail%MQTTnumofbrokers) == 0 )
            {
                rprintf( "Query MDNS brokers...\n");
                MQTTquerybrokers(true);
            }
        }
        if( MQTTnumofbrokers )
        {
            rprintf( ">>> MQTT broker: %s %s:%d\n",MQTTbrokers[MQTTcurrentbroker]->hostname.c_str(),
                MQTTbrokers[MQTTcurrentbroker]->ip.toString().c_str() , MQTTbrokers[MQTTcurrentbroker]->port);
            client.setServer( MQTTbrokers[MQTTcurrentbroker]->ip , MQTTbrokers[MQTTcurrentbroker]->port );
            MQTTserver = String( MQTTbrokers[MQTTcurrentbroker]->ip[0] ) + "." +
                            String( MQTTbrokers[MQTTcurrentbroker]->ip[1] ) + "." +
                            String( MQTTbrokers[MQTTcurrentbroker]->ip[2] ) + "." +
                            String( MQTTbrokers[MQTTcurrentbroker]->ip[3] );
            MQTTserverport = MQTTbrokers[MQTTcurrentbroker]->port;
        }
        else
        {
            rprintf( "!!! ERROR: No MQTT broker found!\n");
        }
    }
    else
    {
        rprintf( "!!! ERROR: No MQTT server specified!\n");
    }

    if( MQTTconnectionfail && MQTTconnectionfail >= MQTTnumofbrokers*2 )
    {
        // Reset the device
        rprintf( ">>> Too many connection fail! Reboot...\n" );
        // delete stored brokers
        // Cannot use prefs functions her because is cause crash
        // 21:42:40.108 -> -- Delete stored brokers --
        // 21:42:40.108 -> Guru Meditation Error: Core  1 panic'ed (Cache disabled but cached memory region accessed)
        // 21:42:40.108 -> Guru Meditation Error: Core  1 panic'ed (IllegalInstruction). Exception was unhandled.
        // 21:42:40.108 -> Memory dump at 0x40159f5c: bad00bad bad00bad bad00bad
        // MQTTdeletestoredbrokers();
        restart();
    }
}

void MQTTsetserver( bool brokershift )
{
    if( MQTTusemdns == true )
    {
        if( brokershift == true && MQTTnumofbrokers > 1 )
        {
            MQTTcurrentbroker =
                (MQTTcurrentbroker+1) >= MQTTnumofbrokers ?
                0 : MQTTcurrentbroker+1;
            rprintf( "MQTT broker shift: %s\n",MQTTbrokers[MQTTcurrentbroker]->hostname.c_str());
        }
        MQTTsetserver();
    }
    else
    {
        rprintf("%s: Broker shift on singleserver connection...\n" , timeClient.getFormattedTime().c_str() );
        // Single server connect
        MQTTsetserver( MQTTserver.c_str() , MQTTserverport );
    }
}

#ifndef MQTTLEDPIN
#define MQTTLEDPIN 12
#endif

int MQTTledstate = 0;

//------------------------------------------------------------------------------

// void MQTTConnect( const char * server = NULL , int port = 0 , const char * node = NULL , const char * user = NULL , const char * psw = NULL )
bool _MQTTConnect( const char * node = NULL , const char * user = NULL , const char * psw = NULL )
{
int     i = 0;
bool    connected = false;

    //--------------------------------------------------------------------------
    //  Increase watchdog timer

    setwatchdogtimeout( 60 );

    //--------------------------------------------------------------------------
    // Change the default parameters if it is necesarry

    if( client.getBufferSize() < MQTT_MYPOCKETSIZE )
    {
        client.setBufferSize( MQTT_MYPOCKETSIZE );
    }

    client.setSocketTimeout( MQTT_MYSOCKETTIMEOUT );

    //--------------------------------------------------------------------------

    // rprintf("--- Connection timeot: %d\n" , MQTT_SOCKET_TIMEOUT );
#ifdef MQTTLEDDEBUG
    MQTTledstate = 0;
#endif
    while( !client.connected() && i < 2 )
    //while ( client.state() != MQTT_CONNECTED && i < 5 )
    {
#ifdef MQTTLEDDEBUG
        MQTTledstate = MQTTledstate ? 0 : 1;
        digitalWrite(MQTTLEDPIN, MQTTledstate);
#endif
        rprintf("--- Connecting to MQTT: ");
        if ( client.connect( MQTTnode , MQTTuser, MQTTpassword ) )
        {
            MQTTconnectionfail = 0;
            rprintf("connected\n");

            //if( MQTTinited == false )
            {
                // FIXME
                // subscribe the controller topic
                //MQTTinited = true;
                String topic = MQTT_CONTROLLERCOMMANDS;
                client.setCallback(_MQTTCallback);
                // client.subscribe( topic.c_str() );
                // rprintf("--- Subscribe to topic: %s\n" , topic.c_str() );
                topic += (String)"/#";
                client.subscribe( topic.c_str() );
                rprintf("--- Subscribe to topic: %s\n" , topic.c_str() );
            }

            if( numofMQTTcallbackitems )
            {
                client.setCallback(_MQTTCallback);
                std::vector<MQTTcallbackitem *>::iterator it = MQTTcallbacks.begin();
                
                while (it != MQTTcallbacks.end())
                {
                    rprintf("--- Subscribe to topic: %s\n" , (*it)->topic );
                    client.subscribe( (*it)->topic );    
                    it++;                
                }
            }
            connected = true;
            MQTTinited = true;
        }
        else
        {
            rprintf("failed with state: %d\n",client.state());
            //Serial.println(client.state());
            // delay(100);
            delay(100);
        }
        i++;
        utilsloop();
    }
    setwatchdogtimeout();

    if( !client.connected() || connected == false )
    {
        MQTTconnectionfail++;
        MQTTsetserver( true );
    }

#ifdef MQTTLEDDEBUG
    MQTTledstate = 0;
    digitalWrite(MQTTLEDPIN, MQTTledstate);
#endif

    return connected;

}

void MQTTConnect( const char * server , int port , const char * node , const char * user , const char * psw )
{
    MQTTsetserver( server , port );
    MQTTnode = node;
    MQTTuser = user;
    MQTTpassword = psw;
    _MQTTConnect();
    _MQTTloopinit();
}

void MQTTConnect( const char * node , const char * user , const char * psw )
{
    rprintf("MQTT Use MDNS...\n");
    MQTTusemdns = true;
    MQTTnode = node;
    MQTTuser = user;
    MQTTpassword = psw;
    MDNSinit();
    MQTTquerybrokers();
    MQTTsetserver();
    _MQTTConnect();
    _MQTTloopinit();
}

//------------------------------------------------------------------------------

bool _MQTTPublish( const char * topic , const char * msg , bool retain )
{
int obtained = pdTRUE;
bool success = false;

    if( !client.connected() || MQTTpublishfail > 2 || client.state() != MQTT_CONNECTED )
    //if( client.state() != MQTT_CONNECTED )
    {
        // int i=0;
        // do {
        //     _MQTTConnect();
        // } while( ++i < MQTTnumofbrokers && client.state() != MQTT_CONNECTED );
        return false;
    }

    if( client.state() == MQTT_CONNECTED )
    {
        if( mutex_mqttpublish != NULL )
        {
            if( xSemaphoreTakeRecursive( mutex_mqttpublish, portMAX_DELAY ) != pdTRUE )
            {
                return false;
            }
        }

        if( client.publish( topic , msg , retain ) != false )
        {
            // rprintf("%d: >>> Publish MQTT topic in: \"%s\" Message: \"%s\"\n", millis() , topic , msg );
            rprintf("%s: >>> Publish MQTT topic in: \"%s\" Message: \"%s\" %s\n", timeClient.getFormattedTime().c_str() , topic , msg , retain ? "#Retain: true":"");
            MQTTpublishfail = 0;
            success = true;
        }
        else
        {
            // rprintf("%d: !!! ERROR: Publish MQTT topic in: \"%s\" Message: \"%s\" FAILED!\n", millis() , topic , msg );
            rprintf("%s: !!! ERROR: Publish MQTT topic in: \"%s\" Message: \"%s\" FAILED!\n", timeClient.getFormattedTime().c_str() , topic , msg );
            rprintf( "Maximum size: %d current: %d\n" , MQTT_MAX_PACKET_SIZE , strlen(topic) + strlen( msg ) );
            MQTTpublishfail++;
        }

        if( mutex_mqttpublish != NULL )
        {
            xSemaphoreGiveRecursive( mutex_mqttpublish );
        }
    }
    else
    {
        rprintf("MQTT is not connected (state: %d).\n", client.state() );
        MQTTpublishfail++;
    }

    return success;

}

//------------------------------------------------------------------------------
// MQTT queue

class MQTTqueueitem {

public:

    time_t  timing;   // Send timing
    String  topic;
    String  payload;
    bool    retain;

    MQTTqueueitem( const char * msgtopic , const char * msg , time_t delay = 0 , bool retain = false )
    {
        timing = (delay == 0) ? 0 : time( NULL ) + delay;
        // rprintf( "QUEUEitem: time:%d delay: %d timing: %d\n" , time( NULL ) , delay , timing );
        topic = String(msgtopic);
        payload = String( msg );
    };

};

//------------------------------------------------------------------------------

std::vector<MQTTqueueitem*> MQTTqueue;


void MQTTPublish( const char * topic , const char * msg , time_t delay , bool retain )
{
bool success = true;

    if( !delay )
    {
        success = _MQTTPublish( topic , msg , retain );
    }

    if( delay || !success )
    {
        // TODO
        //  Create semaphore and catch it before make anytingh with the queue

        rprintf("MQTTPublish::add to queue [%d][%s][%s]\n" , delay , topic , msg );
        String newtopic = String( topic );
        String newmsg = String( msg );

        if( mutex_mqttqueue != NULL )
        {
            if( xSemaphoreTakeRecursive( mutex_mqttqueue, portMAX_DELAY ) != pdTRUE )
            {
                return;
            }
        }

        for (std::vector<MQTTqueueitem*>::iterator it = MQTTqueue.begin(); it != MQTTqueue.end() ; it++ )
        {
            if( (*it)->topic == newtopic && (*it)->payload == newmsg )
            {
                rprintf( "MQTTPublish::add: Same topic found! DELETE! currenttime: %ld timing %ld [%s]::%s\n" , time( NULL ) , (*it)->timing , (*it)->topic.c_str() , (*it)->payload.c_str() );
                delete *it;
                MQTTqueue.erase( it );
                break;
            }
        }

        MQTTqueueitem * message = new MQTTqueueitem( topic , msg , delay , retain );
        // Create delayed topic
        MQTTqueue.push_back( message );

        if( mutex_mqttqueue != NULL )
        {
            xSemaphoreGiveRecursive( mutex_mqttqueue );
        }
    }

}

void MQTTPublish( String topic , String msg , time_t delay )
{
    MQTTPublish( topic.c_str() , msg.c_str() , delay );
}

void MQTTPublish( String topic , String msg , bool retain )
{
    MQTTPublish( topic.c_str() , msg.c_str() , 0 , retain );
}

bool MQTTtopicmatch( const char * topic , const char * matchtopic )
{
    // Last character is #
    int len = std::min( strlen(topic) , strlen(matchtopic)) - 1 ;
    //rprintf( ">>> MQTT matchtopic: [%s]:[%s]=%d\n" , topic , matchtopic,strncasecmp( topic , matchtopic , len ));
    return strncasecmp( topic , matchtopic , len ) == 0 ? true : false;
}

void _MQTTCallback( char* topic, uint8_t * payload, unsigned int length )
{
    bool topicmatch = MQTTtopicmatch( topic , MQTT_CONTROLLERCOMMANDS );
    if( topicmatch || numofMQTTcallbackitems )
    {
        rprintf(  "-->> Topic received: %s\n" , topic );
        char payloadstr[256];
        int i = length > sizeof(payloadstr)-1 ? sizeof(payloadstr)-1: length;
        if( length > sizeof(payloadstr)-1 )
        {
            rprintf( "!!! WARNING: Payload is to long: %d!\n" , length );
        }
        memcpy( payloadstr , payload , i );
        payloadstr[i] = 0;

        if( topicmatch )
        {
            DEBUG_PRINTF( ">>> MQTT Cantroller cmd: [%s]: %s\n" , topic , payloadstr );

            if( !strcmp( topic , MQTT_CONTROLLERCOMMANDS ) ||
                strcasestr( topic , "all" ) != NULL ||
                strcasestr( topic , getMacAddress(true).c_str()) != NULL
            )
            {
                DEBUG_PRINTF( ">>> MQTT controller target is matched.\n" );

                JsonDocument    root;
                DeserializationError err = deserializeJson(root, (const char*)payload);
                if( err == DeserializationError::Ok )
                {
                    if( root.containsKey("command") )
                    {
                        if( root["command"].as<String>() == "debug" )
                        {
                            if( root.containsKey("value") )
                            {
                                DEBUG_PRINTF( "Value: %d\n" , root["value"].as<bool>() );

                                if( root["value"].as<bool>() == true )
                                {
                                    //SYSTEMdebug = true;
                                    setdebugmode( true );
                                    rprintf( "SYSTEMdebug SET to TRUE\n" );
                                }
                                else
                                {
                                    //SYSTEMdebug = false;
                                    setdebugmode( false );
                                    rprintf( "SYSTEMdebug SET to FALSE\n" );
                                }
                            }
                            else
                            {
                                rprintf( "!!! ERROR: [value] is missing!\n" );
                            }



                        }
                        else if( root["command"].as<String>() == "reboot" )
                        {
                            rprintf( ">>> Reboot\n" );
                            // MQTTend( );
                            delay( 250 );
                            restart();
                        }
                        else if( root["command"].as<String>() == "mdnsrescan" )
                        {
                            rprintf( ">>> Rescan mdns mqtt targets...\n" );
                            MQTTquerybrokers(true);
                        }
                        else if( root["command"].as<String>() == "mqttchangeserver" )
                        {
                            rprintf( ">>> Change MQTT broker...\n" );
                            MQTTsetserver(true);
                        }
                        else if( root["command"].as<String>() == "mqttbrokerlist" )
                        {
                            int i;
                            rprintf( ">>> List MQTT brokers: %d ...\n" , MQTTnumofbrokers );
                            for( i = 0 ; i < MQTTnumofbrokers ; i++ )
                            {
                                rprintf( "Broker: host: %s ip: %u.%u.%u.%u:%u \n" ,
                                    MQTTbrokers[i]->hostname.c_str(),
                                    MQTTbrokers[i]->ip[0],MQTTbrokers[i]->ip[1],MQTTbrokers[i]->ip[2],MQTTbrokers[i]->ip[3],
                                    MQTTbrokers[i]->port
                                );
                            }
                        }
                        else if ( MQTTcontrollertopiccallback == NULL )
                        {
                            rprintf( "!!! ERROR: Unhandled command: %s\n" , root["command"].as<const char *>() );
                        }
                    }
                    else
                    {
                        rprintf( "!!! WARNING: No [command] defined! [%s]: %s\n" , topic , payloadstr );
                    }
                    if( MQTTcontrollertopiccallback != NULL )
                    {
                        // (*MQTTcontrollertopiccallback)(topic , payload , length );
                        // WARNING: Us the 0 terminated copy to avoid the buffer
                        // subsequent copy
                        (*MQTTcontrollertopiccallback)(topic , (uint8_t *)payloadstr , length );
                    }

                }
                else
                {
                    rprintf( "!!! ERROR: JSON decode error!\n" );
                }
            }
        }
        else if(  numofMQTTcallbackitems )
        {
            std::vector<MQTTcallbackitem *>::iterator it = MQTTcallbacks.begin();
            while (it != MQTTcallbacks.end())
            {
                if( MQTTtopicmatch( topic , (*it)->topic ) ||
                    (*it)->alltopics == true
                )
                {
                    // rprintf( "--- MQTT call callback\n" );
                    // WARNING: the payloadstr is 0 terminated!
                    (*(*it)->func)( topic , (uint8_t *)payloadstr , length );
                }
                it++;                
            }
        }
    }
}

// Subscribe to controller topic
void MQTTSubscribe( MQTTCallback callback )
{
    MQTTcontrollertopiccallback=callback;
}

void MQTTSubscribe( String topic , MQTTCallback callback )
{
    // If using Stirng then must create a copy

    unsigned int length = topic.length() + 1;
    char * cstr = (char *)malloc( length );
    if( cstr != NULL  )
    {
        topic.toCharArray( cstr , length );
        cstr[length] = 0;
    }
    else
    {
        rprintf( "!!! ERROR: Unable to allocate memory for mqtt subscribe!\n" );       
    }

    MQTTSubscribe( cstr , callback );
}

// Subscribe callback function to topic
// It ccould be use # for get all topics
// Or specific topic: in this case the different callbacks
// is called just when that specific topic (or partially match from the begining)
// is received.
void MQTTSubscribe( const char * topic , MQTTCallback callback )
{
    MQTTcallbackitem * newitem = new struct MQTTcallbackitem;
    newitem->topic = topic;
    newitem->func =  callback;
    newitem->alltopics = strcmp( topic , "#" ) == 0 ? true : false;
    MQTTcallbacks.push_back(newitem);
    client.subscribe(topic);
    rprintf( "--->>> Registered topic: %s\n" , topic );
    numofMQTTcallbackitems++;

}

//------------------------------------------------------------------------------

// FIXME: This function is obsolote
void MQTTLoop( void )
{
}

//------------------------------------------------------------------------------


void _MQTTloopthread( void * params )
{

static unsigned long mqttloopthreadcount = 1;

    rprintf("%s: MQTT loop running on core: %d\n" , timeClient.getFormattedTime().c_str() , xPortGetCoreID() );
    while( true )
    {
        if( MQTTinited )
        {
            if( !client.connected() )
            {
                _MQTTConnect();
            }
            // unsigned long u = millis();
            // if( abs(u - MQTTlooptimer) > 25)
            // {
                client.loop();
            //     MQTTlooptimer = u;
            // }
            // Handle the MQTT queue

            if( mqttloopthreadcount%(2400) == 0 )
            {
                rprintf("%s: MQTT loop thread running on core: %d MQTT server: %s:%u Queue items: %d staksize: %d\n" , timeClient.getFormattedTime().c_str() , xPortGetCoreID() , MQTTserver.c_str() , (unsigned int)MQTTserverport , MQTTqueue.size() , getstacksize() );
                if( MQTTconnectionfail )
                {
                    rprintf("%s: MQTT connection fail: %d\n" , timeClient.getFormattedTime().c_str() , MQTTconnectionfail );
                }
            }

            if( mutex_mqttqueue != NULL )
            {
                if( xSemaphoreTakeRecursive( mutex_mqttqueue, portMAX_DELAY ) != pdTRUE )
                {
                    continue;
                }
            }

            for (std::vector<MQTTqueueitem*>::iterator it = MQTTqueue.begin(); it != MQTTqueue.end() ; )
            {
                if( (*it)->timing == 0 || (*it)->timing <= time( NULL ) )
                {
                    // rprintf( "QUEUEitem: time:%d timing: %d\n" , time( NULL ) , (*it)->timing );
                    //  Publish the topic
                    if( _MQTTPublish( (*it)->topic.c_str() , (*it)->payload.c_str() , (*it)->retain ) )
                    {
                        // rprintf("_MQTTloopthread::delete item\n" );
                        delete *it;
                        // rprintf("_MQTTloopthread::delete vector item\n" );
                        it = MQTTqueue.erase( it );
                    }
                    client.loop();
                }
                else
                {
                    if( abs((*it)->timing - time( NULL )) > 1 )
                    {
                        // Print just once in 15 sec
                        if( (mqttloopthreadcount%150) == 0 )
                        {
                            rprintf("MQTT queue item::time %ld  timing %ld [%s]::%s\n" , time( NULL ) , (*it)->timing , (*it)->topic.c_str() , (*it)->payload.c_str()  );
                        }
                    }
                    it++;
                }

            }
            if( mutex_mqttqueue != NULL )
            {
                xSemaphoreGiveRecursive( mutex_mqttqueue );
            }
            // Handle continous publish fail
            if( MQTTpublishfail > 5 )
            {
                rprintf("%s: !!! ERROR: Continous MQTT publish error!\nNow rebooting the device...\n" , timeClient.getFormattedTime().c_str() );
                delay( 500 );
                restart();
            }
        }
        else
        {
            if( mqttloopthreadcount%(10*60) == 0 )
            {
                rprintf("%s: !!! WARNING: MQTT NOT inited!\n" , timeClient.getFormattedTime().c_str() );
            }
        }

        delay( 25 );
        mqttloopthreadcount++;
    }

}

// Launch loop thread
void _MQTTloopinit( void )
{

#ifdef MQTTLEDDEBUG
    pinMode(MQTTLEDPIN, OUTPUT);
#endif

    if( mutex_mqttpublish == NULL )
    {
        mutex_mqttpublish = xSemaphoreCreateRecursiveMutex( );
    }

    if( mutex_mqttqueue == NULL )
    {
        mutex_mqttqueue = xSemaphoreCreateRecursiveMutex( );
    }

    if( pxMQTTloopthread == NULL )
    {
        // xTaskCreatePinnedToCore
        xTaskCreate(
            _MQTTloopthread,         /* pvTaskCode */
            "MQTT loop",             /* pcName */
            5000,                   /* usStackDepth */
            NULL,                   /* pvParameters */
            0,                      /* uxPriority */
            &pxMQTTloopthread        /* pxCreatedTask */
            );
    }

}

void MQTTend( void )
{
    MQTTinited = false;
    client.disconnect();
    if( pxMQTTloopthread != NULL )
    {
        vTaskDelete( pxMQTTloopthread );
    }
}

//------------------------------------------------------------------------------
// JSON 6.x
void jsonmerge(JsonObject dest, JsonObjectConst src)
{
   for (JsonPairConst kvp : src)
   {
     dest[kvp.key().c_str()] = kvp.value();
   }
}

//------------------------------------------------------------------------------

void MQTTbase::publish( const char * basetopic , const char * devclass , const char * device , const char * name , const char * type , JsonObject values  )
{
JsonDocument    root;
String topic;
String payload;

    // -------------------------------------------------------------------------
    // FIXME:  legacy

    topic = basetopic + String('/') + device + String('/') + devclass;
    root["name"] = name;
    root["type"] = type;

    jsonmerge( root.as<JsonObject>() , values );

    serializeJson( root , payload );
    // payload.replace( "\\r\\n" , "" );
    MQTTPublish( topic , payload );
    // rprintf( "%s: MQTTbase::publish::topic: %s payload: %s\n" , timeClient.getFormattedTime().c_str() , topic.c_str() ,  payload.c_str());
    // -------------------------------------------------------------------------
    
}

