
#include <msp430.h>

volatile unsigned short ADC_value;
#pragmavector=ADC10_VECTOR // ISR TIMER 0 CCR0
__interrupt void ADC_INTERRUPT (void){  
  ADC_value = ADC10MEM;    
}


#pragmavector=TIMER0_A0_VECTOR // ISR TIMER 0 CCR0
__interrupt void Timer_A (void){  
    TA0CCR0 += 1000;    //Cuenta 1000 veces mas desde el ultimo valor de TA0CCR0
    ADC10CTL0 |= ADC10SC;
}

int main(void){

    WDTCTL = WDTPW + WDTHOLD;                 // Stop watchdog timer

    ADC10AE |= (1<<1);      //p1.1  A1
    ADC10CTL0 |= ADC10ON + ADC10IE ;   //enciendo adc
    ADC10CTL1 = INCH_1;

    ADC10CTL0 |= ENC;
    

    TA0CTL = (2<<8)+(2<<4);       //Timer Init
    TA0CCR0 = TA0R + 1000;    //Cuenta 1000 veces sin importar el valor del registro del timer
    TA0CCTL0|=(1<<4); // local interrupt enable
    __bis_SR_register(GIE); // Global interrupt enable


    while(1){

  }
}
