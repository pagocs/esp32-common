#ifndef _LEDBLINK_H
#define _LEDBLINK_H

//------------------------------------------------------------------------------

#ifndef LED_BUILTIN
#ifdef BOARD_LOLIN
// Lolin
#define LED_BUILTIN 5
#endif // BOARD_LOLIN
#ifdef BOARD_DOIT
// Doit
#define LED_BUILTIN 2
#endif // BOARD_LOLIN

#ifdef BOARD_GENERIC
#define LED_BUILTIN 5
#endif // BOARD_GENERIC

#ifdef BOARD_PCB
#define LED_BUILTIN 23
#endif // BOARD_GENERIC
#endif // LED_BUILTIN

#define LEDBLINK_INIT -1
#define LEDBLINK_DISABLE -5
#define LEDBLINK_NOPARAM -2

// FIXME:
// Legacy
int ledblink_blueledstate = LEDBLINK_INIT;
void ledblink( int state = LEDBLINK_NOPARAM );

class LEDBlink {
    protected:
        int     state =  LEDBLINK_INIT;
        bool    status = true;
    public:
        LEDBlink( void );
        void blink( void );
        void disable( void );
        void enable( void  );
};

#include <ledblink.ino>

#endif //#ifndef _LEDBLINK_H
