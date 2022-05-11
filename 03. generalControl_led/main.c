#include <STC8.h>

#define CHECKMODE(mode,status) (EX1=0, (mode)==(status))
#define FIRE() \
{\
    if (fire_stat<0) {fire_io=!fire_io; fire_stat=fire_cycle; led1=fire_io;}; \
    fire_stat--; \
}

#define BREATHING() \
{\
    if (period_idx==propulsion_cycle) {\
        period_idx=0; propulsion_stat=(propulsion_stat+1)%propulsion_cycle;\
        if (propulsion_stat == 0) lighter=!lighter;\
    }\
    if (lighter) {\
        if (period_idx<propulsion_stat) led2 = 0;\
        else led2 = 1;\
    } \
    else {\
        if (period_idx<propulsion_stat) led2 = 1;\
        else led2 = 0;\
    }\
    period_idx++;\
}

sbit led1=P3^0; //fire
sbit led2=P3^1; //propulsion
sbit control=P3^3;

bit  fire_io = 0;
long fire_cycle = 20000;
long fire_stat = 20000;

bit  propulsion_io = 0;
unsigned int propulsion_stat = 0;
bit  lighter = 1;
unsigned int period_idx = 0;
int propulsion_cycle = 404;

volatile int mode = 0;
int num_mode = 5;

void port_init(void)
{
   // P3M1=0xfc;
    P3M1=0xf0;
    P3M0=0x00;
    P3PU=0x0c;
    P3 = 0xFF;
    IT1 = 1;
    EX1 = 1;
    EA = 1;
}

void delay(unsigned int c) 
{
    for (c; c>0; c--)
    {
        unsigned char a,b;
        for(b=100; b>0; b--)
        {
            for(a=100; a>0; a--) {}
        }
    }
}

void INT1_Routine (void) interrupt 2 {
    delay(5);
    if (control==0) mode = (mode+1)%num_mode;
    // mode = (mode+1)%num_mode;
}

void main(void){
    port_init();

    while(1)
    {
        if (CHECKMODE(mode, 0)) {
            EX1 = 1;
            led1=1;
            led2=1;
        } else if (CHECKMODE(mode, 1)) {
            EX1 = 1;
            led1=1;
            BREATHING();
        } else if (CHECKMODE(mode, 2)) {
            EX1 = 1;
            FIRE();
            led2=1;
        } else if (CHECKMODE(mode, 3)) {
            EX1 = 1;
            FIRE();
            BREATHING();
        } else if (CHECKMODE(mode, 4)) {
            EX1 = 1;
            led1=0;
            led2=0;
        }
    }

}