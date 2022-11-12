#include <STC8.h>

#define CHECKMODE(mode,status) (EX1=0, (mode)==(status))
#define JET() \
{ \
    if (jet_idx==jet_cycle[stage_idx]) { \
        jet_idx=0; \
        jet_stat=(jet_stat+1)%jet_cycle[stage_idx]; \
        if (jet_stat == 0) { \
            lighter=!lighter;  \
            stage_idx = (stage_idx+1)%stage_cycle; \
        } \
    } \
    if (lighter) { \
        if (jet_idx<jet_stat) jet = 0; \
        else jet = 1; \
    } \
    else { \
        if (jet_idx<jet_stat) jet = 1; \
        else jet = 0; \
    } \
    jet_idx++; \
}

sbit jet=P3^0; // jet
sbit monitor=P3^1; // fire
// sbit gun=P3^1; // fire
sbit control=P3^3;

unsigned int stb = 0;

bit lighter = 1;
unsigned int stage_idx = 0;
unsigned int jet_idx = 0;
unsigned int jet_stat = 0;
unsigned int stage_cycle = 8;
unsigned int jet_cycle[8] = {300,200,300,200,300,200,400,300};

volatile int mode = 0;
int num_mode = 3;

void port_init(void)
{
    // P3 IO mode set
    P3M1=0xf0;
    P3M0=0x00;
    // P3 pull-up resistor
    P3PU=0x0c;
    // P3 init
    P3 = 0xFF;
    // INT1 set
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
    // delay a period to avoid jitter
    delay(10);
    if (control==0) stb += 1;
    delay(10);
    if (control==0 && stb) mode = (mode+1)%num_mode;
    stb = 0;
}

void main() {
    port_init();

    while(1)
    {
        if (CHECKMODE(mode, 0)) {
            EX1 = 1;
            monitor = 1;
            jet = 1;
        } else if (CHECKMODE(mode, 1)) {
            EX1 = 1;
            monitor = 0;
            JET()
        } else if (CHECKMODE(mode, 2)) {
            EX1 = 1;
            monitor = 0;
            jet = 0;
        }
        
        
    }
}