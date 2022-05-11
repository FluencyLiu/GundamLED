#include <STC15.h>

sbit led1=P3^0;

void port_init(void)
{
   P3M1=0x00;
   P3M0=0x00;
   P3 = 0xFF;
}

void Delay10ms(unsigned int c)                //@11.0592MHz
{
    unsigned char a,b;

    for(c;c>0;c--)
    {
        for(b=38;b>0;b--)
        {
            for(a=130;a>0;a--) {}
        }
    }
}

void main(void){
    port_init();

    while(1)
    {
        led1=0; Delay10ms(100);
        led1=1; Delay10ms(100);
    }

}