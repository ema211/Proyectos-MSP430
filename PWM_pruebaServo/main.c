#include <msp430.h> 


/**
 * main.c
 */

// El movimiento del servomotor esta limitado de un valor en alto de 1 ms a 2 ms.
// Corresponde de 1% a 13% de duty cycle para una señal con periodo de 20 ms

#define T_us 20000 // Constante, indica el periodo de la señal PWM 20 ms
volatile unsigned char dutyCycle; // Especificación de % del DC
volatile unsigned short TON_us, TOFF_us;
volatile unsigned short estado;


volatile long ADC_value;

// ISR ADC10
#pragma vector = ADC10_VECTOR;
__interrupt void ADC_ISR (void) {
    ADC_value = ADC10MEM;
    

}

// ISR TIMER A0
#pragma vector = TIMER0_A0_VECTOR;
__interrupt void TIMER_A0_ISR (void) {
    ADC10CTL0 |= ADC10SC; // INICIAR CONVERSION.
    TA0CCR0 += 50000; // Proxima interrupción (conversión) en 50,000 us => 50 ms.
}

// ISR TIMER A1
#pragma vector = TIMER1_A0_VECTOR;
__interrupt void TIMER1_A0_ISR (void) {
    P2OUT ^= BIT0; // Toggle a P2.0
    if (estado) TA1CCR0 += TOFF_us; // Actualización para siguiente interrupción (Tiempo en bajo).
    else TA1CCR0 += TON_us;
    estado = !estado; // Conmutamos estado - se asigna booleano, solo puede ser 0 o 1
}

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

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    clk_calib(); // Calibración del clk

    // Configuración de pines de puertos inicialmente como entradas GPIO
    P2DIR |= BIT0; // P2.0 como salida
    P2OUT |= BIT0; // Estado inicial HIGH en P2.0
    estado = 1;

    // Configuración ADC10:
    ADC10AE0 |= (1<<1); // P1.1 (A1)
    ADC10CTL0 |= ADC10ON | ADC10IE; // ADC ON, INTERRUPT EN...
    ADC10CTL1 |= INCH_1; // TRABAJAR CON EL CANAL 1.
    ADC10CTL0 |= ENC; // HABILITAR CONVERSION.

    //Configuración Timer A0:
    TA0CTL = (2<<8)|(2<<4); // SMCLK y MODO CONTINUO
    TA0CCR0 = TA0R; // + 1 ms
    TA0CCTL0 |= (1<<4); // LOCAL INTERRUPT ENABLE


    //Configuración Timer A1:
    TA1CTL = (2<<8)|(2<<4); // SMCLK y MODO CONTINUO
    TA1CCR0 = TA1R + 10000; // + 10 ms
    TA1CCTL0 |= (1<<4); // LOCAL INTERRUPT ENABLE

    _bis_SR_register(GIE); // GLOBAL INTERRUPT ENABLE.

    while (1) {
        // Calculos (Tiempos de operación deseados 2% a 14%):
        dutyCycle = (ADC_value*12)/1023 + 2;
        TON_us = ((unsigned long) dutyCycle * T_us)/100;
        TON_us = (TON_us<=T_us)? TON_us : T_us/2;
        TOFF_us = T_us - TON_us;
    }
    return 0;
}