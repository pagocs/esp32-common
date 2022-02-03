#include <Esp.h>
#include <ledblink.h>

//------------------------------------------------------------------------------

void ledblink( int state )
{

    if( ledblink_blueledstate == LEDBLINK_DISABLE || state == LEDBLINK_DISABLE )
    {
        ledblink_blueledstate = LEDBLINK_DISABLE;
        return;
    }

    if( state == LEDBLINK_INIT || ledblink_blueledstate == LEDBLINK_INIT )
    {
      pinMode(LED_BUILTIN, OUTPUT);
      ledblink_blueledstate = HIGH;
    }
    else if( state == LEDBLINK_NOPARAM )
    {
      ledblink_blueledstate = ledblink_blueledstate ? 0 : 1;
    }
    else
    {
      ledblink_blueledstate = state;
    }
    digitalWrite(LED_BUILTIN, ledblink_blueledstate);   // turn the LED on (HIGH is the voltage level)
}

LEDBlink::LEDBlink( )
{
    state =  LEDBLINK_INIT;
    status = true;

}

void LEDBlink::blink( void )
{
    ledblink();
}

void LEDBlink::disable( void )
{
    status = false;
    // legacy
    ledblink( LEDBLINK_DISABLE );

}

void LEDBlink::enable( void )
{
    ledblink( LEDBLINK_INIT );
}
