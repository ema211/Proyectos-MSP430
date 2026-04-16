#include <msp430.h>

void clk_calib (void)
{
    //1Mhz
    if (CALBC1_1MHZ==0xFF)      // If calibration constant erased
    {
        while(1);               // do not load, trap CPU!!
    }
    DCOCTL = 0;                 // Select lowest DCOx and MODx settings
    BCSCTL1 = CALBC1_1MHZ;      // Set range
    DCOCTL = CALDCO_1MHZ;
}

int main(void)
{
    unsigned char i;

    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    clk_calib();

    P1SEL |= BIT2+BIT1;         //P1.1 y P1.2 UART
    P1SEL2|= BIT2+BIT1;         //P1.1 y P1.2 UART

    UCA0CTL1=UCSSEL_3;          //SMCLK = 1 MHz
    UCA0BR0=104;                // Con SMCLK y 9600 bps, nos da 104.17 bps

    UCA0CTL1&=~UCSWRST;         //Apagar bit para que funcione la UART

    //UCA0TXBUF='A';            // Envia 'A' o 0x41 (65). 'a' 0x61 (97)

    for(i='A'; i<='Z'; i++)
    {
        UCA0TXBUF=i;
        do{}while ((IFG2 & UCA0TXIFG)==0);    //No esta empty UCA0TXBUF
    }

    while (1);
}