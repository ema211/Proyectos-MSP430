#include <msp430.h>

void delay();
void motor_port(unsigned char valor);

unsigned char secuencia[8] = {0b1000, 0b1100, 0b0100, 0b0110, 0b0010, 0b0011, 0b0001, 0b1001};

int main(void){
    WDTCTL = WDTPW + WDTHOLD;     //Detiene el watchdog

    P1DIR |= 0x04;                //Puerto P1.2 como salida
    P1DIR |= 0x10;                //Puerto P1.4 como salida
    P2DIR |= 0x01;                //Puerto P2.0 como salida
    P2DIR |= 0x04;                //Puerto P2.2 como salida
  
    unsigned int i = 0;

    while(1){
      if (P1IN & 0x08 ) motor_port(secuencia[i++ % 8]); 
        else motor_port(secuencia[i-- % 8]); 
      delay(7000);
  }
}

void delay(unsigned int cuenta){
  volatile unsigned int i;
  i = cuenta;
  while (i > 0) i--;
}

void motor_port(unsigned char valor){
  P1OUT = (valor & (1<<0)) ? (P1OUT |  (1<<2)) : (P1OUT & ~(1<<2)); //Pin P1.2
  P1OUT = (valor & (1<<1)) ? (P1OUT |  (1<<4)) : (P1OUT & ~(1<<4)); //Pin P1.4
  P2OUT = (valor & (1<<2)) ? (P2OUT |  (1<<0)) : (P2OUT & ~(1<<0)); //Pin P2.0
  P2OUT = (valor & (1<<3)) ? (P2OUT |  (1<<2)) : (P2OUT & ~(1<<2)); //Pin P2.2
} 