#include <msp430.h>

void delay_ms(unsigned int);
void motor_port(unsigned char valor);

unsigned char secuencia[8] = {0b1000, 0b1100, 0b0100, 0b0110, 0b0010, 0b0011, 0b0001, 0b1001};

#pragmavector=TIMER0_A0_VECTOR // ISR TIMER 0 CCR0
__interrupt void Timer_A (void){  

    if (P1IN & 0x08 ) motor_port(secuencia[i++ % 8]); 
    else motor_port(secuencia[i-- % 8]); 
    TA0CCR0 += 1000;    //Cuenta 1000 veces mas desde el ultimo valor de TA0CCR0

}

int main(void){
    WDTCTL = WDTPW + WDTHOLD;     //Detiene el watchdog

    P1DIR |= 0x04;                //Puerto P1.2 como salida
    P1DIR |= 0x10;                //Puerto P1.4 como salida
    P2DIR |= 0x01;                //Puerto P2.0 como salida
    P2DIR |= 0x04;                //Puerto P2.2 como salida
  
    unsigned int i = 0;}
        
    TA0CTL = (2<<8)+(2<<4);       //Timer Init
    TA0CCR0 = TA0R + 1000;    //Cuenta 1000 veces sin importar el valor del registro del timer
    
    TA0CCTL0|=(1<<4); // local interrupt enable
    __bis_SR_register(GIE); // Global interrupt enable

    while(1){

      delay_ms(1);
  }
}

void delay_ms(unsigned int cuenta){
  unsigned int milis;
  milis = cuenta;
  do{
    
    do {}while ( (TA0CCTL0 & 1) == 0);    //espera bandera
    TA0CCTL0 &=~ (1<<0);    //reset a bandera
  } while(--milis != 0);

  
}

void motor_port(unsigned char valor){
  P1OUT = (valor & (1<<0)) ? (P1OUT |  (1<<2)) : (P1OUT & ~(1<<2)); //Pin P1.2
  P1OUT = (valor & (1<<1)) ? (P1OUT |  (1<<4)) : (P1OUT & ~(1<<4)); //Pin P1.4
  P2OUT = (valor & (1<<2)) ? (P2OUT |  (1<<0)) : (P2OUT & ~(1<<0)); //Pin P2.0
  P2OUT = (valor & (1<<3)) ? (P2OUT |  (1<<2)) : (P2OUT & ~(1<<2)); //Pin P2.2
} 
