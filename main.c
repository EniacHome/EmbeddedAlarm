/* 
 * File:   main.c
 * Organisation: Eniac Development group
 * Authors: Lars Gardien, Dennis Smith & Nick de Visser 
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

void initializePIC(void);
void UART_Write_Text(char *text);
void UART_Write(char date);

int main(int argc, char** argv) {    
    initializePIC();
    
    ADCON0bits.GO = 1;  //Start AD Conversion
    //asm("SLEEP");       //Enter sleep mode
    while (1);          //Infinite loop
    
    return (EXIT_SUCCESS);
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

void initializePIC(void){
//      Configure Oscillator:
//    	SPLLEN=b'0' (bit 7)     //4xPLL is disabled
//    	IRCF=b'1011' (bit 6-3)  //Internal Oscillator Frequency is 1MHz 
//    	*bit 2 unimplemented*
//    	SCS=b'00' (bit 1-0)     //System Clock determined by FOSC<2:0> in Config Word 1
    OSCCON = 0b01011000;
    
    TRISCbits.TRISC0 = 0;
    TRISCbits.TRISC1 = 0;
    TRISCbits.TRISC2 = 0;
    
    LATC = 0;
        
    //Temperature sensor
    TRISAbits.TRISA0 = 1;   //input
    ANSELAbits.ANSA0 = 1;   //analog
    
    //SW1
    TRISAbits.TRISA2 = 1;   //input
    ANSELAbits.ANSA2 = 0;   //NOT analog
    
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
    
    /*bit 7 : unimplemented*/
    /*bit 6-3 : */
    T2CON = 0b01111111; // ((256 * 8uS) * prescaler ) * postscaler = 5.198 seconds
    
        //Setup TIMER4
    T4CON = 0b00100000;         //Timer4 delay. Prescaler 1:1, Postscaler 1:5
                                //((8uS * 256) * Prescaler) * Postscaler = ((8uS * 256) * 1) * 5 =~ 0.01024 seconds    
            
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
    //IOCANbits.IOCAN2 = 1; // When SW1 is pressed, enter the ISR
    
    //Interrupt on change negative edge triggerd interrupts
    IOCANbits.IOCAN1 = 1; //Enable the Interrupt_On_Change bit for the smoke detector
    //IOCANbits.IOCAN3 = 1; //Enable the Interrupt_On_Change bit for the first contact sensor
    //IOCANbits.IOCAN5 = 1; //Enable the Interrupt_On_Change bit for the second contact sensor
    IOCANbits.IOCAN2 = 1;   //Enable the Interrupt_On_Change bit for the infrared sensor
    
    
    //Interrupt on change positive edge triggered interrupts
    IOCAPbits.IOCAP3 = 1;   //Enable the Interrupt_On_Change bit for the first contact sensor
    IOCAPbits.IOCAP5 = 1;   //Enable the Interrupt_On_Change bit for the second contact sensor
    IOCAPbits.IOCAP2 = 1;   //Enable the Interrupt_On_Change bit for the infrared sensor
    
    PIE1bits.TMR2IE = 1;    //Enable the TIMER2 Interrupt_Enable bit
    PIE3bits.TMR4IE = 1;    //Enable TIMER4 overflow interrupt enable bit
    INTCONbits.GIE = 1;     //Enable the Global_Interrupt_Enable bit
    
}

//Interrupt Service Routine
void interrupt ISR(void) {    
    //Analog/Digital conversion Interrupt flag handler
    //Triggers when a AD conversion is finished
    if(ADIF){
        char str[15];
        sprintf(str," %d", ADRES);
        UART_Write_Text(str);                                           //Call the procedure UART_Write_Text with str as parameter
        ADCON0bits.GO = 0;                                              //Toggle the Analog to Digital conversion module GO bit 
        ADIF = 0;                                                       //Clear the Analog to Digital conversion interrupt flag
    }
    
    //Received data interrupt handler
    //Triggers when data has been received
    if(PIR1bits.RCIF){
        char x = RCREG;                                                 //Read from FIFO buffer so it doesn't trigger overrun error  
        PIR1bits.RCIF = 0;                                              //Clear the flag
    }
    
    //TIMER2 overflow interrupt handler
    if(PIR1bits.TMR2IF){
        ADCON0bits.GO = 1;                                              //Toggle the Analog to Digital conversion module GO bit. Start the AD conversion
        PIR1bits.TMR2IF = 0;                                            //Clear the TIMER2 Interrupt flag
    }
    
    //Smoke detector interrupt handler
    if(IOCAFbits.IOCAF1){                   
        UART_Write_Text("Smoke has been detected.\n");                   //Send the message to the UART_Write_Text procedure
        IOCAFbits.IOCAF1 = 0;                                           //Clear the Interrupt_On_Change bit 1 flag
    }
    
    //Contact sensor 1 interrupt handler
    if(IOCAFbits.IOCAF3){
        UART_Write_Text("Contact sensor 1 has been disconnected.\n");    //Send the message to the UART_Write_Text procedure
        IOCAFbits.IOCAF3 = 0;                                           //Clear the Interrupt_On_Change bit 3 flag 
    }
    
    //Contact sensor 2 interrupt handler
    if(IOCAFbits.IOCAF5){
        UART_Write_Text("Contact sensor 1 had been disconnected.\n");   //Send the message to the UART_Write_Text procedure
        IOCAFbits.IOCAF5 = 0;                                           //Clear the Interrupt_On_Change bit 3 flag 
    }
    
    //Infrared sensor interrupt handler
    if(IOCAFbits.IOCAF2){
        T4CONbits.TMR4ON = 1;                                           //Enable TIMER 4        
        IOCAFbits.IOCAF2 = 0;                                           //Clear the Interrupt_On_Change bit 2 flag
    }
    
    //Used as debounce
    if(TMR4IF){
        T4CONbits.TMR4ON = 0;                                           //Disable TIMER4
        
        if(!PORTAbits.RA2){
            LATCbits.LATC0 = 1;                                        //Enable LACTC0 (the alarm light)
        }
        else{
            LATCbits.LATC0 = 0;                                        //Disable LACTC0 (the alarm light)
        }
        TMR4IF = 0;                                                     //Clear the TIMER4 Interrupt flag
    }
    
    //asm("SLEEP");                                                       //Enter sleep mode
}