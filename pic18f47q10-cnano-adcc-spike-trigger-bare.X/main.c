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

#pragma config WDTE = OFF     /*disable Watchdog*/
#pragma config LVP = ON  /* Low voltage programming enabled, RE3 pin is MCLR */

#include <xc.h>
#include <stdint.h>

/*channel number that connects to VSS*/
#define DISCHARGE_SAMPLE_CAP                0x3C
/*channel number that connects to RA0*/
#define ANALOG_CHANNEL                      0x00
#define UPPER_THRESHOLD                     35   
#define LOWER_THRESHOLD                     -35   
#define NUMBER_OF_REPETITIONS               16    /*maximum 255 repetitions*/
#define FOSC_DIVIDED_128                    0x3F
#define FRQ_1MHZ                             0x00

static void CLK_init(void);
static void PORT_init(void);
static void ADCC_init(void);
static void ADCC_dischargeSampleCap(void);
static void INTERRUPT_init(void);
static void ADCC_startConversion(uint8_t channel);
static void ADCC_ThresholdISR(void);
void __interrupt() INTERRUPT_InterruptManager(void);

uint16_t volatile errVal;

static void CLK_init(void)
{
    /* select HFINTOSC */
        OSCCON1 = _OSCCON1_NOSC1_MASK | _OSCCON1_NOSC2_MASK 
    /* clk divided by 4 */            
                | _OSCCON1_NDIV2_MASK;  
        OSCFRQ = FRQ_1MHZ;
}

static void PORT_init(void)
{
    ANSELA |= _ANSELA_ANSELA0_MASK;       /*set pin RA0 as analog*/
    TRISA |= _TRISA_TRISA0_MASK;          /*set pin RA0 as input*/
}


static void ADCC_init(void) 
{
    ADCON0 = _ADCON0_ADON_MASK     /*enable ADCC module*/
           | _ADCON0_ADCONT_MASK   /*enable continuous operation*/
           | _ADCON0_ADFM_MASK;    /*result right justified*/
    ADCLK = FOSC_DIVIDED_128;
    
    ADRPT = NUMBER_OF_REPETITIONS;
    
    /*clear status bits on overflow enabled (this setting prevents overflow 
    interrupts that  trigger the same interrupt as the threshold interrupt)*/
    ADCON2 = _ADCON2_ADACLR_MASK 
            /* Average mode */
            | _ADCON2_ADMD1_MASK
            /*result is right shifted by 16*/
            | _ADCON2_ADCRS2_MASK;  
    /* mode: error bigger than upper threshold
         or lower than lower threshold*/
    ADCON3 = _ADCON3_ADTMD2_MASK;  
    /*this register also selects the error calculation method which is the 
      difference between the last result and the current result*/

    ADUTH = UPPER_THRESHOLD;
    ADLTH = LOWER_THRESHOLD; 
 
    
    PIR1 &= (~_PIR1_ADTIF_MASK);  /* Clear the ADC Threshold interrupt flag */
    PIE1 |= _PIE1_ADTIE_MASK;     /* Enable ADCC threshold interrupt*/
    

}

static void ADCC_dischargeSampleCap(void)
{
    ADPCH = DISCHARGE_SAMPLE_CAP;
}

static void INTERRUPT_init(void)
{
    INTCON |= _INTCON_GIE_MASK     /* Enable global interrupts */
            | _INTCON_PEIE_MASK;   /* Enable peripheral interrupts */
}

static void ADCC_startConversion(uint8_t channel) 
{
    ADPCH = channel;
    
    ADCON0 |= _ADCON0_ADGO_MASK;  /* Start the conversion */
}

static void ADCC_ThresholdISR(void)
{
    /*clear interrupt flag*/
    PIR1 &= (~_PIR1_ADTIF_MASK);
    /*read the error*/
    errVal =  ((ADERRH << 8) + ADERRL);
}


void __interrupt() INTERRUPT_InterruptManager(void) 
{
    if (INTCON & _INTCON_PEIE_MASK) 
    {
        if ((PIE1 & _PIE1_ADTIE_MASK) && (PIR1 & _PIR1_ADTIF_MASK))
            
        {
            ADCC_ThresholdISR();
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
        
    ADCC_startConversion(ANALOG_CHANNEL);

    while (1) 
    {
        ;
    }
}
