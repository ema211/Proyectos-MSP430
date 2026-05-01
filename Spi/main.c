#include <msp430.h>

void uart_hw_init();
void clk_calib (void);
void USCI_B0_SPI_init();

volatile unsigned char valorPot[2] = {0, 50};
volatile unsigned tr_index = 0;
volatile unsigned int valor = 0;
volatile unsigned char dato = 0;


/****************** UART RX ISR  ******************/
#pragma vector=USCIAB0RX_VECTOR
__interrupt void UART_RX_ISR (void) {
    dato = UCA0RXBUF;
    if ((dato >='0') && (dato<='9')) valor=valor*10 + dato-'0';
    if (dato == 13) {
        valorPot[1] = valor;
        valor = 0;
        tr_index = 0;

        P2OUT &= ~(BIT3);   //cs en bajo para activar al esclavo
        IE2 |= UCB0TXIE; //Hab local de interrupción
    }
}

#pragma vector=USCIAB0TX_VECTOR
__interrupt void USCI_AB0_ISR (void){
    volatile char dummy; // Variable para limpiar el buffer de entrada
    UCB0TXBUF = valorPot[tr_index++]; 
    
    // Mientras se transmite, el hardware recibe y no queremos que se vuelva a escribir otro dato
    //Por eso leemos el dato actual para borrar la bandera
    dummy = UCB0RXBUF; 

    if (tr_index == 2){
      while (UCB0STAT & UCBUSY);  //Se espera a que se mande el ultimo byte
      IE2 &= ~UCB0TXIE; // Deshabilitar al terminar
      P2OUT |= BIT3;    //Deshab esclavo
    }
}


int main(void){
    WDTCTL = WDTPW + WDTHOLD;                 // Stop watchdog timer

    clk_calib();

    P2OUT |= BIT3;     //El esclavo inicia deshabilitado
    P2DIR |= BIT3;     //P2.3 como CS spi            

    USCI_B0_SPI_init();        
    uart_hw_init();   

    __bis_SR_register(GIE);
    while(1){
  }
}

void clk_calib (void){
            //1Mhz
    if (CALBC1_1MHZ==0xFF){                   // If calibration constant erased
        while(1);                             // do not load, trap CPU!!
    }
    DCOCTL = 0;                               // Select lowest DCOx and MODx settings
    BCSCTL1 = CALBC1_1MHZ;                   // Set range
    DCOCTL = CALDCO_1MHZ;
}


void USCI_B0_SPI_init(){
  P1SEL |= BIT5 + BIT6 + BIT7;
  P1SEL2 |= BIT5 + BIT6 + BIT7;

  UCB0CTL0 |= UCCKPH + UCMSB + UCMST + UCSYNC;
  UCB0CTL1 |= UCSSEL_2;                     

  UCB0CTL1 &= ~UCSWRST;
}

void uart_hw_init(){
    /****** USCI - UART ******/
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
    UCA0CTL1 &= ~UCSWRST; //Apagar bit de reset para que funcione USCI - UART. Predeterminado en 1

    // Registro habilitador de interrupciones
    IE2 |= UCA0RXIE; //Hab local de interrupción
}