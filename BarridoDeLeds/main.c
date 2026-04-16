#include <msp430.h>

unsigned char secuencia[] = {8,4,2,1};

int main(void){
    WDTCTL = WDTPW + WDTHOLD;                 // Stop watchdog timer
    P2DIR |= 0x0F;                            // Set P1.0 to output direction


    unsigned int i = 0;
    while(1){
      if (P1IN & 0x08 ) P2OUT = secuencia[i-- % 4]; 
        else P2OUT = secuencia[i++ % 4]; 
        
      delay();
  }
}




void delay(){
  volatile unsigned int i;
  for (i=10000; i>0; i--);
}