/* 
 * File:   main.c
 * Author: larsg
 *
 * Created on August 19, 2016, 2:24 PM
 */

#ifndef __XTAL_FREQ
#define _XTAL_FREQ 1000000
#endif

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <xc.h>

void initializePIC(void){
//      Configure Oscillator:
//    	SPLLEN=b'0' (bit 7)     //4xPLL is disabled
//    	IRCF=b'1011' (bit 6-3)  //Internal Oscillator Frequency is 1MHz 
//    	*bit 2 unimplemented*
//    	SCS=b'00' (bit 1-0)     //System Clock determined by FOSC<2:0> in Config Word 1
    OSCCON = 0b01011000;
    
    //Enable interrupts
    INTCONbits.GIE = 1; // global interrupt enabled
    INTCONbits.PEIE = 1; // peripheral interrupt enabled
    INTCONbits.IOCIE = 1; // enable interrupt on change global
    
//      Configure AD Conversion 0:
//      *bit 7 unimplemented*
//      CHS=b'00000' (bit 6-2)  //Select AN0/RA0 as Input Channel
//      GO/DONE=b'0' (bit 1)    //Do not start conversion just yet
//      ADON=b'1' (bit 0)       //Enable ADC circuitry
    ADCON0 = 0b00000001; 
//      Configure AD Conversion 1:
//      ADFM=b'1' (bit 7)       //Select right justified Format
//      ADCS=b'000' (bit 6-4)   //Select Fosc/2 speed
//      *bit 3 unimplemented*
//      ADNREF=b'0' (bit 2)     //Negative Voltage Reference is connected to Vss
//      ADPREF=b'00' (bit 1-0)  //Positive Voltage Reference is connected to Vdd
    ADCON1 = 0b10000000;
    
    PIR1bits.ADIF = 0; //Clear ADC interrupt flag
    PIE1bits.ADIE = 1; //Enable ADC interrupts
    
    //RXTX
    APFCON0bits.RXDTSEL = 1; // Set RC5 as RX
    APFCON0bits.TXCKSEL = 1; // Set RC4 as TX
    
    BRG16 = 0; 
    TXSTAbits.BRGH = 1; // Enable High-Speed mode
    SPBRG = 5; // For final baud rate of 10417 bit/s
    
    TXSTAbits.SYNC = 0; // Configure EUSART  for Asynchronous operation (UART mode)
    RCSTAbits.SPEN = 1; // Enables EUSART and configures RXTX pins as output
    TXSTAbits.TXEN = 1; // Enable EUSART transmitter circuitry
    RCSTAbits.CREN = 1; // Enable EUSART receiver circuitry
    
    PIE1bits.RCIE = 1; // Enable EUSART receive interrupt
    IOCANbits.IOCAN2 = 1; // When SW1 is pressed, enter the ISR
}

void UART_Write(char data)
{
    while(!TRMT);
    TXREG = data;
}

void UART_Write_Text(char *text)
{
    int count;
    for(count = 0; text[count]!='\0'; count++)
    {
        UART_Write(text[count]);
    }
}

//Interrupt Service Routine
void interrupt ISR(void) {
    if (IOCAF) {                                //SW1 was just pressed                     //must clear the flag in software
        __delay_ms(5);                          //debounce by waiting and seeing if still held down
        if (!PORTAbits.RA2) {
            LATCbits.LATC0 ^= 1;
            UART_Write_Text("Hello, World!\n");
        }
        IOCAF = 0;  
    }
    
    if(ADIF){
        char str[15];
        sprintf(str," %d", ADRES);
        UART_Write_Text(str);
        //LATCbits.LATC1 ^= 1;
        GO = 1;
        ADIF = 0;
    }
    
    if(RCIF){
        char x = RCREG; // Read from FIFO buffer so it doesn't trigger overrun error
        LATCbits.LATC2^= 1;
        RCIF = 0;
    }
}

/*
 * 
 */
int main(int argc, char** argv) {
    TRISCbits.TRISC0 = 0;
    TRISCbits.TRISC1 = 0;
    TRISCbits.TRISC2 = 0;
    
    LATC = 0;
        
    //Temperatuursensor
    TRISAbits.TRISA0 = 1;   //input
    ANSELAbits.ANSA0 = 1;   //analog
    
    //SW1
    TRISAbits.TRISA2 = 1;   //input
    ANSELAbits.ANSA2 = 0;   //NOT analog
    
    initializePIC();
    
    ADCON0bits.GO = 1; //Start AD Conversion
    while (1);
    
    return (EXIT_SUCCESS);
}