/*
    (c) 2020 Microchip Technology Inc. and its subsidiaries. 
    
    Subject to your compliance with these terms, you may use Microchip software and any 
    derivatives exclusively with Microchip products. It is your responsibility to comply with third party 
    license terms applicable to your use of third party software (including open source software) that 
    may accompany Microchip software.
    
    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER 
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY 
    IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS 
    FOR A PARTICULAR PURPOSE.
    
    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND 
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP 
    HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO 
    THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL 
    CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT 
    OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS 
    SOFTWARE.
*/

/*disable Watchdog*/
#pragma config WDTE = OFF
/* Low voltage programming enabled, RE3 pin is MCLR */
#pragma config LVP = ON  

#include <xc.h>
#include <stdint.h>

static void CLK_init(void);
static void PORT_init(void);
static void ADCC_init(void);
static void ADCC_dischargeSampleCap(void);
static void INTERRUPT_init(void);
static void ADCC_startConversion(uint8_t channel);
static void ADCC_thresholdISR(void);
void __interrupt() INTERRUPT_InterruptManager(void);

uint16_t volatile errVal;

static void CLK_init(void)
{
    /* set HFINTOSC Oscillator */
    OSCCON1bits.NOSC = 6;
    /* clk divided by 4 */            
    OSCCON1bits.NDIV = 4;
    /* set HFFRQ to 1 MHz */
    OSCFRQbits.HFFRQ = 0;
}

static void PORT_init(void)
{
    /*set pin RA0 as analog*/
    ANSELAbits.ANSELA0 = 1;
    /*set pin RA0 as input*/
    TRISAbits.TRISA0 = 1;  
}


static void ADCC_init(void) 
{
    /* Enable the ADCC module */
    ADCON0bits.ADON = 1; 
    /* Enable continuous operation*/
    ADCON0bits.ADCONT = 1;
    /* result right justified */
    ADCON0bits.ADFM = 1;
    /*FOSC divided by 128*/
    ADCLKbits.ADCS = 63;    
    ADRPT = 16;
    
    /*clear status bits on overflow enabled (this setting prevents overflow 
    interrupts that  trigger the same interrupt as the threshold interrupt)*/
    ADCON2bits.ADACLR = 1;  
    /* Average mode */
    ADCON2bits.ADMD = 2;
    /*result is right shifted by 16*/
    ADCON2bits.ADCRS = 4; 
    /* mode: error bigger than upper threshold
         or lower than lower threshold*/
    ADCON3bits.ADTMD = 4;  
    /*error calculation method:  
      difference between the last result and the current result*/
    ADCON3bits.ADCALC = 0;
    /*upper threshold*/
    ADUTH = 35;
    /*lower threshold*/
    ADLTH = -35; 
 
    /* Clear the ADC Threshold interrupt flag */   
    PIR1bits.ADTIF = 0;  
    /* Enable ADCC threshold interrupt*/
    PIE1bits.ADTIE = 1;    
}

static void ADCC_dischargeSampleCap(void)
{
    /*channel number that connects to VSS*/
    ADPCH = 0x3C;
}

static void INTERRUPT_init(void)
{
    /* Enable global interrupts */
    INTCONbits.GIE = 1;
    /* Enable peripheral interrupts */    
    INTCONbits.PEIE = 1;   
}

static void ADCC_startConversion(uint8_t channel) 
{
    ADPCH = channel;
    /* Start the conversion */
    ADCON0bits.ADGO = 1;  
}

static void ADCC_thresholdISR(void)
{
    /*read the error*/
    errVal =  ((ADERRH << 8) + ADERRL);
    /*clear interrupt flag*/
    PIR1bits.ADTIF = 0;
}


void __interrupt() INTERRUPT_InterruptManager(void) 
{
    if (INTCONbits.PEIE) 
    {
        if ((PIE1bits.ADTIE) && (PIR1bits.ADTIF))
            
        {
            ADCC_thresholdISR();
        }
    }
}

void main(void)
{
    CLK_init();
    PORT_init();   
    ADCC_init();
    ADCC_dischargeSampleCap();
    INTERRUPT_init();
    
    /*channel number that connects to RA0*/    
    ADCC_startConversion(0x00);
    while (1) 
    {
        ;
    }
}
