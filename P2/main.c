#include <msp430.h> 
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

//Velocidad
unsigned char mensaje[]={"##.## m/s\r\n"};
volatile unsigned char indexUart;
volatile unsigned int contador_ms = 0;
volatile unsigned char paso_medicion = 0;

// GPS
volatile unsigned char head = 0;
volatile unsigned char tail = 0;
#define maxSize 82 // bytes
volatile unsigned char cola[maxSize];

// Bateria
unsigned char nivel[]={"### %\r\n"};
unsigned char nivel_b;
volatile unsigned char indexNivel;



/****************** UART RX ISR  ******************/
#pragma vector=USCIAB0RX_VECTOR
__interrupt void UART_RX_ISR (void) {
    cola[tail] = UCA0RXBUF; // Dato recibido por GPS
    if (cola[head] == '$') { // Si se almacena desde primer caracter de trama
        if (cola[tail++] == '\n') { // Si termino de recibirse la trama
            IE2 &= ~UCA0RXIE; // Deshabilitador local interrupción Rx. Cesamos recepción.
            IE2 |= UCA0TXIE; // Habilitador local interrupción Tx. Habilitamos transmision de la trama
        }
    }
}

/****************** UART TX ISR  ******************/
#pragma vector=USCIAB0TX_VECTOR
__interrupt void UART_TX_ISR (void) {
    if (head < tail && cola[tail-1] == '\n') { // Si se completo la recepcion de trama GPS
        UCA0TXBUF = cola[head++];
    }
    else if (mensaje[indexUart] != 0){
        UCA0TXBUF = mensaje[indexUart++]; // Envio de dato de velocidad.
    }
    else if (nivel[indexNivel] != 0) {
        UCA0TXBUF = nivel[indexNivel++]; // Envio de dato de nivel de tensión.
    }
    else {
        IE2 &= ~UCA0TXIE; // Deshabilitador local interrupción Tx.
        IE2 |= UCA0RXIE; // Habilitador local interrupción Rx.
        indexUart = 0;
        indexNivel = 0;
        head = 0;
        tail = 0;
    }

}

volatile unsigned short ADC_value[2]; // Almacenamos dos lecturas. Muestra de canal mass grande al mass pequeño
// ISR ADC10
#pragma vector = ADC10_VECTOR
__interrupt void ADC_ISR (void) {

    // Las conversiones se han realizado y por ende tambien su almacenamiento en arreglo.
    // Trabajo con ADC_value[1] - Lectura de P1.3
    if (ADC_value[1] <= 52) duty_cycle = 0;
    else if (ADC_value[1] >= 971) duty_cycle = 100;
    else {
        duty_cycle = ((ADC_value[1]-52)*97)/919 + 1;
    }
    // Modificación de CCR1 del timer empleado:
    if (duty_cycle < 100)  TA0CCR1 = ((unsigned long)duty_cycle * periodoMotor)/100; // us.
    else TA0CCR1 = periodoMotor-5; // us.

    // Trabajo con ADC_value[0] - Lectura de P1.4
    if (ADC_value[0] <= 731) nivel_b = 0; // Modulos operacionales a partir de una tension de 9 V => 2.36 => 731
    else if (ADC_value[0] >= 971) nivel_b = 100; // Se interpreta nivel de bateria al 100 % arriba de 971
    else nivel_b = ((ADC_value[0]-731)*100)/240;

    nivel[2] = (nivel_b % 10) + '0';       // Unidades
    nivel_b /= 10;
    nivel[1] = (nivel_b % 10) + '0';       // Decenas
    nivel_b /= 10;
    nivel[0] = (nivel_b % 10) + '0';       // Centenas

    TA1CCTL0 |= CCIE;   //Se activa interrupcion por Imput Capture
    TA1CTL |= TAIE;     //Se activa interrupcion por desbordamiento
}


/****** TIMER 1 ISR´s  ******/
//ISR vector
#pragma vector=TIMER1_A1_VECTOR
__interrupt void T1_CCR1_CCR2_OVF (void){
    switch (TA1IV){
        case 2      :  break;          //CCR1
        case 4      :  break;          //CCRS2
        case 0x0A   :
            overflow_timer++;
            //if ((TA1CCTL0 & CCIE) && overflow_timer >= 8)
            if (overflow_timer >= 8){
                TA1CCTL0 &=~ CCIE;  //Se desactiva interrupcion por Imput Capture
                TA1CTL &=~ TAIE;    //Se desactiva interrupcion por desbordamiento

                paso_medicion = 0;
                overflow_timer = 0;

                mensaje[4] = '0';
                mensaje[3] = '0';
                mensaje[1] = '0';
                mensaje[0] = '0';

                indexUart = 0;
                IE2 |= UCA0TXIE;

            }
            break;
    }
}

//TA0 CCR0 Velocimetro
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
            TA1CCTL0 &=~ CCIE;  //Se desactiva interrupcion por Imput Capture
            TA1CTL &=~ TAIE;    //Se desactiva interrupcion por desbordamiento

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

            //indexUart = 0;
            IE2 |= UCA0TXIE;        // Disparamos la transmisión de la nueva frecuencia
        }
    }
}


/****** TIMER 0 ISR´s  ******/
//ISR vector
#pragma vector=TIMER0_A1_VECTOR
__interrupt void T0_CCR1_CCR2_OVF (void){
    switch (TA0IV){
    case 2      :  break;          //CCR1
    case 4      :  break;          //CCRS2
    case 0x0A   :
        contador_ms++;
            if (contador_ms >= 10000) { // 10,000 ciclos = 0.5s
                if (!(ADC10CTL1 & ADC10BUSY)) {
                    ADC10CTL0 &= ~ENC;
                    ADC10SA = (unsigned int)ADC_value;
                    ADC10CTL0 |= ENC | ADC10SC;
                }
                contador_ms = 0;
        }
        break;
    }
}


/******************/
/******      MAIN      ******/
/******************/
int main(void){
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    clk_calib();

    frecuencimeter_init();
    dc_motor_init();
    uart_hw_init();
    adc_init();

    __bis_SR_register(GIE);

    while(1){}
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

    TA1CTL|=TASSEL_2+MC_2;
    TA1CCTL0 = CM_1+CCIS_1+CAP;  //Flanco de subida, Mux con entrada B, Capture mode
    //Interrupciones por ImputCapture y desbordamiento deshabilitados
}

void dc_motor_init(){
    //Timer A0 CCR0, CCCR1
    //P1.6 como salida TA0.1
    P1DIR |= BIT6;
    P1SEL |= BIT6;
    P1SEL2 &= ~BIT6;

    //Timer 0 up mode
    TA0CCR0 = 49;           // Frecuencia de 20 kHz (1/50ms)
    TA0CCR1 = 0;

    TA0CTL |= TASSEL_2 | MC_1; // SMCLK, Up Mode, Limpiar contador
    TA0CCTL1 = OUTMOD_7;        // Modo Reset-Set
    TA0CTL |= TAIE;
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


void adc_init(){
    // Configuración ADC10:
    ADC10AE0 |= BIT3 | BIT4; // P1.3 (A3) y P1.4 (A4)
    ADC10CTL0 = ADC10ON | ADC10IE | MSC | ADC10SHT_3; // ADC ON, INTERRUPT EN y MULTIPLE SAMPLE & CONVERSION.
    ADC10CTL1 = INCH_4+CONSEQ_1; // TRABAJAR CON EL CANAL 4 hasta 0. Modo sequence of channels.
    ADC10DTC1 = 2; // Dos muestras, 2 transferencias.
    ADC10CTL0 |= ENC; // HABILITAR CONVERSION.
}
