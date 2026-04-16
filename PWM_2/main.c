#include <msp430.h>

#define T_us 1000 // Constante, indica el periodo de la señal PWM
volatile unsigned char dutyCycle =0; // Especificación de % del DC
volatile unsigned short TON_us, TOFF_us;
volatile unsigned short estado;

void clk_calib(void) {
    //1Mhz
    if (CALBC1_1MHZ==0xFF) // If calibration constant erased
    {
    while(1); // do not load, trap CPU!!
    }
    DCOCTL = 0; // Select lowest DCOx and MODx settings
    BCSCTL1 = CALBC1_1MHZ; // Set range
    DCOCTL = CALDCO_1MHZ;
}

#pragma vector=TIMER0_A0_VECTOR // ISR Timer0 CCR0
__interrupt void Timer_A (void) {
    P2OUT ^= BIT0; // Toggle a P2.0
    if (estado) TA0CCR0 += TOFF_us; // Actualización para siguiente interrupción (Tiempo en bajo).
    else TA0CCR0 += TON_us;
    estado = !estado; // Conmutamos estado - se asigna booleano, solo puede ser 0 o 1
}
void main(void) {
    WDTCTL = WDTPW | WDTHOLD; // stop watchdog timer

    clk_calib(); // Calibración del clk

    // Configuración de pines de puertos inicialmente como entradas GPIO
    P2DIR |= BIT0; // P2.0 como salida

    TA0CTL=(2<<8)|(2<<4); // SMCLK, Modo Continuo

    // Calculos iniciales (Tiempos de operación deseados):
    TON_us = ((unsigned long) dutyCycle * T_us)/100;
    TON_us = (TON_us<=T_us)? TON_us : T_us/2;
    TOFF_us = T_us - TON_us;
    // Se configura interrupción inicial en el tiempo deseado en alto
    P2OUT |= BIT0; // Estado inicial HIGH en P2.0
    estado = 1;
    TA0CCR0= TA0R + TON_us; // Tiempo en alto inicial

    TA0CCTL0|=(1<<4); // local interrupt enable
    __bis_SR_register(GIE); // Global interrupt enable

    while (1);
}