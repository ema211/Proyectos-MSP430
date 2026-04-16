#include <msp430.h> 
#include <stdbool.h>
#include <string.h>

volatile unsigned short t_viejo=0;
volatile unsigned long frecuencia;
volatile unsigned short overflow_timer;


void clk_calib (void);
void frecuencimeter_init();
void dc_motor_init();
void dc_motor_init();
void uart_hw_init();
void adc_init();

//volatile unsigned char dato;
volatile unsigned int valor = 0;
volatile unsigned int duty_cycle;
volatile unsigned int periodoMotor = 49;

unsigned char mensaje[]={"##.## m/s\r\n"};
volatile unsigned char indexUart;

volatile unsigned int contador_ms = 0;

volatile unsigned char paso_medicion = 0; 


/****************** UART RX ISR  
#pragma vector=USCIAB0RX_VECTOR
__interrupt void UART_RX_ISR (void) {
    dato = UCA0RXBUF;
    if ((dato >='0') && (dato<='9')) valor=valor*10 + dato-'0';
    if (dato == 13) {
        duty_cycle = valor;
        valor=0;
        // Modificación de CCR1 del timer empleado:
        if (duty_cycle < 100)  TA0CCR1 = ((unsigned long)duty_cycle*periodoMotor)/100; // us.
        else TA0CCR1 = periodoMotor; // us.
    }
}
******************/

/****************** UART TX ISR  ******************/
#pragma vector=USCIAB0TX_VECTOR
__interrupt void UART_TX_ISR (void){
    UCA0TXBUF=mensaje[indexUart++];
    if (mensaje[indexUart]==0) IE2&=~UCA0TXIE;                  //Deshab local
}

// ISR ADC10
#pragma vector = ADC10_VECTOR
__interrupt void ADC_ISR (void) {
    volatile long ADC_value;
    ADC_value = ADC10MEM;

    if (ADC_value <= 52) duty_cycle = 0;
    else if (ADC_value >= 971) duty_cycle = 100;
    else {
        duty_cycle = ((ADC_value-52)*97)/919 + 1;
    }

    // Modificación de CCR1 del timer empleado:
    if (duty_cycle < 100)  TA0CCR1 = ((unsigned long)duty_cycle * periodoMotor)/100; // us.
    else TA0CCR1 = periodoMotor; // us.
}



/****************** TIMER 1 ISR´s  ******************/
//ISR vector 
#pragma vector=TIMER1_A1_VECTOR
__interrupt void T1_CCR1_CCR2_OVF (void){
    switch (TA1IV){
        case 2      :  break;          //CCR1
        case 4      :  break;          //CCRS2
        case 0x0A   :
            overflow_timer++;

            
            break;
    }
}

//CCR0 Velocimetro
#pragma vector=TIMER1_A0_VECTOR
__interrupt void Input_capture_ISR (void){
    //si es hora de medir, primero dejamos pasar el primer ciclo
    if (paso_medicion == 0) {
        paso_medicion = 1; 
    }

    //si ya termino el primer ciclo, pasamos al segundo para tener un 
    //t viejo y t nuevo reales
    if (paso_medicion == 1) {
        t_viejo = TA1CCR0;      
        overflow_timer = 0;      
        paso_medicion = 2;       
    } 
    else if (paso_medicion == 2) { //una vez la medicion completa se mide
        unsigned short t_nuevo = TA1CCR0;
        unsigned long periodo_largo;
        unsigned short vel;
        

        if (t_viejo > t_nuevo) {
            periodo_largo = (unsigned long)t_nuevo - t_viejo + (overflow_timer - 1) * 65536;
        } else {
            periodo_largo = (unsigned long)t_nuevo - t_viejo + (overflow_timer) * 65536;
        }

        
        if(periodo_largo < 1000){ //valor demaciado grande, talvez ruido -> invalido
            paso_medicion = 0;    //se vuelve a medir
        } else{
            TA1CCTL0 &=~ CCIE;
            paso_medicion = 0;      
            
            /*
            El encoder que usamos es de 20 dientes y usamos una rueda de 31mm
            Se necesita distancia para velocidad, esto en una rueda es el perimetro
            Perimetro rueda = pi x 31mm = 97.39 mm
            distancia entre pulso = perimetro/pulsos posibles =97.39/20 = 4.87mm
            Si frecuencia es pulsos por segundo y distancia es distancia por un pulso entonces  velocidad es frecuencia por distancia entre pulso
            vel = frecuencia * 0.0048695m

            Antes se tenia la formula para frecuencia 
            frecuencia = 1000000 / periodo_largo;
            Entonces primero se multiplica por un millon y recorremos el decimal para trabajar con enteros
            1_000_000 * 0.004895 * 1000 = 4_869_500
            teniendo eso solo dividimos  ese valor entre el periodo
            */

            vel = 486950 / periodo_largo;

            mensaje[4] = (vel % 10) + '0';       // Centésimas
            vel /= 10;
            mensaje[3] = (vel % 10) + '0';       // Décimas
            vel /= 10;
            
            mensaje[1] = (vel % 10) + '0';       // Unidades
            vel /= 10;
            mensaje[0] = (vel % 10) + '0';       // Decenas

            indexUart = 0;
            IE2 |= UCA0TXIE;        // Disparamos la transmisión de la nueva frecuencia
        }
    }
}


/****************** TIMER 0 ISR´s  ******************/
//ISR vector 
#pragma vector=TIMER0_A1_VECTOR
__interrupt void T0_CCR1_CCR2_OVF (void){
    switch (TA0IV){
    case 2      :  break;          //CCR1
    case 4      :  break;          //CCRS2
    case 0x0A   :
        contador_ms++;
            if (contador_ms >= 10000) { // 10,000 ciclos = 0.5s
                TA1CCTL0 |= CCIE;
                ADC10CTL0 |= ADC10SC; // iniciar conversion adc
                contador_ms = 0;
        }
        break;
    }
}


/****************************************************/
/******************      MAIN      ******************/
/****************************************************/
int main(void){
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
        
    clk_calib();

    frecuencimeter_init();
    dc_motor_init();
    uart_hw_init();
    adc_init();

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


void frecuencimeter_init(){
    //Timer A1 CCR0, Overflow
	P2SEL|=BIT3;
	P2SEL2&=~BIT3;
	P2DIR&=~BIT3;

	TA1CTL|=TASSEL_2+MC_2+TAIE;
	TA1CCTL0 = CM_1+CCIS_1+CAP;  //Flanco de subida, Mux con entrada B, Capture mode, 
    //Empieza deshabilitado
}

void dc_motor_init(){
    //Timer A0 CCR0, CCCR1
    //P1.6 como salida TA0.1
    P1DIR |= BIT6; 
    P1SEL |= BIT6; 
    P1SEL2 &= ~BIT6;         

    //Timer 0 up mode
    TA0CCR0 = 49;           // Frecuencia de 20 kHz (1/50ms)
    TA0CCR1 = 25;
    
    TA0CTL |= TASSEL_2 | MC_1; // SMCLK, Up Mode, Limpiar contador
    TA0CCTL1 = OUTMOD_7;        // Modo Reset-Set
    TA0CTL |= TAIE;
}



void uart_hw_init(){
    /****************** USCI - UART ******************/
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


void adc_init(){
    // Configuración ADC10:
    ADC10AE0 |= BIT3; // P1.3 (A3)
    ADC10CTL0 |= ADC10ON | ADC10IE; // ADC ON, INTERRUPT EN
    ADC10CTL1 |= INCH_3; // TRABAJAR CON EL CANAL 3
    ADC10CTL0 |= ENC; // HABILITAR CONVERSION
}