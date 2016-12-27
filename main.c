/* 
 * File:   main.c
 * Organisation: Eniac Development group
 * Authors: Lars Gardien, Dennis Smith & Nick de Visser 
 *
 * Created on August 19, 2016, 2:24 PM
 */


#ifndef __XTAL_FREQ
#define _XTAL_FREQ 500000
#endif

//Port A 1 - 5 will be used as sensors
#define SENSOR_COUNT 5

#define SWITCH PORTAbits.RA2

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <xc.h>

void initializePIC(void);
void initializeSensors(void);
void autoBaud(void);
char get_port_pin(unsigned char *port, unsigned char pin);
void default_handler(void);
void switch_handler(void);
void UART_Write_Text(char *text);
void UART_Write(char date);

typedef struct debounce_t {
    unsigned char shouldDebounce     : 1;
    unsigned char previousState      : 1;
    void (*handler)(void);
} Debounce;

typedef struct sensor_t{
    unsigned char index;
    unsigned char *port;
    unsigned char pin;
    Debounce debounce;
} Sensor;

Sensor sensors[SENSOR_COUNT];
char shouldAutoBaud = 0;

int main(void) {    
    initializePIC();            //Initialize the PIC
    initializeSensors();
    
     while(1){
        if(shouldAutoBaud){ //AutoBaud detection takes time, so do it over here and not in ISR
            autoBaud();                 //Initiate AutoBaud
            UART_Write_Text("V");       // Send initialization and AutoBaud success
            PIE1bits.RCIE = 1;          // Enable EUSART receive 
            PIE1bits.TXIE = 1;          // Enable EUSART transmit interrupt
            shouldAutoBaud = 0;         // AutoBaud succeeded so no longer needed
        }
        
        //When one of the following errors occured: recover by resetting
        if(RCSTAbits.OERR)
        {
            RCSTAbits.SPEN = 0;
            RCSTAbits.SPEN = 1;
        }

        if(RCSTAbits.FERR)
        {
            RCSTAbits.SPEN = 0;
            RCSTAbits.SPEN = 1;
        } 
    }
    
    return (EXIT_SUCCESS);      
}

char get_port_pin(unsigned char *port, unsigned char pin){
    return (*port & (1U << pin));
}

void default_handler(void){
    UART_Write_Text("xy");
}

void switch_handler(void){
    shouldAutoBaud = 1;
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
//  Configure Oscillator:
//  SPLLEN=b'0' (bit 7)         //4xPLL is disabled
//  IRCF=b'0111' (bit 6-3)      //Internal Oscillator Frequency is 500KHz 
//  *bit 2 unimplemented*
//  SCS=b'00' (bit 1-0)         //System Clock determined by FOSC<2:0> in Config Word 1
    OSCCON = 0b00111000;
        
//      Configure Interrupts:
//      GIE=b'1' (bit 7)        //Enable global Interrupts
//      PEIE=b'1' (bit 6)       //Enable Peripheral Interrupts
//      TMR0IE=b'0' (bit 5)     //Disable Timer 0 Interrupts
//      INTE b'0' (bit 4)       //Disable INT external Interrupts
//      IOCIE=b'1' (bit 3)      //Enable Interrupt-on-Change Interrupts
//      TMR0IF=b'0' (bit 2)     //Clear Timer 0 Interrupt Flag
//      INTF=b'0' (bit 1)       //Clear INT external Interrupt flag
//      IOCIF=b'0' (bit 0)      //Clear Interrupt-on-Change Interrupt flag
    INTCON = 0b11001000;
    
    //LEDs are output
    TRISCbits.TRISC0 = 0;
    TRISCbits.TRISC1 = 0;
    TRISCbits.TRISC2 = 0;
    TRISCbits.TRISC3 = 0;
    LATC = 0;               //Clear all LEDs
      
    //Temperature sensor
    TRISAbits.TRISA0 = 1;   //input
    ANSELAbits.ANSA0 = 1;   //analog
    
    //Smoke sensor
    TRISAbits.TRISA1 = 1;   //input
    ANSELAbits.ANSA1 = 0;   //digital
    
    //SW1 (sync baud)
    TRISAbits.TRISA2 = 1;   //input
    ANSELAbits.ANSA2 = 0;   //digital
    
    //Contact sensor
    TRISAbits.TRISA3 = 1;   //input
    
    //Infrared sensor
    TRISAbits.TRISA4 = 1;   //input
    ANSELAbits.ANSA4 = 0;   //digital
    
    //Contact sensor
    TRISAbits.TRISA5 = 1;   //input
    
    //Interrupt on change negative edge triggered interrupts
    IOCANbits.IOCAN1 = 1;   //Enable the Interrupt_On_Change for the smoke detector
    IOCANbits.IOCAN2 = 1;   //Enable the Interrupt_On_Change for the switch
    IOCANbits.IOCAN4 = 1;   //Enable the Interrupt_On_Change for the infrared sensor
    
    //Interrupt on change positive edge triggered interrupts
    IOCAPbits.IOCAP3 = 1;   //Enable the Interrupt_On_Change for the first contact sensor
    IOCAPbits.IOCAP5 = 1;   //Enable the Interrupt_On_Change for the second contact sensor  
    
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
    
//  Timer 2 delay (AD timeout)
    T2CON = 0b01111111;     // ((256 * 8uS) * prescaler ) * postscaler = 5.198 seconds
    PIE1bits.TMR2IE = 1;    //Enable TIMER2 overflow Interrupt
    
//  Configure Timer6 (Switch debounce):
//  *bit 7 unimplemented*
//  T6OUTPS=b'0100' (bit 6-3)   //1:5 Postscaler value
//  TMR6ON=b'0'                 //Do not start timer just yet
//  T6CKPS=b'00'                //1:1 Prescalar value
    T6CON = 0b00100000;
    PR6 = 125;
    PIE3bits.TMR6IE = 1;        //Enable Timer 6 Interrupts
    
    //RXTX
    APFCON0bits.RXDTSEL = 1; // Set RC5 as RX
    APFCON0bits.TXCKSEL = 1; // Set RC4 as TX
    
    BRG16 = 1; 
    TXSTAbits.BRGH = 1; // Enable High-Speed mode
    SPBRG = 12;         // For final baud rate of 9615 bit/s
    
    TXSTAbits.SYNC = 0; // Configure EUSART  for Asynchronous operation (UART mode)
    RCSTAbits.SPEN = 1; // Enables EUSART and configures RXTX pins as output
    TXSTAbits.TXEN = 1; // Enable EUSART transmitter circuitry
    RCSTAbits.CREN = 1; // Enable EUSART receiver circuitry
    
    PIE1bits.RCIE = 1; // Enable EUSART receive interrupt
}

void initializeSensors(void){
    for(int i = 0; i < SENSOR_COUNT; ++i){
        sensors[i].index = i;
        sensors[i].port = &PORTA;
        sensors[i].pin = i + 1;
        sensors[i].debounce.handler = default_handler;
    }
    
    //PORTA 2 - Switch used for AutoBaud
    sensors[1].debounce.handler = switch_handler;
}

void autoBaud(void)
{
    char discard;                   //Used to discard RCREG data
    int index = 0;                  //Used to iterate different BRGH and BRG16 settings
    int sync_count;                 //Used to indicate how much 'U' chars were received
    
try:
    //NOTE: Some clock speeds will need different settings for BRG16 and BRGH, iterate them
    switch(index) //Get the correct BRGH and BRG16 settings for index
    {
        case 0:
            BAUDCONbits.BRG16 = 0;
            TXSTAbits.BRGH = 0;
            break;
        case 1:
            BAUDCONbits.BRG16 = 0;
            TXSTAbits.BRGH = 1;
            break;
        case 2:
            BAUDCONbits.BRG16 = 1;
            TXSTAbits.BRGH = 0;
            break;
        case 3:
            BAUDCONbits.BRG16 = 1;
            TXSTAbits.BRGH = 1;
            break;
    }

    BAUDCONbits.ABDEN = 1;  //Start AutoBaud sequence
    while(!PIR1bits.RCIF);  //Wait for AutoBaud to complete
    discard = RCREG;        //discard the read char
    
    if(SPBRG != 0)          //Null check for the rare occasion where SPBRG=0 to avoid wraparound
    {
        --SPBRG;            // AutoBaud starts counting at 1 so decrement
    }
    
    //Handle errors
    if(BAUDCONbits.ABDOVF)  //SPBRG overflow while AutoBauding 
    {
        BAUDCONbits.ABDOVF = 0; //Clear the flag
        goto try;               //Retry AutoBaud
    }
    if(!BAUDCONbits.BRG16 && SPBRGH) //Wanted to use 8 bit mode, but overflowed into 16 bit
    {
        goto try;               //Retry AutoBaud
    }
    
    //Verify the AutoBaud by trying to receive 2 more 'U' s
    for (sync_count=0; sync_count < 2; ++sync_count)
    {
        while(!PIR1bits.RCIF);  // waiting for USART incoming byte 'U'
        if(RCREG != 'U')        // If it wasn't a 'U'
        {
            ++index;            // Retry with different setting
            if(index > 3)   
                index = 0;      //Setting wraparound
            goto try;           //AutoBaud was not successful, retry
        }
    }
}

//Interrupt Service Routine
void interrupt ISR(void) {    
    int shouldDebounce = 0;
    
    //Used as debounce
    if(PIR3bits.TMR6IF){
        T6CONbits.TMR6ON = 0;                                           //Disable TIMER6
        
        for(int i = 0; i < SENSOR_COUNT; ++i){                          //Iterate all sensors
            Debounce debounce = sensors[i].debounce;
            if(debounce.shouldDebounce &&                               //If sensor should debounce and debounce successful
                    debounce.previousState == get_port_pin(sensors[i].port, sensors[i].pin)){
                debounce.handler();                                     //Handle 
                debounce.shouldDebounce = 0;                            //Mark as handled
            }
        }
        
        TMR6IF = 0;                                                     //Clear the TIMER4 Interrupt flag
    }
    
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
    
    //TIMER2 overflow interrupt handler (AD timeout)
    if(PIR1bits.TMR2IF){
        ADCON0bits.GO = 1;                                              //Toggle the Analog to Digital conversion module GO bit. Start the AD conversion
        PIR1bits.TMR2IF = 0;                                            //Clear the TIMER2 Interrupt flag
    }
    
    //Smoke detector interrupt handler
    if(IOCAFbits.IOCAF1){                   
        shouldDebounce = 1;
        sensors[0].debounce.shouldDebounce = 1;                         //This needs to be processed after debounce
        sensors[0].debounce.previousState = PORTAbits.RA1;              //Save previous value
        IOCAFbits.IOCAF1 = 0;                                           //Clear the Interrupt_On_Change flag
    }
    
    //Switch(sync baud) interrupt handler
    if(IOCAFbits.IOCAF2){  
        shouldDebounce = 1;
        sensors[1].debounce.shouldDebounce = 1;
        sensors[1].debounce.previousState = PORTAbits.RA2;
        IOCAFbits.IOCAF2 = 0;                                           //Clear the Interrupt_On_Change flag
    }
    
    //Contact sensor 1 interrupt handler
    if(IOCAFbits.IOCAF3){
        shouldDebounce = 1;
        sensors[2].debounce.shouldDebounce = 1;                         //This needs to be processed after debounce
        sensors[2].debounce.previousState = PORTAbits.RA3;              //Save previous value
        IOCAFbits.IOCAF3 = 0;                                           //Clear the Interrupt_On_Change flag 
    }
    
    //Infrared sensor interrupt handler
    if(IOCAFbits.IOCAF4){   
        shouldDebounce = 1;
        sensors[3].debounce.shouldDebounce = 1;                         //This needs to be processed after debounce
        sensors[3].debounce.previousState = PORTAbits.RA4;              //Save previous value
        IOCAFbits.IOCAF4 = 0;                                           //Clear the Interrupt_On_Change flag
    }
    
    //Contact sensor 2 interrupt handler
    if(IOCAFbits.IOCAF5){
        shouldDebounce = 1;
        sensors[4].debounce.shouldDebounce = 1;                         //This needs to be processed after debounce
        sensors[4].debounce.previousState = PORTAbits.RA5;              //Save previous value
        IOCAFbits.IOCAF5 = 0;                                           //Clear the Interrupt_On_Change flag 
    }
    
    if(shouldDebounce){
        T6CONbits.TMR6ON = 1;   //Debounce
    }
}