/*******************************************************************************
Copyright 2016 Microchip Technology Inc. (www.microchip.com)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

To request to license the code under the MLA license (www.microchip.com/mla_license), 
please contact mla_licensing@microchip.com
*******************************************************************************/

/** INCLUDES *******************************************************/
#include <xc.h>
#include "system.h"

#include "usb_config.h"
#include "usb.h"
#include "usb_device.h"
#include "usb_device_cdc.h"

#include "LCD.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>
#include <math.h>

#include "RTCC.h"

#define FCY 16000000
#define BAUDRATE 9600
#define BRGVAL 416//((FCY/BAUDRATE)/4)-1

void APP_CDCInitialize(void);

void APP_CDCTasks(void);

void pwmInit(int duty){
	OC1CON1 = 0; // OC modul beállításainak törlése
	OC1CON2 = 0;
	OC1CON1bits.OCTSEL = 0; // TMR2-vel m?ködik
	OC1CON2bits.SYNCSEL = 0x0C; // szinkronizálás TMR2-vel
	PR2 = 1600 - 1; // TMR2 periódusa
	TMR2 = 0; // TMR2 törése
	OC1R = 0; // kitöltés
	OC1CON1bits.OCM = 6; // Edge Aligned PWM mode
	T2CONbits.TON = 0;
}


void setupUART(){
    // UART1
    U1MODE = 0; // UART1 alap állapotban történ? használata
    U1STA = 0; // UART1 státuszregiszter nullázása 
    U1BRG = BRGVAL; // Baudrate beállítása
    U1MODEbits.BRGH = 1; // osztás 
    U1MODEbits.UARTEN = 1; // UART engedélyezése
    U1STAbits.UTXEN = 1; // Küldés engedélyezése
    _U1RXIF = 0;
    _U1RXIE = 1;
    U1STAbits.URXISEL = 0b10;

}

//karakter küldése
char putUART1(char c)
{
    while ( U1STAbits.UTXBF); //csak ha TX buffer üres
        U1TXREG = c;
    return c;
}
//karakterfüzés küldése
void putsUART1(const char *s)
{
    while( *s) // *s == '\0' -ig
        putUART1( *s++); // karakter küldése
}

char getUART1( void){
    while (!U1STAbits.URXDA); // várakozás új karakter érkezésére
    return U1RXREG; // karakter kiolvasása
}
//Karaktertömb olvasása, adott hosszig, vagy Enter-ig
char *getsUART1( char *s, int len){
    char *p = s;
    do{
        *s = getUART1(); // várakozás karakter érkezésére
    putUART1(*s); // echo
        if ( *s=='\n') // \n kihagyása
            continue;
        if ( *s=='\r') // kilépés a ciklusból
            break; 
        s++; 
        len--;
    } while ( len > 1 ); // buffer végéig
    *s = '\0'; // \0 végz?dés? karakterlánc
return p;
}

void checkRxErrorUART1(void) {
uint8_t c;
//hiba figyelése és törlése
    if (U1STAbits.PERR) 
    {
        c = U1RXREG; //parítás hiba törlése
    }
    if (U1STAbits.FERR)
    {
        c = U1RXREG; //keret hiba törlése
    }
    if (U1STAbits.OERR)
    {
        U1STAbits.OERR = 0; //hiba törlése
    }
}

void playFrek(uint16_t frek, uint8_t ms){
	PR2 = ((double)FCY/frek-1);			//given frequency
    int shift = (log10(FCY/frek)/log10(2) - 1);
	OC1R = 1 << shift;	//50%
	TMR2 = 0;
	T2CONbits.TON = 1;	//start
	DELAY_MS(ms);
	OC1R = 0;
}

void csiip(){
    playFrek(1318, 100);
    DELAY_MS(100);
    playFrek(1244, 100);
    DELAY_MS(100);
    playFrek(1318, 100);
    DELAY_MS(100);
    playFrek(1244, 100);
    DELAY_MS(100);
    playFrek(1318, 100);
    DELAY_MS(100);
    playFrek(987, 100);
    DELAY_MS(100);
    playFrek(1244, 100);
    DELAY_MS(100);
    playFrek(1046, 100);
    DELAY_MS(100);
    playFrek(880, 200);
}

/** VARIABLES ******************************************************/

RTCTime ido = {0,0,0,0,0,0,0};
RTCTime currenttime = {0,0,0,0,0,0,0};
int lastsec = 0;

static uint8_t readBuffer[CDC_DATA_OUT_EP_SIZE];
static uint8_t writeBuffer[CDC_DATA_IN_EP_SIZE];
char LCD[80];

uint8_t chks[2] = {129};
uint8_t chkscalc[2] = {0};

char c[14] = {129};	//unused ASCII, means not overwritten
uint8_t number[10];

MAIN_RETURN main(void)
{
    //Orajel forras beallitasa
    // PLL kimenete
    CLKDIVbits.CPDIV = 0;    // 1:1 32MHz
    // megvarjuk amig a PLL modul vegez
    while(!OSCCONbits.LOCK) Nop();
	
    // szoftveres WDT kikapcsolasa
    RCONbits.SWDTEN = 0;
	
	// Orakvarc engedelyezese
	__builtin_write_OSCCONL(OSCCON | 0x02);

	// Periferia - lab osszerendeles PPS (pp.135)
	__builtin_write_OSCCONL(OSCCON & 0xbf);  //PPSUnLock;
	RPOR8bits.RP17R = 18;
    RPOR11bits.RP23R = 3; //62-es láb TX
    RPINR18bits.U1RXR = 24; //61-es láb RX
	__builtin_write_OSCCONL(OSCCON | 0x40); //PPSLock;
    
    SYSTEM_Initialize(SYSTEM_STATE_USB_START);
    USBDeviceInit();
    USBDeviceAttach();

    // Labak iranya	
	_TRISG6 = 0;	// LED1 kimenet
	_TRISG7 = 0;	// LED2 kimenet
	_TRISG8 = 0;	// LED3 kimenet
    _TRISG9 = 0;	// LED4 kimenet
    _TRISD15 = 0;	// LEDR kimenet
    _TRISF4 = 0;	// LEDG kimenet
    _TRISF5 = 0;	// LEDB kimenet
	_TRISC1 = 1;	// SW1 bemenet
	_TRISC3 = 1;	// SW2 bemenet 
	_TRISE8 = 1;	// SW3 bemenet
    _TRISE9 = 1;	// SW4 bemenet
    
    lcdInit();                  //LCD modul inicializálása
    lcdLoadHuChars();                //Magyar karakterek betöltése
    pwmInit(512);
    setupUART();
    
    PR1 = F_SEC_OSC / 4;    //250ms
    T1CON = 0x8002;         //external, 1:1, on
    _T1IE = 1;
    
    //setRTCTime(ido);
    
    playFrek(440,50);   //music
    DELAY_MS(10);
    playFrek(523,50);
    DELAY_MS(10);
    playFrek(329,50);
    DELAY_MS(10);
    
    while(1)
    {
        APP_CDCTasks();
        
		
        LED1 = U1STAbits.URXDA;
        LED2 = U1STAbits.PERR;
        LED3 = U1STAbits.FERR;
        LED4 = U1STAbits.OERR;
        
        
        if(SW1){
            DELAY_MS(20);
            while(SW1) Nop();
            DELAY_MS(20);
            lcdClear();
        }
        
        currenttime = getRTCTime();
//        sprintf(LCD,"%02x:%02x:%02x",currenttime.hour, currenttime.minute, currenttime.second);
//        lcdClear();
//        lcdPutStr(LCD);
        
        if(currenttime.second == 0x59){
            currenttime.second = 0x05;
            setRTCTime(currenttime);
        }
        checkRxErrorUART1();
        DELAY_MS(500);

    }//end while
}//end main



// ISR

void __attribute__((interrupt(auto_psv))) _U1RXInterrupt(){
    LEDR = 1;
    
    currenttime = getRTCTime();
    if(currenttime.second > 0x01)
    {
    
	if(U1STAbits.URXDA){//
			int i = 0;
			for(i; i < strlen(c);i++)
				if(c[i]!=-127) break;
			
            
            if(c[0] == -127)
                getsUART1(c,15);
            
            for(int i = 0; i < 10; i++){
                if(c[i + 1] < ((int)'9' + 1))   //0 .. 9
                    number[i] = c[i + 1] - '0';
                else if(c[i + 1] < ((int)'F' + 1))   //A .. F
                    number[i] = c[i + 1] - 'A' + 10;
                else if(c[i + 1] < ((int)'f' + 1))   //a .. f
                    number[i] = c[i + 1] - 'a' + 10;
            }
            
            for(int i = 10; i < 12; i++){
                if(c[i + 1] < ((int)'9' + 1))   //0 .. 9
                    chks[i-10] = c[i + 1] - '0';
                else if(c[i + 1] < ((int)'F' + 1))   //A .. F
                    chks[i-10] = c[i + 1] - 'A' + 10;
                else if(c[i + 1] < ((int)'f' + 1))   //a .. f
                    chks[i-10] = c[i + 1] - 'a' + 10;
            }
            
            for(int i = 0; i < 5; i++){
                chkscalc[0] = chkscalc[0] ^ number[2*i];
                chkscalc[1] = chkscalc[1] ^ number[2*i + 1];
            }
            
            if((chkscalc[0] == chks[0]) && (chkscalc[1] == chks[1])){
                LEDG = 1;
                
                memset(writeBuffer,0,sizeof(writeBuffer));
                
                char numberToSend[10];
                
                for(int i = 0; i < 10; i++)
                    numberToSend[i] = c[i+1];
                
                strcat(numberToSend,"\r\n");
                strcpy((char*)writeBuffer,numberToSend);
                
                putUSBUSART(writeBuffer,strlen((char*)writeBuffer));
                CDCTxService();
                csiip();
                setRTCTime(ido);
                LEDG = 0;
            }
            
            
	}
	
    DELAY_MS(50);
    
    }
    _U1RXIF = 0;
     LEDR = 0;
}

void __attribute__((interrupt(auto_psv))) _T1Interrupt(){
    _T1IF = 0;
}

/*********************************************************************
* Function: void APP_CDCInitialize(void);
*
* Overview: Initializes the code
*
* PreCondition: None
*
* Input: None
*
* Output: None
*
********************************************************************/
void APP_CDCInitialize()
{   
    line_coding.bCharFormat = 0;
    line_coding.bDataBits = 8;
    line_coding.bParityType = 0;
    line_coding.dwDTERate = 9600;
}

/*********************************************************************
* Function: void APP_CDCTasks(void);
*
* Overview: Keeps the demo running.
*
* PreCondition: The demo should have been initialized and started via
*   the APP_CDCInitialize()
*
* Input: None
*
* Output: None
*
********************************************************************/
void APP_CDCTasks()
{
    
    /* If the USB device isn't configured yet, we can't really do anything
     * else since we don't have a host to talk to.  So jump back to the
     * top of the while loop. */
    if( USBGetDeviceState() < CONFIGURED_STATE )
    {
        return;
    }

    /* If we are currently suspended, then we need to see if we need to
     * issue a remote wakeup.  In either case, we shouldn't process any
     * keyboard commands since we aren't currently communicating to the host
     * thus just continue back to the start of the while loop. */
    if( USBIsDeviceSuspended()== true )
    {
        return;
    }
        

    /* Check to see if there is a transmission in progress, if there isn't, then
     * we can see about performing an echo response to data received.
     */
     if( USBUSARTIsTxTrfReady() == true)
    {
        uint8_t numBytesRead;

        numBytesRead = getsUSBUSART(readBuffer, sizeof(readBuffer));
        if (numBytesRead>0)
        {
            memset(writeBuffer,0,sizeof(writeBuffer));
     
            switch(readBuffer[0])
            {
                case 0x0A:
                case 0x0D:
                    break;
                case 'R':
                    LEDR = 1;
                    break;
                case 'r':
                    LEDR = 0;
                    break;
                case '?':
                    strcpy((char *)writeBuffer,"uMogi2\r\n");
                    break;
                default:
                    strcpy((char *)writeBuffer,"?\r\n");
                    break;
            }
            putsUSBUSART(writeBuffer);
        }
    }
    
    CDCTxService();
}