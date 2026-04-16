#include <msp430.h> 
//EL MECANISMO DEL LIMPIA PARABRISAS ES IDEAL PARA QUE FUNCIONE CON UN MOTOR GIRANDO EN UNA SOLA DIRECCIÓN
#define T_us 1000 // Constante, indica el periodo de la se al PWM

void clk_calib(void) {
    //1Mhz
    if (CALBC1_1MHZ==0xFF) // If calibration constant erased
        while(1); // do not load, trap CPU!!
    DCOCTL = 0; // Select lowest DCOx and MODx settings
    BCSCTL1 = CALBC1_1MHZ; // Set range
    DCOCTL = CALDCO_1MHZ;
}

//volatile long ADC_value;
unsigned short ADC_values[2]; // Muestra de canal mas grande al mas pequeño.

// ISR TIMER A1 - PWM Para motor
#pragma vector = TIMER1_A0_VECTOR;
__interrupt void TIMER1_A0_ISR (void) {

  if (ADC_values[1] != 0){
    if(ADC_values[1] > 0x3FF){
      //manera continua
      P2OUT |= BIT0;

    }else{ 

      
      if(ADC_values[1] > 0x100){
        // proporcional
        TON_us = ((unsigned long) ADC_values[1] * T_us)/0x400;
        TON_us = (TON_us<=T_us)? TON_us : T_us/2;
        TOFF_us = T_us - TON_us;

        P2OUT ^= BIT0; // Toggle a P2.0
        if (estado) TA0CCR0 += TOFF_us; // Actualizacion para siguiente interrupcion (Tiempo en bajo).
        else TA0CCR0 += TON_us;
        estado = !estado; // Conmutamos estado - se asigna booleano, solo puede ser 0 o 1

      } else{
        //velocidad equivalente a entrada 0x200 pero se activa proporcionalmente de 0x01 a 0xFF
        TON_us = ((unsigned long) ADC_values[1] * T_us)/0x400;
        TON_us = (TON_us<=T_us)? TON_us : T_us/2;
        TOFF_us = T_us - TON_us;

        P2OUT ^= BIT0; // Toggle a P2.0
        if (estado) TA0CCR0 += TOFF_us; // Actualizacion para siguiente interrupcion (Tiempo en bajo).
        else TA0CCR0 += TON_us;
        estado = !estado; // Conmutamos estado - se asigna booleano, solo puede ser 0 o 1
      } 
  
      else{
        if (ADC_values[0] > 0x47){
          //se activa el motor a una velocidad predeterminada
          TON_us = ((unsigned long) 0x200 * T_us)/0x400;
          TON_us = (TON_us<=T_us)? TON_us : T_us/2;
          TOFF_us = T_us - TON_us;

          P2OUT ^= BIT0; // Toggle a P2.0
          if (estado) TA0CCR0 += TOFF_us; // Actualizacion para siguiente interrupcion (Tiempo en bajo).
          else TA0CCR0 += TON_us;
          estado = !estado; // Conmutamos estado - se asigna booleano, solo puede ser 0 o 1

        }
      }
    }
}

// ISR TIMER A0 - PWM para tiempo de converción
#pragma vector=TIMER0_A0_VECTOR
__interrupt void ISR_TA0_CCR0 (void){
    TA0CCR0=TAR+1000;
    ADC10CTL1=ADC_values[i];
    ADC10CTL0|=(1<<0);          //Start of conversion
    ADC10SA = (unsigned int)&ADC_values;   // Data buffer start
}


int main(void){
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    clk_calib(); // Calibracion del clk

    P2DIR |= BIT0; // P2.0 como salida
    P2OUT |= BIT0; // Estado inicial HIGH en P2.0

    // Configuracion ADC10 - Multiples canales un muestreo secuencial:
    //A1 es la perilla, A0 es el sensor
    ADC10AE0 |= (1<<1) | (1<<0); // P1.1 (A1) y P1.0 (A0) como entradas analogicas
    ADC10CTL0 |= ADC10ON  | MSC; // ADC ON, INTERRUPT EN//| ADC10IE... y Multiple Sample and Conversion (Conversiones encadenadas)
    ADC10CTL1 |= INCH_1 | CONSEQ_1; // TRABAJAR DESDE EL CANAL 1 HASTA EL CANAL 0.
    ADC10DTC1 = 2; // Data Transfer Controller, guarda n veces el resultado.
    ADC10SA = (unsigned int)ADC_values; // Se carga la direccion del arreglo que contendra a las lecturas/muestras.

    ADC10CTL0 |= ENC; // HABILITAR CONVERSION.

    //Configuracion Timer A0:
    TA0CTL = (2<<8)|(2<<4); // SMCLK y MODO CONTINUO
    TA0CCR0 = TA0R + 10000; // + 10 ms
    TA0CCTL0 |= (1<<4); // LOCAL INTERRUPT ENABLE

    //Configuracion Timer A1:
    TA1CTL = (2<<8)|(2<<4); // SMCLK y MODO CONTINUO
    TA1CCR0 = TA1R + 10000; // + 10 ms
    TA1CCTL0 |= (1<<4); // LOCAL INTERRUPT ENABLE

        // Se configura interrupci n inicial en el tiempo deseado en alto
    P2OUT |= BIT0; // Estado inicial HIGH en P2.0
    estado = 1;
    TA0CCR0= TA0R + TON_us; // Tiempo en alto inicial
    
    _bis_SR_register(GIE); // GLOBAL INTERRUPT ENABLE.

    while (1);

    return 0;
}