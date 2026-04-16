#include <msp430.h>


void delay_us (unsigned short tiempo_us) {
    TA0CCR0 = TA0R + tiempo_us;
    while ((TA0CCTL0 & 1)==0); // Esperamos bandera
    TA0CCTL0 &= ~(1<<0); // Apagamos bandera
}

void main(void) {
    WDTCTL = WDTPW | WDTHOLD; // stop watchdog timer
    // P2SEL despues de reset GPIO
    // P2SEL2 despues de reset GPIO
    P2DIR=0x01; // P2.0 como salida
    TA0CTL=(2<<8)+(2<<4); // SMCLK, Modo Continuo

    unsigned short T_us = 1000; // Periodo de 1 ms
    unsigned char dutyCycle = 0;
    unsigned short TON_us, TOFF_us;
    TON_us = ((unsigned long) dutyCycle * T_us)/100;
    TON_us = (TON_us<=T_us)? TON_us : 500;
    TOFF_us = T_us - TON_us;

    while (1) {
        P2OUT |= 0x01;
        delay_us (TON_us);
        P2OUT &= ~0x01;
        delay_us (TOFF_us);
    }
}