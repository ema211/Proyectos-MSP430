#include <msp430.h>
volatile unsigned char dato;
volatile unsigned char valor;
volatile unsigned char duty_cycle;
volatile unsigned int periodo = 1000;

// ISR UART 0:
#pragma vector=USCIAB0RX_VECTOR
__interrupt void UART_RX_ISR (void) {
    dato = UCA0RXBUF;
    if ((dato >='0') && (dato<='9')) valor=valor*10 + dato-'0';
    if (dato == 13) {
        duty_cycle = valor;
        valor=0;
        // Modificación de CCR1 del timer empleado:
        if (duty_cycle < 100)  TA1CCR1 = ((unsigned long)duty_cycle*periodo)/100; // us.
        else TA1CCR1 = periodo; // us.
    }
}



void clk_calib (void) {
    //1Mhz
    if (CALBC1_1MHZ==0xFF) // If calibration constant erased
    {
        while(1); // do not load, trap CPU!!
    }
    DCOCTL = 0; // Select lowest DCOx and MODx settings
    BCSCTL1 = CALBC1_1MHZ; // Set range
    DCOCTL = CALDCO_1MHZ;
}

int main(void) {
    WDTCTL = WDTPW | WDTHOLD; // stop watchdog timer
    clk_calib();

    /*  ***  USCI - UART  ***  */
    // P1.1 UCA0RXD y P1.2 UCA0TXD
    P1SEL|=BIT2+BIT1; //P1.1 y P1.2 UART
    P1SEL2|=BIT2+BIT1; //P1.1 y P1.2 UART

    // Regisro de control 0 no se toca. Paridad deshabilitada, Primero LSB, 8 bits de datos, 1 bit de stop, Modo UART y Modo asincrono.

    // Registro de control 1:
    UCA0CTL1 = UCSSEL_3; // Selección de fuente de reloj: SMCLK = 1 MHz

    // Registro baud-rate control 1-0
    // Con SMCLK y 9600 bps, nos da 104.17 bps
    UCA0BR0 = 104;

    // Registro de control 1:
    UCA0CTL1 &= ~UCSWRST; //Apagar bit de reset para que funcione USCI - UART. Predeterminado en 1.

    // Registro habilitador de interrupciones:
    IE2 |= UCA0RXIE; //Hab local de interrupción en la recepción.

    /*  Timer A1 - PWM OUT MODE 7 con CCR1  */
    // Configuración de P2.2 para fungir como Timer1_A3.TA1(modo up, reset set)
    P2DIR |= BIT2;
    P2SEL |= BIT2;

    // Periodo
    TA1CCR0 = periodo; 
    // DutyCycle
    TA1CCR1 = 0; 

    TA1CTL |= TASSEL_2 | MC_1; // SMCLK, UP MODE
    TA1CCTL1 = OUTMOD_7; //Reset Set

    __bis_SR_register(GIE); // Habilitador global

    while (1);
}