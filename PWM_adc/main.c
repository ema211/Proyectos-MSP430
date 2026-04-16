#include <msp430.h> 


/**
 * main.c
 */


#define T_us 10000 // Constante, indica el periodo de la señal PWM
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
    if (TON_us == 0) {
        P2OUT &= ~BIT0;
        TA1CCR0 += T_us; // Se mantiene lo establecido como periodo
        estado = 0;
    }
    else if (TON_us == T_us) {
        P2OUT |= BIT0;
        TA1CCR0 += T_us; // Se mantiene lo establecido como periodo
        estado = 1;
    }
    else {
        P2OUT ^= BIT0; // Toggle a P2.0
        if (estado) TA1CCR0 += TOFF_us; // Actualización para siguiente interrupción (Tiempo en bajo).
        else TA1CCR0 += TON_us;
        estado = !estado; // Conmutamos estado - se asigna booleano, solo puede ser 0 o 1
    }
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
    TA0CCR0 = TA0R; // + 0 ms
    TA0CCTL0 |= (1<<4); // LOCAL INTERRUPT ENABLE


    //Configuración Timer A1:
    TA1CTL = (2<<8)|(2<<4); // SMCLK y MODO CONTINUO
    TA1CCR0 = TA1R + 10000; // + 10 ms
    TA1CCTL0 |= (1<<4); // LOCAL INTERRUPT ENABLE

    _bis_SR_register(GIE); // GLOBAL INTERRUPT ENABLE.

    while (1) {
        // Calculos (Tiempos de operación deseados):
        // Trabajamos con los valores del ADC en el rango de 52 - 971, son los limites a los que se puede tratar al duty cycle de forma deseada con el potenciometro empleado.
        if (ADC_value <= 52) dutyCycle = 0;
        else if (ADC_value >= 971) dutyCycle = 100;
        else {
            dutyCycle = ((ADC_value-52)*97)/919 + 1;
        }
        TON_us = ((unsigned long) dutyCycle * T_us)/100;
        TOFF_us = T_us - TON_us;
    }

    return 0;
}