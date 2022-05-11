#include <STC15.h>

sbit led1=P3^0;

void port_init(void)
{
   P3M1=0x00;
   P3M0=0x00;
   P3 = 0xFF;
}

void delay(unsigned int c)
{
    unsigned char a,b;

    for(c;c>0;c--)
    {
        for(b=100;b>0;b--)
        {
            for(a=100;a>0;a--) {}
        }
    }
}

void main(void){
    port_init();

    while(1)
    {
        led1=0; delay(100);
        led1=1; delay(100);
    }

}