#include <STC8.h>

sbit led1=P3^0; //fire
sbit led2=P3^1; //propulsion

bit  fire_io = 0;
long fire_cycle = 20000;
long fire_stat = 20000;

bit  propulsion_io = 0;
unsigned int propulsion_stat = 0;
bit  lighter = 1;
unsigned int period_idx = 0;
int propulsion_cycle = 500;

void port_init(void)
{
   P3M1=0x00;
   P3M0=0x00;
   P3 = 0xFF;
}


void main(void){
    port_init();

    while(1)
    {
        // fire
        if (fire_stat<0) {fire_io=!fire_io; fire_stat=fire_cycle; led1=fire_io;};
        fire_stat--;
        
        // propulsion
        if (period_idx==propulsion_cycle) {
            period_idx=0; propulsion_stat=(propulsion_stat+1)%propulsion_cycle;
            if (propulsion_stat == 0) lighter=!lighter;
        }
        if (lighter) {
            if (period_idx<propulsion_stat) led2 = 0;
            else led2 = 1;
        } 
        else {
            if (period_idx<propulsion_stat) led2 = 1;
            else led2 = 0;
        }
        period_idx++;

    }

}